#include "sea_port/port_simulation.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace sea_port {
namespace {

constexpr double kUrgentPriority = 5.0e14;
constexpr double kFairnessPriority = 1.0e15;
constexpr double kRatingLossForAbandonment = -2.5;
constexpr double kRatingLossForRejection = -1.5;

std::string ShipLabel(const Ship& ship) { return ship.name() + " (" + ship.ship_type_name() + ")"; }

}  // namespace

PortSimulation::PortSimulation(SimulationConfig config)
    : config_(config),
      random_generator_(config_.random_seed),
      weather_by_day_(static_cast<std::size_t>(config_.simulation_days + 1)) {
  ValidateConfig();
  BuildCranes();
}

void PortSimulation::ValidateConfig() const {
  if (config_.simulation_days < 1 || config_.step_days < 1 || config_.step_days > 3) {
    throw std::invalid_argument("Simulation days must be positive and step must be 1..3");
  }
  if (config_.waiting_capacity == 0) {
    throw std::invalid_argument("Waiting capacity must be positive");
  }
  for (std::size_t index = 0; index < kCargoTypeCount; ++index) {
    if (config_.crane_counts[index] < 0 || config_.unloading_places[index] < 0 ||
        config_.crane_purchase_cost[index] < 0.0) {
      throw std::invalid_argument("Crane, berth, and cost values cannot be negative");
    }
  }
  if (config_.arrival_deviation_min_days > config_.arrival_deviation_max_days ||
      config_.unloading_delay_min_days > config_.unloading_delay_max_days ||
      config_.penalty_per_extra_day < 0.0 || config_.current_day_arrival_rate < 0.0 ||
      config_.future_arrival_decay < 0.0 || config_.fairness_wait_days < 0) {
    throw std::invalid_argument("Invalid simulation configuration range");
  }
}

void PortSimulation::BuildCranes() {
  for (std::size_t type_index = 0; type_index < kCargoTypeCount; ++type_index) {
    const CargoType type = static_cast<CargoType>(type_index);
    for (int number = 1; number <= config_.crane_counts[type_index]; ++number) {
      const std::string name =
          "CRANE-" + std::to_string(type_index + 1) + "-" + std::to_string(number);
      switch (type) {
        case CargoType::kBulk:
          cranes_.push_back(std::make_unique<BulkCrane>(next_entity_id_++, name));
          break;
        case CargoType::kLiquid:
          cranes_.push_back(std::make_unique<LiquidCrane>(next_entity_id_++, name));
          break;
        case CargoType::kContainer:
          cranes_.push_back(std::make_unique<ContainerCrane>(next_entity_id_++, name));
          break;
        case CargoType::kCount:
          throw std::logic_error("Invalid crane type");
      }
    }
  }
}

std::uint64_t PortSimulation::ScheduleShip(const ShipSpec& spec) {
  if (finalized_) {
    throw std::logic_error("Cannot schedule a ship after finalization");
  }
  if (spec.name.empty() || spec.booking_day < 1 || spec.planned_arrival_day < spec.booking_day ||
      spec.planned_arrival_day > config_.simulation_days || spec.cargo_weight_tons <= 0.0 ||
      spec.planned_stay_days < 1 || spec.max_wait_days < 0) {
    throw std::invalid_argument("Invalid ship specification");
  }

  const int actual_arrival = spec.actual_arrival_day.value_or(
      random_generator_.GenerateActualArrivalDay(spec, config_, spec.booking_day));
  const int unloading_delay =
      spec.unloading_delay_days.value_or(random_generator_.GenerateUnloadingDelay(config_));
  if (actual_arrival < spec.booking_day || unloading_delay < 0) {
    throw std::invalid_argument("Invalid fixed arrival or unloading delay");
  }

  const std::uint64_t id = next_entity_id_++;
  scheduled_ships_.push_back(CreateShip(id, spec, actual_arrival, unloading_delay));
  return id;
}

void PortSimulation::SetWeather(int day, const Weather& weather) {
  if (day < current_day_ || day > config_.simulation_days || weather.wave_level < 0 ||
      weather.wave_level > 5 || weather.operability < 0.0 || weather.operability > 1.0) {
    throw std::invalid_argument("Invalid weather day or values");
  }
  weather_by_day_[static_cast<std::size_t>(day)] = weather;
}

void PortSimulation::GenerateRandomParametersForDay(int day, int planning_horizon_days) {
  if (day < current_day_ || day > config_.simulation_days || planning_horizon_days < 1 ||
      planning_horizon_days > 7) {
    throw std::invalid_argument("Random planning horizon must be between 1 and 7 days");
  }
  if (!weather_by_day_[static_cast<std::size_t>(day)].has_value()) {
    SetWeather(day, random_generator_.GenerateWeather(config_.season));
  }

  const int last_planned_day = std::min(config_.simulation_days, day + planning_horizon_days - 1);
  for (int planned_day = day; planned_day <= last_planned_day; ++planned_day) {
    const int offset = planned_day - day;
    const int count = random_generator_.GenerateBookingCount(config_.current_day_arrival_rate,
                                                             config_.future_arrival_decay, offset);
    for (int ship_number = 0; ship_number < count; ++ship_number) {
      ScheduleShip(random_generator_.GenerateShipSpec(day, planned_day));
    }
  }
}

DailyReport PortSimulation::SimulateDay() {
  if (current_day_ > config_.simulation_days) {
    throw std::out_of_range("The simulation month has ended");
  }
  if (finalized_) {
    throw std::logic_error("Simulation is finalized");
  }

  auto& weather = weather_by_day_[static_cast<std::size_t>(current_day_)];
  if (!weather.has_value()) {
    weather = random_generator_.GenerateWeather(config_.season);
  }

  DailyReport report;
  report.day = current_day_;
  report.weather = *weather;
  ProcessArrivals(report);
  ProcessAbandonments(report);
  AssignWaitingShips(report);
  ProcessUnloading(*weather, report);

  const std::size_t queue_length = waiting_ship_count();
  statistics_.RecordQueueLength(queue_length);
  report.waiting = queue_length;
  report.unloading = active_unloadings_.size();
  for (std::size_t type_index = 0; type_index < kCargoTypeCount; ++type_index) {
    queues_[type_index].ForEach([this, &report, type_index](const Ship& ship) {
      report.queues[type_index].push_back(ship.name() + " (ожидает " +
                                          std::to_string(current_day_ - ship.actual_arrival_day()) +
                                          " дн.)");
    });
  }
  for (const auto& unloading : active_unloadings_) {
    report.active_operations.push_back(unloading.ship->name() + " — " + unloading.crane->name());
  }

  // Persistent long queues reduce reputation even before ships abandon the port.
  std::size_t overdue_waiters = 0;
  for (const auto& queue : queues_) {
    queue.ForEach([this, &overdue_waiters](const Ship& ship) {
      if (current_day_ - ship.actual_arrival_day() >= config_.fairness_wait_days) {
        ++overdue_waiters;
      }
    });
  }
  AdjustRating(-0.05 * static_cast<double>(overdue_waiters));
  report.rating = rating_;
  ++current_day_;
  return report;
}

void PortSimulation::ProcessArrivals(DailyReport& report) {
  std::size_t index = 0;
  while (index < scheduled_ships_.size()) {
    if (scheduled_ships_[index]->actual_arrival_day() > current_day_) {
      ++index;
      continue;
    }

    std::unique_ptr<Ship> ship = std::move(scheduled_ships_[index]);
    scheduled_ships_.erase(scheduled_ships_.begin() + static_cast<std::ptrdiff_t>(index));
    statistics_.RecordArrival();
    ++report.arrived;
    if (waiting_ship_count() >= config_.waiting_capacity) {
      ship->set_state(ShipState::kRejected);
      statistics_.RecordRejection();
      ++report.rejected;
      AdjustRating(kRatingLossForRejection);
      report.events.push_back(ShipLabel(*ship) + ": отказано — нет места ожидания");
      continue;
    }

    ship->set_state(ShipState::kWaiting);
    report.events.push_back(ShipLabel(*ship) + ": прибыл в порт");
    queues_[CargoIndex(ship->cargo().type())].Push(std::move(ship));
  }
}

void PortSimulation::ProcessAbandonments(DailyReport& report) {
  for (auto& queue : queues_) {
    auto abandoned = queue.RemoveIf([this](const Ship& ship) {
      return current_day_ - ship.actual_arrival_day() >= ship.max_wait_days();
    });
    for (auto& ship : abandoned) {
      ship->set_state(ShipState::kAbandoned);
      statistics_.RecordAbandonment();
      ++report.abandoned;
      AdjustRating(kRatingLossForAbandonment);
      report.events.push_back(ShipLabel(*ship) + ": ушёл, не дождавшись разгрузки");
    }
  }
}

void PortSimulation::AssignWaitingShips(DailyReport& report) {
  for (std::size_t type_index = 0; type_index < kCargoTypeCount; ++type_index) {
    const CargoType type = static_cast<CargoType>(type_index);
    while (!queues_[type_index].empty() &&
           ActiveCount(type) < static_cast<std::size_t>(config_.unloading_places[type_index])) {
      Crane* crane = FindAvailableCrane(type);
      if (crane == nullptr) {
        break;
      }

      std::unique_ptr<Ship> ship = queues_[type_index].ExtractBest(
          [this](const Ship& candidate) { return PriorityScore(candidate); });
      ship->set_state(ShipState::kUnloading);
      ship->set_unloading_start_day(current_day_);
      crane->Assign(ship->id());
      report.events.push_back(ShipLabel(*ship) + ": начата разгрузка краном " + crane->name());
      active_unloadings_.push_back(ActiveUnloading{.ship = std::move(ship),
                                                   .crane = crane,
                                                   .start_day = current_day_,
                                                   .remaining_tons = 0.0,
                                                   .finish_delay_remaining = 0,
                                                   .cargo_finished = false});
      active_unloadings_.back().remaining_tons =
          active_unloadings_.back().ship->cargo().weight_tons();
      active_unloadings_.back().finish_delay_remaining =
          active_unloadings_.back().ship->unloading_delay_days();
      ++report.started_unloading;
    }
  }
}

void PortSimulation::ProcessUnloading(const Weather& weather, DailyReport& report) {
  std::size_t index = 0;
  while (index < active_unloadings_.size()) {
    ActiveUnloading& unloading = active_unloadings_[index];
    bool completed = false;
    if (unloading.cargo_finished) {
      if (unloading.finish_delay_remaining > 0) {
        --unloading.finish_delay_remaining;
      }
      completed = unloading.finish_delay_remaining == 0;
    } else {
      unloading.remaining_tons -= unloading.crane->DailyThroughput(weather);
      if (unloading.remaining_tons <= 0.0) {
        unloading.cargo_finished = true;
        completed = unloading.finish_delay_remaining == 0;
      }
    }

    if (!completed) {
      ++index;
      continue;
    }

    CompletionRecord record = CompleteUnloading(unloading);
    unloading.ship->set_state(ShipState::kCompleted);
    unloading.crane->Release();
    statistics_.RecordCompletion(record);
    ++report.completed;
    report.daily_net_revenue += record.net_revenue;
    if (record.waiting_days == 0) {
      AdjustRating(0.8);
    } else if (record.waiting_days <= 2) {
      AdjustRating(0.3);
    }
    report.events.push_back(record.ship_name + ": разгрузка завершена, результат " +
                            std::to_string(static_cast<long long>(record.net_revenue)) + " у.е.");
    active_unloadings_.erase(active_unloadings_.begin() + static_cast<std::ptrdiff_t>(index));
  }
}

CompletionRecord PortSimulation::CompleteUnloading(ActiveUnloading& unloading) const {
  const Ship& ship = *unloading.ship;
  const int waiting_days = unloading.start_day - ship.actual_arrival_day();
  const int total_port_days = current_day_ - ship.actual_arrival_day() + 1;
  const int extra_port_days = std::max(0, total_port_days - ship.planned_stay_days());
  const double base_revenue = ship.potential_revenue();
  const double credited_revenue = base_revenue * RevenueMultiplier(waiting_days);
  const double penalty = config_.penalty_per_extra_day * extra_port_days;

  return CompletionRecord{.ship_id = ship.id(),
                          .ship_name = ship.name(),
                          .cargo_type = ship.cargo().type(),
                          .planned_arrival_day = ship.planned_arrival_day(),
                          .actual_arrival_day = ship.actual_arrival_day(),
                          .unloading_start_day = unloading.start_day,
                          .completion_day = current_day_,
                          .waiting_days = waiting_days,
                          .unloading_duration_days = current_day_ - unloading.start_day + 1,
                          .additional_unloading_delay_days = ship.unloading_delay_days(),
                          .extra_port_days = extra_port_days,
                          .base_revenue = base_revenue,
                          .credited_revenue = credited_revenue,
                          .penalty = penalty,
                          .net_revenue = credited_revenue - penalty};
}

Crane* PortSimulation::FindAvailableCrane(CargoType type) {
  for (const auto& crane : cranes_) {
    if (crane->cargo_type() == type && !crane->is_busy()) {
      return crane.get();
    }
  }
  return nullptr;
}

std::size_t PortSimulation::ActiveCount(CargoType type) const {
  return static_cast<std::size_t>(std::count_if(
      active_unloadings_.begin(), active_unloadings_.end(),
      [type](const ActiveUnloading& item) { return item.ship->cargo().type() == type; }));
}

double PortSimulation::PriorityScore(const Ship& ship) const {
  const int waiting_days = current_day_ - ship.actual_arrival_day();
  const int patience_left = ship.max_wait_days() - waiting_days;
  double score =
      ship.potential_revenue() + static_cast<double>(waiting_days) * config_.waiting_priority_bonus;
  if (patience_left <= 1) {
    score += kUrgentPriority;
  }
  if (waiting_days >= config_.fairness_wait_days) {
    score += kFairnessPriority + static_cast<double>(waiting_days) * 1.0e9;
  }
  return score;
}

double PortSimulation::RevenueMultiplier(int waiting_days) const {
  if (waiting_days == 0) {
    return 1.12;
  }
  if (waiting_days == 1) {
    return 1.06;
  }
  return std::max(0.55, 1.0 - 0.04 * static_cast<double>(waiting_days - 1));
}

void PortSimulation::AdjustRating(double delta) {
  rating_ = std::clamp(rating_ + delta, 0.0, 100.0);
}

void PortSimulation::RunRandomMonth() {
  while (current_day_ <= config_.simulation_days) {
    GenerateRandomParametersForDay(current_day_, 7);
    SimulateDay();
  }
  Finalize();
}

void PortSimulation::Finalize() {
  if (finalized_) {
    return;
  }
  statistics_.set_unfinished_ships(waiting_ship_count() + active_unloadings_.size());
  finalized_ = true;
}

int PortSimulation::current_day() const { return current_day_; }
double PortSimulation::rating() const { return rating_; }

std::size_t PortSimulation::waiting_ship_count() const {
  std::size_t count = 0;
  for (const auto& queue : queues_) {
    count += queue.size();
  }
  return count;
}

std::size_t PortSimulation::active_unloading_count() const { return active_unloadings_.size(); }

std::size_t PortSimulation::scheduled_ship_count() const { return scheduled_ships_.size(); }

double PortSimulation::CranePurchaseCost() const {
  double cost = 0.0;
  for (std::size_t index = 0; index < kCargoTypeCount; ++index) {
    cost += static_cast<double>(config_.crane_counts[index]) * config_.crane_purchase_cost[index];
  }
  return cost;
}

const SimulationConfig& PortSimulation::config() const { return config_; }
const Statistics& PortSimulation::statistics() const { return statistics_; }

}  // namespace sea_port
