#include "sea_port/cargo.h"

#include <stdexcept>

namespace sea_port {
namespace {

constexpr double kGrainRevenuePerTon = 45.0;
constexpr double kOilRevenuePerTon = 80.0;
constexpr double kContainerRevenuePerTon = 110.0;

}  // namespace

Cargo::Cargo(double weight_tons,  // NOLINT(bugprone-easily-swappable-parameters)
             double revenue_per_ton)
    : weight_tons_(weight_tons), revenue_per_ton_(revenue_per_ton) {
  if (weight_tons_ <= 0.0 || revenue_per_ton_ <= 0.0) {
    throw std::invalid_argument("Cargo weight and revenue must be positive");
  }
}

double Cargo::weight_tons() const { return weight_tons_; }

double Cargo::revenue_per_ton() const { return revenue_per_ton_; }

double Cargo::base_revenue() const { return weight_tons_ * revenue_per_ton_; }

GrainCargo::GrainCargo(double weight_tons) : Cargo(weight_tons, kGrainRevenuePerTon) {}

CargoType GrainCargo::type() const { return CargoType::kBulk; }

std::string GrainCargo::name() const { return "зерно"; }

OilCargo::OilCargo(double weight_tons) : Cargo(weight_tons, kOilRevenuePerTon) {}

CargoType OilCargo::type() const { return CargoType::kLiquid; }

std::string OilCargo::name() const { return "нефть"; }

ContainerCargo::ContainerCargo(double weight_tons) : Cargo(weight_tons, kContainerRevenuePerTon) {}

CargoType ContainerCargo::type() const { return CargoType::kContainer; }

std::string ContainerCargo::name() const { return "контейнеры"; }

std::unique_ptr<Cargo> CreateCargo(CargoType type, double weight_tons) {
  switch (type) {
    case CargoType::kBulk:
      return std::make_unique<GrainCargo>(weight_tons);
    case CargoType::kLiquid:
      return std::make_unique<OilCargo>(weight_tons);
    case CargoType::kContainer:
      return std::make_unique<ContainerCargo>(weight_tons);
    case CargoType::kCount:
      break;
  }
  throw std::invalid_argument("Cannot create cargo with unknown type");
}

}  // namespace sea_port
