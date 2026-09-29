#ifndef SEA_PORT_CARGO_H_
#define SEA_PORT_CARGO_H_

#include <memory>
#include <string>

#include "sea_port/types.h"

namespace sea_port {

class Cargo {
 public:
  Cargo(double weight_tons, double revenue_per_ton);
  virtual ~Cargo() = default;

  Cargo(const Cargo&) = delete;
  Cargo& operator=(const Cargo&) = delete;

  double weight_tons() const;
  double revenue_per_ton() const;
  double base_revenue() const;
  virtual CargoType type() const = 0;
  virtual std::string name() const = 0;

 private:
  double weight_tons_;
  double revenue_per_ton_;
};

class GrainCargo final : public Cargo {
 public:
  explicit GrainCargo(double weight_tons);
  CargoType type() const override;
  std::string name() const override;
};

class OilCargo final : public Cargo {
 public:
  explicit OilCargo(double weight_tons);
  CargoType type() const override;
  std::string name() const override;
};

class ContainerCargo final : public Cargo {
 public:
  explicit ContainerCargo(double weight_tons);
  CargoType type() const override;
  std::string name() const override;
};

std::unique_ptr<Cargo> CreateCargo(CargoType type, double weight_tons);

}  // namespace sea_port

#endif  // SEA_PORT_CARGO_H_
