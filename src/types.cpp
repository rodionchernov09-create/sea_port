#include "sea_port/types.h"

#include <stdexcept>

namespace sea_port {

std::string ToString(CargoType type) {
  switch (type) {
    case CargoType::kBulk:
      return "bulk";
    case CargoType::kLiquid:
      return "liquid";
    case CargoType::kContainer:
      return "container";
    case CargoType::kCount:
      break;
  }
  throw std::invalid_argument("Unknown cargo type");
}

std::string ToString(Season season) {
  switch (season) {
    case Season::kWinter:
      return "winter";
    case Season::kSpring:
      return "spring";
    case Season::kSummer:
      return "summer";
    case Season::kAutumn:
      return "autumn";
  }
  throw std::invalid_argument("Unknown season");
}

std::string ToString(ShipState state) {
  switch (state) {
    case ShipState::kScheduled:
      return "sceduled";
    case ShipState::kWaiting:
      return "waiting";
    case ShipState::kUnloading:
      return "unloading";
    case ShipState::kCompleted:
      return "completed";
    case ShipState::kAbandoned:
      return "gone";
    case ShipState::kRejected:
      return "rejected";
  }
  throw std::invalid_argument("Unknown ship state");
}

}  // namespace sea_port
