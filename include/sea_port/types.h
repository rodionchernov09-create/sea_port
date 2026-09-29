#ifndef SEA_PORT_TYPES_H_
#define SEA_PORT_TYPES_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sea_port {

enum class EntityType : std::uint8_t { kShip, kCrane };
enum class CargoType : std::uint8_t { kBulk = 0, kLiquid = 1, kContainer = 2, kCount = 3 };
enum class Season : std::uint8_t { kWinter = 0, kSpring = 1, kSummer = 2, kAutumn = 3 };
enum class ShipState : std::uint8_t {
  kScheduled,
  kWaiting,
  kUnloading,
  kCompleted,
  kAbandoned,
  kRejected
};

constexpr std::size_t kCargoTypeCount = static_cast<std::size_t>(CargoType::kCount);

constexpr std::size_t CargoIndex(CargoType type) { return static_cast<std::size_t>(type); }

std::string ToString(CargoType type);
std::string ToString(Season season);
std::string ToString(ShipState state);

struct Weather {
  int wave_level = 0;
  double operability = 1.0;
  bool storm = false;
};

struct SimulationConfig {
  Season season = Season::kSummer;
  int simulation_days = 30;
  int step_days = 1;
  std::array<int, kCargoTypeCount> crane_counts = {2, 1, 1};
  std::array<int, kCargoTypeCount> unloading_places = {4, 3, 3};
  std::size_t waiting_capacity = 20;

  int arrival_deviation_min_days = -2;
  int arrival_deviation_max_days = 9;
  int unloading_delay_min_days = 0;
  int unloading_delay_max_days = 12;
  double penalty_per_extra_day = 2000.0;

  double current_day_arrival_rate = 0.55;
  double future_arrival_decay = 0.48;
  int fairness_wait_days = 4;
  double waiting_priority_bonus = 120000.0;
  double minimum_rating_for_optimization = 70.0;
  double minimum_service_rate_for_optimization = 0.35;

  std::array<double, kCargoTypeCount> crane_purchase_cost = {600000.0, 900000.0, 1200000.0};
  std::uint64_t random_seed = 5489U;
};

struct ShipSpec {
  CargoType cargo_type = CargoType::kBulk;
  std::string name;
  int booking_day = 1;
  int planned_arrival_day = 1;
  double cargo_weight_tons = 1000.0;
  int planned_stay_days = 3;
  int max_wait_days = 5;
  std::optional<int> actual_arrival_day;
  std::optional<int> unloading_delay_days;
};

struct CompletionRecord {
  std::uint64_t ship_id = 0;
  std::string ship_name;
  CargoType cargo_type = CargoType::kBulk;
  int planned_arrival_day = 0;
  int actual_arrival_day = 0;
  int unloading_start_day = 0;
  int completion_day = 0;
  int waiting_days = 0;
  int unloading_duration_days = 0;
  int additional_unloading_delay_days = 0;
  int extra_port_days = 0;
  double base_revenue = 0.0;
  double credited_revenue = 0.0;
  double penalty = 0.0;
  double net_revenue = 0.0;
};

struct DailyReport {
  int day = 0;
  Weather weather;
  int arrived = 0;
  int started_unloading = 0;
  int completed = 0;
  int abandoned = 0;
  int rejected = 0;
  std::size_t waiting = 0;
  std::size_t unloading = 0;
  double daily_net_revenue = 0.0;
  double rating = 100.0;
  std::array<std::vector<std::string>, kCargoTypeCount> queues;
  std::vector<std::string> active_operations;
  std::vector<std::string> events;
};

}  // namespace sea_port

#endif  // SEA_PORT_TYPES_H_
