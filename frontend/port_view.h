#pragma once
#include "imgui.h"
#include "sea_port/port_simulation.h"
#include <map>
#include <vector>

class PortView {
 public:
  void Reset();
  void Update(const sea_port::PortSimulation& simulation, const sea_port::DailyReport& report);
  void Sync(const sea_port::PortSimulation& simulation);
  bool IsAnimating() const;
  void Draw(const sea_port::PortSimulation& simulation, float delta, float maxHeight);
  void DrawQueues(const sea_port::PortSimulation& simulation);
 private:
  struct Vessel {
    sea_port::ShipView data;
    ImVec2 position{};
    std::vector<ImVec2> route;
    float age = 0;
    float delay = 0;
    float duration = 0;
    bool leaving = false;
    bool appearing = false;
  };
  std::map<std::uint64_t, Vessel> vessels_;
  std::vector<sea_port::ShipView> snapshot_;
  float laneHeight_ = 170;
  float width_ = 1120;
  float clock_ = 0;
  ImVec2 Target(const sea_port::ShipView& ship) const;
};
