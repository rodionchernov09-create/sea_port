#ifndef SEA_PORT_PORT_SIMULATION_H_
#define SEA_PORT_PORT_SIMULATION_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "sea_port/crane.h"
#include "sea_port/random_generator.h"
#include "sea_port/ship_queue.h"
#include "sea_port/statistics.h"
#include "sea_port/types.h"

namespace sea_port {

class PortSimulation {
 public:
  explicit PortSimulation(SimulationConfig config);

  std::uint64_t ScheduleShip(const ShipSpec& spec);
  void SetWeather(int day, const Weather& weather);
  void GenerateRandomParametersForDay(int day, int planning_horizon_days = 7);
  DailyReport SimulateDay();
  void RunRandomMonth();
  void Finalize();

  int current_day() const;
  double rating() const;
  std::size_t waiting_ship_count() const;
  std::size_t active_unloading_count() const;
  std::size_t scheduled_ship_count() const;
  double CranePurchaseCost() const;
  const SimulationConfig& config() const;
  const Statistics& statistics() const;

 private:
  struct ActiveUnloading {
    std::unique_ptr<Ship> ship;
    Crane* crane = nullptr;
    int start_day = 0;
    double remaining_tons = 0.0;
    int finish_delay_remaining = 0;
    bool cargo_finished = false;
  };

  void ValidateConfig() const;
  void BuildCranes();
  void ProcessArrivals(DailyReport& report);
  void ProcessAbandonments(DailyReport& report);
  void AssignWaitingShips(DailyReport& report);
  void ProcessUnloading(const Weather& weather, DailyReport& report);
  CompletionRecord CompleteUnloading(ActiveUnloading& unloading) const;
  Crane* FindAvailableCrane(CargoType type);
  std::size_t ActiveCount(CargoType type) const;
  double PriorityScore(const Ship& ship) const;
  double RevenueMultiplier(int waiting_days) const;
  void AdjustRating(double delta);

  SimulationConfig config_;
  RandomGenerator random_generator_;
  int current_day_ = 1;
  std::uint64_t next_entity_id_ = 1;
  double rating_ = 100.0;
  bool finalized_ = false;

  std::array<ShipQueue, kCargoTypeCount> queues_;
  std::vector<std::unique_ptr<Crane>> cranes_;
  std::vector<std::unique_ptr<Ship>> scheduled_ships_;
  std::vector<ActiveUnloading> active_unloadings_;
  std::vector<std::optional<Weather>> weather_by_day_;
  Statistics statistics_;
};

}  // namespace sea_port

#endif  // SEA_PORT_PORT_SIMULATION_H_
