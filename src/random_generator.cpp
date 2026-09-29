#include "sea_port/random_generator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "sea_port/weather.h"

namespace sea_port {

RandomGenerator::RandomGenerator(std::uint64_t seed) : engine_(seed) {}

Weather RandomGenerator::GenerateWeather(Season season) {
  // Seasonal weights for wave levels 0..5.
  constexpr std::array<std::array<double, 6>, 4> kWaveWeights = {{
      {0.05, 0.12, 0.22, 0.28, 0.23, 0.10},  // winter
      {0.14, 0.24, 0.28, 0.20, 0.10, 0.04},  // spring
      {0.30, 0.32, 0.22, 0.11, 0.04, 0.01},  // summer
      {0.10, 0.20, 0.27, 0.24, 0.14, 0.05},  // autumn
  }};
  const auto season_index = static_cast<std::size_t>(season);
  std::discrete_distribution<int> wave_distribution(kWaveWeights[season_index].begin(),
                                                    kWaveWeights[season_index].end());
  return WeatherFromWaveLevel(wave_distribution(engine_));
}

int RandomGenerator::GenerateBookingCount(double current_day_rate, double decay,
                                          int future_offset) {
  if (current_day_rate < 0.0 || decay < 0.0 || future_offset < 0) {
    throw std::invalid_argument("Invalid booking distribution parameters");
  }
  const double lambda = current_day_rate * std::exp(-decay * future_offset);
  std::poisson_distribution<int> distribution(lambda);
  return distribution(engine_);
}

ShipSpec RandomGenerator::GenerateShipSpec(int booking_day, int planned_arrival_day) {
  // Expensive container and liquid ships are deliberately less frequent.
  std::discrete_distribution<int> type_distribution({0.58, 0.27, 0.15});
  const auto cargo_type = static_cast<CargoType>(type_distribution(engine_));

  std::uniform_real_distribution<double> weight_fraction(0.0, 1.0);
  const double fraction = weight_fraction(engine_);
  double minimum_weight = 0.0;
  double maximum_weight = 0.0;
  switch (cargo_type) {
    case CargoType::kBulk:
      minimum_weight = 1500.0;
      maximum_weight = 9000.0;
      break;
    case CargoType::kLiquid:
      minimum_weight = 1200.0;
      maximum_weight = 7000.0;
      break;
    case CargoType::kContainer:
      minimum_weight = 800.0;
      maximum_weight = 5000.0;
      break;
    case CargoType::kCount:
      throw std::logic_error("Random generator produced an invalid cargo type");
  }

  ++generated_ship_number_;
  std::ostringstream name;
  name << "AUTO-" << std::setw(4) << std::setfill('0') << generated_ship_number_;
  std::uniform_int_distribution<int> planned_stay(2, 7);
  std::uniform_int_distribution<int> patience(2, 9);

  return ShipSpec{
      .cargo_type = cargo_type,
      .name = name.str(),
      .booking_day = booking_day,
      .planned_arrival_day = planned_arrival_day,
      .cargo_weight_tons = minimum_weight + fraction * (maximum_weight - minimum_weight),
      .planned_stay_days = planned_stay(engine_),
      .max_wait_days = patience(engine_),
      .actual_arrival_day = std::nullopt,
      .unloading_delay_days = std::nullopt};
}

int RandomGenerator::GenerateActualArrivalDay(const ShipSpec& spec, const SimulationConfig& config,
                                              int earliest_possible_day) {
  if (config.arrival_deviation_min_days > config.arrival_deviation_max_days) {
    throw std::invalid_argument("Invalid arrival deviation range");
  }
  std::uniform_int_distribution<int> deviation(config.arrival_deviation_min_days,
                                               config.arrival_deviation_max_days);
  return std::max(spec.planned_arrival_day + deviation(engine_), earliest_possible_day);
}

int RandomGenerator::GenerateUnloadingDelay(const SimulationConfig& config) {
  if (config.unloading_delay_min_days > config.unloading_delay_max_days) {
    throw std::invalid_argument("Invalid unloading delay range");
  }
  std::uniform_int_distribution<int> delay(config.unloading_delay_min_days,
                                           config.unloading_delay_max_days);
  return delay(engine_);
}

}  // namespace sea_port
