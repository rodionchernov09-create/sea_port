#include "sea_port/weather.h"

#include <algorithm>
#include <stdexcept>

namespace sea_port {

Weather WeatherFromWaveLevel(int wave_level) {
  if (wave_level < 0 || wave_level > 5) {
    throw std::invalid_argument("Wave level must be between 0 and 5");
  }

  constexpr double kOperabilityByWave[] = {1.0, 0.94, 0.82, 0.62, 0.32, 0.0};
  return Weather{.wave_level = wave_level,
                 .operability = kOperabilityByWave[wave_level],
                 .storm = wave_level >= 4};
}

}  // namespace sea_port
