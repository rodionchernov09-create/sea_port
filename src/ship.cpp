#include "sea_port/ship.h"

#include <stdexcept>
#include <utility>

namespace sea_port {

Ship::Ship(std::uint64_t id, std::string name,
           int booking_day,  // NOLINT(bugprone-easily-swappable-parameters)
           int planned_arrival_day, int actual_arrival_day, int planned_stay_days,
           int max_wait_days, int unloading_delay_days, std::unique_ptr<Cargo> cargo)
    : SeaPort(id, std::move(name)),
      booking_day_(booking_day),
      planned_arrival_day_(planned_arrival_day),
      actual_arrival_day_(actual_arrival_day),
      planned_stay_days_(planned_stay_days),
      max_wait_days_(max_wait_days),
      unloading_delay_days_(unloading_delay_days),
      cargo_(std::move(cargo)) {
  if (booking_day_ < 1 || planned_arrival_day_ < 1 || actual_arrival_day_ < 1) {
    throw std::invalid_argument("Booking and arrival days must be positive");
  }
  if (planned_stay_days_ < 1 || max_wait_days_ < 0 || unloading_delay_days_ < 0) {
    throw std::invalid_argument("Invalid ship time parameters");
  }
  if (cargo_ == nullptr) {
    throw std::invalid_argument("Ship cargo cannot be null");
  }
}

EntityType Ship::entity_type() const { return EntityType::kShip; }

int Ship::booking_day() const { return booking_day_; }
int Ship::planned_arrival_day() const { return planned_arrival_day_; }
int Ship::actual_arrival_day() const { return actual_arrival_day_; }
int Ship::planned_stay_days() const { return planned_stay_days_; }
int Ship::max_wait_days() const { return max_wait_days_; }
int Ship::unloading_delay_days() const { return unloading_delay_days_; }
int Ship::unloading_start_day() const { return unloading_start_day_; }
ShipState Ship::state() const { return state_; }
const Cargo& Ship::cargo() const { return *cargo_; }
double Ship::potential_revenue() const { return cargo_->base_revenue(); }

void Ship::set_state(ShipState state) { state_ = state; }

void Ship::set_unloading_start_day(int day) {
  if (day < actual_arrival_day_) {
    throw std::invalid_argument("Unloading cannot start before arrival");
  }
  unloading_start_day_ = day;
}

BulkCarrier::BulkCarrier(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
                         int unloading_delay_days)
    : Ship(id, spec.name, spec.booking_day, spec.planned_arrival_day, actual_arrival_day,
           spec.planned_stay_days, spec.max_wait_days, unloading_delay_days,
           std::make_unique<GrainCargo>(spec.cargo_weight_tons)) {}

std::string BulkCarrier::ship_type_name() const { return "сухогруз"; }
CargoType BulkCarrier::supported_cargo_type() const { return CargoType::kBulk; }

Tanker::Tanker(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
               int unloading_delay_days)
    : Ship(id, spec.name, spec.booking_day, spec.planned_arrival_day, actual_arrival_day,
           spec.planned_stay_days, spec.max_wait_days, unloading_delay_days,
           std::make_unique<OilCargo>(spec.cargo_weight_tons)) {}

std::string Tanker::ship_type_name() const { return "танкер"; }
CargoType Tanker::supported_cargo_type() const { return CargoType::kLiquid; }

ContainerShip::ContainerShip(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
                             int unloading_delay_days)
    : Ship(id, spec.name, spec.booking_day, spec.planned_arrival_day, actual_arrival_day,
           spec.planned_stay_days, spec.max_wait_days, unloading_delay_days,
           std::make_unique<ContainerCargo>(spec.cargo_weight_tons)) {}

std::string ContainerShip::ship_type_name() const { return "контейнеровоз"; }
CargoType ContainerShip::supported_cargo_type() const { return CargoType::kContainer; }

std::unique_ptr<Ship> CreateShip(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
                                 int unloading_delay_days) {
  switch (spec.cargo_type) {
    case CargoType::kBulk:
      return std::make_unique<BulkCarrier>(id, spec, actual_arrival_day, unloading_delay_days);
    case CargoType::kLiquid:
      return std::make_unique<Tanker>(id, spec, actual_arrival_day, unloading_delay_days);
    case CargoType::kContainer:
      return std::make_unique<ContainerShip>(id, spec, actual_arrival_day, unloading_delay_days);
    case CargoType::kCount:
      break;
  }
  throw std::invalid_argument("Cannot create ship with unknown cargo type");
}

}  // namespace sea_port
