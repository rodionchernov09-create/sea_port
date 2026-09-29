#ifndef SEA_PORT_CRANE_H_
#define SEA_PORT_CRANE_H_

#include <cstdint>
#include <optional>
#include <string>

#include "sea_port/sea_port.h"

namespace sea_port {

class Crane : public SeaPort {
 public:
  Crane(std::uint64_t id, std::string name, CargoType cargo_type,
        double base_throughput_tons_per_day);
  ~Crane() override = default;

  EntityType entity_type() const final;
  virtual std::string crane_type_name() const = 0;

  CargoType cargo_type() const;
  double base_throughput_tons_per_day() const;
  double DailyThroughput(const Weather& weather) const;
  bool is_busy() const;
  std::optional<std::uint64_t> assigned_ship_id() const;
  void Assign(std::uint64_t ship_id);
  void Release();

 private:
  CargoType cargo_type_;
  double base_throughput_tons_per_day_;
  std::optional<std::uint64_t> assigned_ship_id_;
};

class BulkCrane final : public Crane {
 public:
  BulkCrane(std::uint64_t id, std::string name);
  std::string crane_type_name() const override;
};

class LiquidCrane final : public Crane {
 public:
  LiquidCrane(std::uint64_t id, std::string name);
  std::string crane_type_name() const override;
};

class ContainerCrane final : public Crane {
 public:
  ContainerCrane(std::uint64_t id, std::string name);
  std::string crane_type_name() const override;
};

}  // namespace sea_port

#endif  // SEA_PORT_CRANE_H_
