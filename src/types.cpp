#include "sea_port/types.h"

#include <stdexcept>

namespace sea_port {

std::string ToString(CargoType type) {
  switch (type) {
    case CargoType::kBulk:
      return "сыпучий";
    case CargoType::kLiquid:
      return "жидкий";
    case CargoType::kContainer:
      return "контейнерный";
    case CargoType::kCount:
      break;
  }
  throw std::invalid_argument("Unknown cargo type");
}

std::string ToString(Season season) {
  switch (season) {
    case Season::kWinter:
      return "зима";
    case Season::kSpring:
      return "весна";
    case Season::kSummer:
      return "лето";
    case Season::kAutumn:
      return "осень";
  }
  throw std::invalid_argument("Unknown season");
}

std::string ToString(ShipState state) {
  switch (state) {
    case ShipState::kScheduled:
      return "запланирован";
    case ShipState::kWaiting:
      return "ожидает";
    case ShipState::kUnloading:
      return "разгружается";
    case ShipState::kCompleted:
      return "разгружен";
    case ShipState::kAbandoned:
      return "ушёл";
    case ShipState::kRejected:
      return "не принят";
  }
  throw std::invalid_argument("Unknown ship state");
}

}  // namespace sea_port
