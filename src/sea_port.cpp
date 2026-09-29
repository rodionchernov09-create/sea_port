#include "sea_port/sea_port.h"

#include <stdexcept>
#include <utility>

namespace sea_port {

SeaPort::SeaPort(std::uint64_t id, std::string name) : id_(id), name_(std::move(name)) {
  if (id_ == 0) {
    throw std::invalid_argument("Entity id must be positive");
  }
  if (name_.empty()) {
    throw std::invalid_argument("Entity name cannot be empty");
  }
}

std::uint64_t SeaPort::id() const { return id_; }

const std::string& SeaPort::name() const { return name_; }

}  // namespace sea_port
