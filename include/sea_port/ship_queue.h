#ifndef SEA_PORT_SHIP_QUEUE_H_
#define SEA_PORT_SHIP_QUEUE_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "sea_port/ship.h"

namespace sea_port {

// A dynamically allocated singly-linked queue. Nodes own ships and the next node.
class ShipQueue {
 public:
  ShipQueue() = default;
  ~ShipQueue() = default;

  ShipQueue(const ShipQueue&) = delete;
  ShipQueue& operator=(const ShipQueue&) = delete;
  ShipQueue(ShipQueue&&) noexcept = default;
  ShipQueue& operator=(ShipQueue&&) noexcept = default;

  bool empty() const { return head_ == nullptr; }
  std::size_t size() const { return size_; }

  void Push(std::unique_ptr<Ship> ship) {
    if (ship == nullptr) {
      throw std::invalid_argument("Cannot enqueue a null ship");
    }
    auto node = std::make_unique<Node>(std::move(ship));
    Node* new_tail = node.get();
    if (tail_ == nullptr) {
      head_ = std::move(node);
    } else {
      tail_->next = std::move(node);
    }
    tail_ = new_tail;
    ++size_;
  }

  std::unique_ptr<Ship> Pop() {
    if (head_ == nullptr) {
      return nullptr;
    }
    std::unique_ptr<Node> old_head = std::move(head_);
    head_ = std::move(old_head->next);
    if (head_ == nullptr) {
      tail_ = nullptr;
    }
    --size_;
    return std::move(old_head->ship);
  }

  std::unique_ptr<Ship> ExtractBest(const std::function<double(const Ship&)>& score) {
    if (head_ == nullptr) {
      return nullptr;
    }

    Node* best = head_.get();
    Node* best_previous = nullptr;
    Node* previous = nullptr;
    for (Node* current = head_.get(); current != nullptr; current = current->next.get()) {
      if (score(*current->ship) > score(*best->ship)) {
        best = current;
        best_previous = previous;
      }
      previous = current;
    }

    if (best_previous == nullptr) {
      return Pop();
    }

    std::unique_ptr<Node> selected = std::move(best_previous->next);
    best_previous->next = std::move(selected->next);
    if (best_previous->next == nullptr) {
      tail_ = best_previous;
    }
    --size_;
    return std::move(selected->ship);
  }

  std::vector<std::unique_ptr<Ship>> RemoveIf(const std::function<bool(const Ship&)>& predicate) {
    std::vector<std::unique_ptr<Ship>> removed;
    std::unique_ptr<Node>* link = &head_;
    Node* previous = nullptr;
    while (*link != nullptr) {
      if (predicate(*(*link)->ship)) {
        std::unique_ptr<Node> selected = std::move(*link);
        *link = std::move(selected->next);
        removed.push_back(std::move(selected->ship));
        --size_;
      } else {
        previous = link->get();
        link = &((*link)->next);
      }
    }
    tail_ = previous;
    return removed;
  }

  void ForEach(const std::function<void(const Ship&)>& visitor) const {
    for (Node* current = head_.get(); current != nullptr; current = current->next.get()) {
      visitor(*current->ship);
    }
  }

 private:
  struct Node {
    explicit Node(std::unique_ptr<Ship> value) : ship(std::move(value)) {}
    std::unique_ptr<Ship> ship;
    std::unique_ptr<Node> next;
  };

  std::unique_ptr<Node> head_;
  Node* tail_ = nullptr;
  std::size_t size_ = 0;
};

}  // namespace sea_port

#endif  // SEA_PORT_SHIP_QUEUE_H_
