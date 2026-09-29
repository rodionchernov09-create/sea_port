#include "sea_port/crane.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sea_port {
namespace {

constexpr double kBulkThroughput = 1800.0;
constexpr double kLiquidThroughput = 2400.0;
constexpr double kContainerThroughput = 1200.0;

}  // namespace

Crane::Crane(std::uint64_t id, std::string name, CargoType cargo_type,
             double base_throughput_tons_per_day)
    : SeaPort(id, std::move(name)),
      cargo_type_(cargo_type),
      base_throughput_tons_per_day_(base_throughput_tons_per_day) {
  if (cargo_type_ == CargoType::kCount || base_throughput_tons_per_day_ <= 0.0) {
    throw std::invalid_argument("Invalid crane parameters");
  }
}

EntityType Crane::entity_type() const { return EntityType::kCrane; }
CargoType Crane::cargo_type() const { return cargo_type_; }
double Crane::base_throughput_tons_per_day() const { return base_throughput_tons_per_day_; }

double Crane::DailyThroughput(const Weather& weather) const {
  return base_throughput_tons_per_day_ * std::clamp(weather.operability, 0.0, 1.0);
}

bool Crane::is_busy() const { return assigned_ship_id_.has_value(); }
std::optional<std::uint64_t> Crane::assigned_ship_id() const { return assigned_ship_id_; }

void Crane::Assign(std::uint64_t ship_id) {
  if (is_busy()) {
    throw std::logic_error("Crane is already busy");
  }
  assigned_ship_id_ = ship_id;
}

void Crane::Release() { assigned_ship_id_.reset(); }

BulkCrane::BulkCrane(std::uint64_t id, std::string name)
    : Crane(id, std::move(name), CargoType::kBulk, kBulkThroughput) {}
std::string BulkCrane::crane_type_name() const { return "кран для сыпучих грузов"; }

LiquidCrane::LiquidCrane(std::uint64_t id, std::string name)
    : Crane(id, std::move(name), CargoType::kLiquid, kLiquidThroughput) {}
std::string LiquidCrane::crane_type_name() const { return "кран для жидких грузов"; }

ContainerCrane::ContainerCrane(std::uint64_t id, std::string name)
    : Crane(id, std::move(name), CargoType::kContainer, kContainerThroughput) {}
std::string ContainerCrane::crane_type_name() const { return "контейнерный кран"; }

}  // namespace sea_port
