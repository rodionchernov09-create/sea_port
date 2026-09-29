#ifndef SEA_PORT_SHIP_H_
#define SEA_PORT_SHIP_H_

#include <cstdint>
#include <memory>
#include <string>

#include "sea_port/cargo.h"
#include "sea_port/sea_port.h"

namespace sea_port {

class Ship : public SeaPort {
 public:
  Ship(std::uint64_t id, std::string name, int booking_day, int planned_arrival_day,
       int actual_arrival_day, int planned_stay_days, int max_wait_days, int unloading_delay_days,
       std::unique_ptr<Cargo> cargo);
  ~Ship() override = default;

  EntityType entity_type() const final;
  virtual std::string ship_type_name() const = 0;
  virtual CargoType supported_cargo_type() const = 0;

  int booking_day() const;
  int planned_arrival_day() const;
  int actual_arrival_day() const;
  int planned_stay_days() const;
  int max_wait_days() const;
  int unloading_delay_days() const;
  int unloading_start_day() const;
  ShipState state() const;
  const Cargo& cargo() const;
  double potential_revenue() const;

  void set_state(ShipState state);
  void set_unloading_start_day(int day);

 private:
  int booking_day_;
  int planned_arrival_day_;
  int actual_arrival_day_;
  int planned_stay_days_;
  int max_wait_days_;
  int unloading_delay_days_;
  int unloading_start_day_ = 0;
  ShipState state_ = ShipState::kScheduled;
  std::unique_ptr<Cargo> cargo_;
};

class BulkCarrier final : public Ship {
 public:
  BulkCarrier(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
              int unloading_delay_days);
  std::string ship_type_name() const override;
  CargoType supported_cargo_type() const override;
};

class Tanker final : public Ship {
 public:
  Tanker(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day, int unloading_delay_days);
  std::string ship_type_name() const override;
  CargoType supported_cargo_type() const override;
};

class ContainerShip final : public Ship {
 public:
  ContainerShip(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
                int unloading_delay_days);
  std::string ship_type_name() const override;
  CargoType supported_cargo_type() const override;
};

std::unique_ptr<Ship> CreateShip(std::uint64_t id, const ShipSpec& spec, int actual_arrival_day,
                                 int unloading_delay_days);

}  // namespace sea_port

#endif  // SEA_PORT_SHIP_H_
