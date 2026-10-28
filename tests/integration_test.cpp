#include "sea_port/port_simulation.h"
#include <iostream>
#include <stdexcept>
void check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
  using namespace sea_port;
  SimulationConfig c;
  c.crane_counts = {1, 1, 1};
  c.unloading_places = {1, 1, 1};
  c.arrival_deviation_min_days = c.arrival_deviation_max_days = 0;
  c.unloading_delay_min_days = c.unloading_delay_max_days = 0;
  PortSimulation s(c);
  for (int i = 0; i < 3; ++i) {
    ShipSpec ship; ship.name = "test" + std::to_string(i);
    ship.cargo_type = static_cast<CargoType>(i);
    ship.cargo_weight_tons = 100;
    s.ScheduleShip(ship);
  }
  s.SetWeather(1, Weather{5, 1.0, true});
  auto storm = s.SimulateDay();
  check(storm.arrived == 3 && storm.unloading == 3 && storm.completed == 0, "storm must stop all unloading");
  auto snapshot = s.VisualSnapshot();
  check(snapshot.size() == 3, "visual snapshot must include all active ships");
  for (const auto& v : snapshot) check(v.berth == 0 && v.remaining_tons == 100 && v.state == ShipState::kUnloading, "snapshot crane/progress mismatch");
  s.SetWeather(2, Weather{0, 1.0, false});
  auto calm = s.SimulateDay();
  check(calm.completed == 3 && calm.unloading == 0, "all cargo types must complete");
  check(calm.departures.size() == 3 && s.VisualSnapshot().empty(), "completed ships must leave the map");
  check(calm.departures[0].id == snapshot[0].id, "visual identity must survive completion");
  check(s.statistics().completions().size() == 3, "completion records mismatch");
  s.Finalize();
  check(s.statistics().unfinished_ships() == 0, "finalization mismatch");
  c.crane_counts = {0,0,0}; c.waiting_capacity = 1;
  PortSimulation q(c);
  ShipSpec ship; ship.name = "waiting"; ship.max_wait_days = 1;
  q.ScheduleShip(ship); ship.name = "rejected"; q.ScheduleShip(ship);
  auto day = q.SimulateDay();
  check(day.waiting == 1 && day.rejected == 1, "capacity/queue mismatch");
  check(day.departures.size() == 1 && day.departures[0].state == ShipState::kRejected, "rejection must be visible");
  auto abandoned = q.SimulateDay();
  check(abandoned.abandoned == 1 && abandoned.departures.size() == 1 && abandoned.departures[0].state == ShipState::kAbandoned, "waiting ship must visibly abandon");
  c.arrival_deviation_min_days = c.arrival_deviation_max_days = -2;
  PortSimulation early(c);
  ship.name = "early"; ship.booking_day = 1; ship.planned_arrival_day = 3;
  early.ScheduleShip(ship);
  check(early.SimulateDay().arrived == 1, "manual advance booking must allow early arrival");
  SimulationConfig parallel_config;
  parallel_config.current_day_arrival_rate = 0;
  parallel_config.crane_counts = {2, 0, 0};
  parallel_config.unloading_places = {2, 0, 0};
  parallel_config.arrival_deviation_min_days = parallel_config.arrival_deviation_max_days = 0;
  parallel_config.unloading_delay_min_days = parallel_config.unloading_delay_max_days = 0;
  PortSimulation parallel(parallel_config);
  ShipSpec first; first.name = "first"; first.cargo_weight_tons = 100;
  ShipSpec second = first; second.name = "second";
  const auto first_id = parallel.ScheduleShip(first);
  const auto second_id = parallel.ScheduleShip(second);
  parallel.SetWeather(1, Weather{0, 1.0, false});
  const auto simultaneous = parallel.SimulateDay();
  check(simultaneous.departures.size() == 2, "both parallel ships must depart");
  check(simultaneous.departures[0].id == first_id && simultaneous.departures[0].berth == 0,
        "first departure must retain its distinct berth and identity");
  check(simultaneous.departures[1].id == second_id && simultaneous.departures[1].berth == 1,
        "second departure must retain its distinct berth and identity");
  std::cout << "Integration checks passed: cargo types, weather, queues, rejection, abandonment, statistics\n";
}
