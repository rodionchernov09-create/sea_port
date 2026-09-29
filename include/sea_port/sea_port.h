#ifndef SEA_PORT_SEA_PORT_H_
#define SEA_PORT_SEA_PORT_H_

#include <cstdint>
#include <string>

#include "sea_port/types.h"

namespace sea_port {

// Common base required by the task for all physical port entities.
class SeaPort {
 public:
  SeaPort(std::uint64_t id, std::string name);
  virtual ~SeaPort() = default;

  SeaPort(const SeaPort&) = delete;
  SeaPort& operator=(const SeaPort&) = delete;
  SeaPort(SeaPort&&) noexcept = default;
  SeaPort& operator=(SeaPort&&) noexcept = default;

  std::uint64_t id() const;
  const std::string& name() const;
  virtual EntityType entity_type() const = 0;

 private:
  std::uint64_t id_;
  std::string name_;
};

}  // namespace sea_port

#endif  // SEA_PORT_SEA_PORT_H_
