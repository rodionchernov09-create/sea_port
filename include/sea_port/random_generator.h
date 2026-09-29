#ifndef SEA_PORT_RANDOM_GENERATOR_H_
#define SEA_PORT_RANDOM_GENERATOR_H_

#include <cstdint>
#include <memory>
#include <random>

#include "sea_port/ship.h"
#include "sea_port/types.h"

namespace sea_port {

class RandomGenerator {
 public:
  explicit RandomGenerator(std::uint64_t seed);

  Weather GenerateWeather(Season season);
  int GenerateBookingCount(double current_day_rate, double decay, int future_offset);
  ShipSpec GenerateShipSpec(int booking_day, int planned_arrival_day);
  int GenerateActualArrivalDay(const ShipSpec& spec, const SimulationConfig& config,
                               int earliest_possible_day);
  int GenerateUnloadingDelay(const SimulationConfig& config);

 private:
  std::mt19937_64 engine_;
  std::uint64_t generated_ship_number_ = 0;
};

}  // namespace sea_port

#endif  // SEA_PORT_RANDOM_GENERATOR_H_
