#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "sea_port/port_simulation.h"
#include "sea_port/types.h"

namespace {

struct ScheduledShip {
  sea_port::ShipSpec spec;
};

void DrawShipList(const std::vector<ScheduledShip>& ships) {
  ImGui::Text("SCHEDULED SHIPS: %zu", ships.size());

  if (ImGui::BeginTable("ShipSchedule", 5,
                        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                        ImVec2(0, 150))) {
    ImGui::TableSetupColumn("Name");
    ImGui::TableSetupColumn("Cargo");
    ImGui::TableSetupColumn("Arrival day");
    ImGui::TableSetupColumn("Weight (t)");
    ImGui::TableSetupColumn("Stay (days)");
    ImGui::TableHeadersRow();

    for (const auto& item : ships) {
      const auto& ship = item.spec;
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(ship.name.c_str());
      ImGui::TableNextColumn();
      ImGui::TextUnformatted(sea_port::ToString(ship.cargo_type).c_str());
      ImGui::TableNextColumn();
      ImGui::Text("%d", ship.planned_arrival_day);
      ImGui::TableNextColumn();
      ImGui::Text("%.0f", ship.cargo_weight_tons);
      ImGui::TableNextColumn();
      ImGui::Text("%d", ship.planned_stay_days);
    }

    ImGui::EndTable();
  }
}

// Tests crane configurations using the same scheduled ships and random seed.
// A configuration is acceptable when it meets the backend's rating and service-rate targets.
// Among acceptable configurations, the one with the lowest purchase cost is selected.
bool FindCraneConfiguration(const sea_port::SimulationConfig& baseConfig,
                            const std::vector<ScheduledShip>& ships,
                            std::array<int, sea_port::kCargoTypeCount>& bestCounts,
                            double& bestPenalty, double& bestRating, double& bestServiceRate) {
  bool found = false;
  double bestCost = 0.0;
  bestPenalty = 0.0;
  bestRating = 0.0;
  bestServiceRate = 0.0;

  // Check 0..5 cranes for each cargo type.
  for (int bulk = 0; bulk <= 5; ++bulk) {
    for (int liquid = 0; liquid <= 5; ++liquid) {
      for (int containers = 0; containers <= 5; ++containers) {
        sea_port::SimulationConfig config = baseConfig;
        config.crane_counts = {bulk, liquid, containers};

        sea_port::PortSimulation test(config);

        for (const auto& item : ships) {
          test.ScheduleShip(item.spec);
        }

        for (int day = 1; day <= config.simulation_days; ++day) {
          test.GenerateRandomParametersForDay(day, 7);
          test.SimulateDay();
        }

        test.Finalize();
        const auto& stats = test.statistics();

        const bool meetsTargets =
            test.rating() >= config.minimum_rating_for_optimization &&
            stats.service_rate() >= config.minimum_service_rate_for_optimization;

        if (!meetsTargets) {
          continue;
        }

        const double cost = test.CranePurchaseCost();
        if (!found || cost < bestCost ||
            (cost == bestCost && stats.total_penalties() < bestPenalty)) {
          found = true;
          bestCost = cost;
          bestCounts = {bulk, liquid, containers};
          bestPenalty = stats.total_penalties();
          bestRating = test.rating();
          bestServiceRate = stats.service_rate();
        }
      }
    }
  }

  return found;
}

}  // namespace

int main() {
  if (!glfwInit()) {
    return 1;
  }

  GLFWwindow* window = glfwCreateWindow(1400, 900, "Sea Port", nullptr, nullptr);

  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 12.0f;
  style.ChildRounding = 12.0f;
  style.FrameRounding = 8.0f;
  style.WindowPadding = ImVec2(20, 20);
  style.FramePadding = ImVec2(12, 8);
  style.ItemSpacing = ImVec2(12, 12);

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 130");

  bool simulationStarted = false;
  bool simulationFinished = false;
  bool isRunning = false;
  bool hasReport = false;
  bool optimizerChecked = false;
  bool optimizerFound = false;

  std::unique_ptr<sea_port::PortSimulation> simulation;
  sea_port::DailyReport lastReport;
  std::vector<sea_port::DailyReport> reportHistory;
  std::vector<ScheduledShip> scheduledShips;

  double lastStepTime = 0.0;

  int cranesType1 = 2;
  int cranesType2 = 3;
  int cranesType3 = 1;
  int arrivalMin = -2;
  int arrivalMax = 2;
  int unloadingMin = 0;
  int unloadingMax = 3;
  int simulationStep = 1;

  char shipName[64] = "Ship-001";
  int cargoTypeIndex = 0;
  int plannedArrivalDay = 1;
  float cargoWeight = 1000.0f;
  int plannedStayDays = 3;
  int nextShipNumber = 2;

  std::array<int, sea_port::kCargoTypeCount> optimizedCounts = {0, 0, 0};
  double optimizedPenalty = 0.0;
  double optimizedRating = 0.0;
  double optimizedServiceRate = 0.0;
  std::string optimizerMessage;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    bool advanceRequested = false;
    bool finishRequested = false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("Sea Port", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("SEA PORT");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Separator();
    ImGui::Spacing();

    if (!simulationStarted) {
      ImGui::Text("SIMULATION SETTINGS");
      ImGui::BeginChild("Settings", ImVec2(0, 245), true);

      ImGui::Text("NUMBER OF CRANES");
      ImGui::InputInt("Bulk cargo cranes", &cranesType1);
      ImGui::InputInt("Liquid cargo cranes", &cranesType2);
      ImGui::InputInt("Container cranes", &cranesType3);

      ImGui::Separator();
      ImGui::Text("ARRIVAL DEVIATION (DAYS)");
      ImGui::InputInt("Minimum##arrival", &arrivalMin);
      ImGui::InputInt("Maximum##arrival", &arrivalMax);

      ImGui::SameLine();
      ImGui::Text(" ");
      ImGui::Text("UNLOADING DELAY (DAYS)");
      ImGui::InputInt("Minimum##unloading", &unloadingMin);
      ImGui::InputInt("Maximum##unloading", &unloadingMax);
      ImGui::SliderInt("Simulation step (days)", &simulationStep, 1, 3);

      ImGui::EndChild();

      ImGui::Spacing();
      ImGui::Text("SHIP ARRIVAL SCHEDULE");
      ImGui::BeginChild("ScheduleInput", ImVec2(0, 220), true);

      ImGui::InputText("Ship name", shipName, sizeof(shipName));
      const char* cargoNames[] = {"Bulk", "Liquid", "Container"};
      ImGui::Combo("Cargo type", &cargoTypeIndex, cargoNames, 3);
      ImGui::InputInt("Planned arrival day", &plannedArrivalDay);
      ImGui::InputFloat("Cargo weight (tons)", &cargoWeight, 100.0f, 500.0f);
      ImGui::InputInt("Planned stay (days)", &plannedStayDays);

      if (ImGui::Button("ADD SHIP", ImVec2(150, 38))) {
        if (shipName[0] == '\0') {
          std::snprintf(shipName, sizeof(shipName), "Ship-%03d", nextShipNumber++);
        }

        sea_port::ShipSpec spec;
        spec.name = shipName;
        spec.cargo_type = static_cast<sea_port::CargoType>(std::clamp(cargoTypeIndex, 0, 2));
        spec.booking_day = std::max(1, plannedArrivalDay);
        spec.planned_arrival_day = std::max(1, plannedArrivalDay);
        spec.cargo_weight_tons = std::max(1.0f, cargoWeight);
        spec.planned_stay_days = std::max(1, plannedStayDays);

        scheduledShips.push_back({spec});

        std::snprintf(shipName, sizeof(shipName), "Ship-%03d", nextShipNumber++);
        plannedArrivalDay = std::min(30, plannedArrivalDay + 1);
      }

      ImGui::SameLine();
      if (ImGui::Button("REMOVE LAST", ImVec2(150, 38)) && !scheduledShips.empty()) {
        scheduledShips.pop_back();
      }

      DrawShipList(scheduledShips);
      ImGui::EndChild();

      ImGui::Spacing();

      if (ImGui::Button("FIND CRANE CONFIGURATION", ImVec2(280, 42))) {
        sea_port::SimulationConfig config;
        config.crane_counts = {std::max(0, cranesType1), std::max(0, cranesType2),
                               std::max(0, cranesType3)};
        config.arrival_deviation_min_days = std::min(arrivalMin, arrivalMax);
        config.arrival_deviation_max_days = std::max(arrivalMin, arrivalMax);
        config.unloading_delay_min_days = std::min(unloadingMin, unloadingMax);
        config.unloading_delay_max_days = std::max(unloadingMin, unloadingMax);
        config.step_days = std::clamp(simulationStep, 1, 3);

        optimizerChecked = true;
        optimizerFound =
            FindCraneConfiguration(config, scheduledShips, optimizedCounts, optimizedPenalty,
                                   optimizedRating, optimizedServiceRate);

        if (optimizerFound) {
          optimizerMessage = "Acceptable configuration found.";
        } else {
          optimizerMessage = "No configuration met the rating and service-rate targets.";
        }
      }

      if (optimizerChecked) {
        ImGui::TextWrapped("%s", optimizerMessage.c_str());
        if (optimizerFound) {
          ImGui::Text("Suggested cranes (bulk / liquid / container): %d / %d / %d",
                      optimizedCounts[0], optimizedCounts[1], optimizedCounts[2]);
          ImGui::Text("Penalty: %.2f | Rating: %.1f | Service rate: %.2f", optimizedPenalty,
                      optimizedRating, optimizedServiceRate);
          if (ImGui::Button("USE SUGGESTED CONFIGURATION")) {
            cranesType1 = optimizedCounts[0];
            cranesType2 = optimizedCounts[1];
            cranesType3 = optimizedCounts[2];
          }
        }
      }

      ImGui::Spacing();
      if (ImGui::Button("START SIMULATION", ImVec2(250, 48))) {
        cranesType1 = std::max(0, cranesType1);
        cranesType2 = std::max(0, cranesType2);
        cranesType3 = std::max(0, cranesType3);
        if (arrivalMin > arrivalMax) std::swap(arrivalMin, arrivalMax);
        if (unloadingMin > unloadingMax) std::swap(unloadingMin, unloadingMax);

        sea_port::SimulationConfig config;
        config.crane_counts = {cranesType1, cranesType2, cranesType3};
        config.arrival_deviation_min_days = arrivalMin;
        config.arrival_deviation_max_days = arrivalMax;
        config.unloading_delay_min_days = unloadingMin;
        config.unloading_delay_max_days = unloadingMax;
        config.step_days = simulationStep;

        simulation = std::make_unique<sea_port::PortSimulation>(config);
        for (const auto& item : scheduledShips) {
          simulation->ScheduleShip(item.spec);
        }

        reportHistory.clear();
        simulationStarted = true;
        simulationFinished = false;
        isRunning = false;
        hasReport = false;
        lastStepTime = glfwGetTime();
      }
    }

    if (simulationStarted && simulation) {
      ImGui::Text("SETTINGS");
      ImGui::BeginChild("SmallSettings", ImVec2(0, 105), true);
      ImGui::Text("Cranes (bulk / liquid / container): %d / %d / %d", cranesType1, cranesType2,
                  cranesType3);
      ImGui::Text("Arrival deviation: %d ... %d days", arrivalMin, arrivalMax);
      ImGui::Text("Unloading delay: %d ... %d days", unloadingMin, unloadingMax);
      ImGui::Text("Step: %d day(s)", simulationStep);
      ImGui::Text("Scheduled ships: %zu", scheduledShips.size());
      ImGui::EndChild();

      ImGui::Spacing();
      ImGui::Text("SIMULATION");
      ImGui::BeginChild("Simulation", ImVec2(0, 0), true);

      ImGui::Text("DAY %d / %d",
                  std::min(simulation->current_day() - 1, simulation->config().simulation_days),
                  simulation->config().simulation_days);
      ImGui::Separator();

      if (hasReport) {
        ImGui::Text("DAILY REPORT");
        ImGui::Text("Arrived: %d | Started unloading: %d | Completed: %d", lastReport.arrived,
                    lastReport.started_unloading, lastReport.completed);
        ImGui::Text("Waiting: %zu | Unloading: %zu", lastReport.waiting, lastReport.unloading);
        ImGui::Text("Rating: %.1f | Daily net revenue: %.2f", lastReport.rating,
                    lastReport.daily_net_revenue);

        ImGui::Separator();
        ImGui::Text("QUEUES BY CARGO TYPE");
        for (std::size_t i = 0; i < sea_port::kCargoTypeCount; ++i) {
          ImGui::Text("%s:", i == 0 ? "Bulk" : (i == 1 ? "Liquid" : "Container"));
          for (const auto& ship : lastReport.queues[i]) {
            ImGui::BulletText("%s", ship.c_str());
          }
        }

        ImGui::Text("ACTIVE OPERATIONS");
        for (const auto& operation : lastReport.active_operations) {
          ImGui::BulletText("%s", operation.c_str());
        }
        ImGui::Text("EVENTS");
        for (const auto& event : lastReport.events) {
          ImGui::BulletText("%s", event.c_str());
        }
      } else {
        ImGui::Text("Press STEP or RUN to start.");
      }

      ImGui::Separator();
      ImGui::Text("RECENT DAYS");
      int firstDay = std::max(0, static_cast<int>(reportHistory.size()) - 5);
      for (int i = firstDay; i < static_cast<int>(reportHistory.size()); ++i) {
        ImGui::PushID(i);
        if (ImGui::TreeNode("Day", "DAY %d", reportHistory[i].day)) {
          const auto& report = reportHistory[i];
          ImGui::Text("Arrived: %d | Completed: %d | Waiting: %zu", report.arrived,
                      report.completed, report.waiting);
          for (const auto& event : report.events) {
            ImGui::BulletText("%s", event.c_str());
          }
          ImGui::TreePop();
        }
        ImGui::PopID();
      }

      ImGui::Separator();
      ImGui::Text("COMPLETED UNLOADINGS");
      const auto& records = simulation->statistics().completions();
      if (ImGui::BeginTable(
              "Unloadings", 6,
              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
              ImVec2(0, 180))) {
        ImGui::TableSetupColumn("Ship");
        ImGui::TableSetupColumn("Cargo");
        ImGui::TableSetupColumn("Arrival");
        ImGui::TableSetupColumn("Waiting");
        ImGui::TableSetupColumn("Start");
        ImGui::TableSetupColumn("Duration");
        ImGui::TableHeadersRow();

        for (const auto& record : records) {
          ImGui::TableNextRow();
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(record.ship_name.c_str());
          ImGui::TableNextColumn();
          ImGui::TextUnformatted(sea_port::ToString(record.cargo_type).c_str());
          ImGui::TableNextColumn();
          ImGui::Text("%d", record.actual_arrival_day);
          ImGui::TableNextColumn();
          ImGui::Text("%d days", record.waiting_days);
          ImGui::TableNextColumn();
          ImGui::Text("%d", record.unloading_start_day);
          ImGui::TableNextColumn();
          ImGui::Text("%d days", record.unloading_duration_days);
        }
        ImGui::EndTable();
      }

      ImGui::Separator();
      if (!simulationFinished) {
        if (ImGui::Button("STEP", ImVec2(100, 38))) {
          advanceRequested = true;
          isRunning = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("RUN", ImVec2(100, 38))) {
          isRunning = true;
          lastStepTime = glfwGetTime();
        }
        ImGui::SameLine();
        if (ImGui::Button("STOP", ImVec2(100, 38))) {
          isRunning = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("FINISH SIMULATION", ImVec2(200, 38))) {
          finishRequested = true;
          isRunning = false;
        }
      } else {
        ImGui::Text("SIMULATION FINISHED");
      }

      ImGui::EndChild();
    }

    if (simulationFinished && simulation) {
      ImGui::Spacing();
      ImGui::Text("FINAL STATISTICS");
      ImGui::BeginChild("Statistics", ImVec2(0, 230), true);
      const auto& stats = simulation->statistics();

      ImGui::Text("Arrived ships: %d", stats.arrived_ships());
      ImGui::Text("Completed ships: %d", stats.completed_ships());
      ImGui::Text("Abandoned ships: %d", stats.abandoned_ships());
      ImGui::Text("Rejected ships: %d", stats.rejected_ships());
      ImGui::Text("Unfinished ships: %zu", stats.unfinished_ships());
      ImGui::Separator();
      ImGui::Text("Average queue length: %.2f", stats.average_queue_length());
      ImGui::Text("Maximum queue length: %zu", stats.maximum_queue_length());
      ImGui::Text("Average waiting time: %.2f days", stats.average_waiting_days());
      ImGui::Text("Maximum waiting time: %d days", stats.maximum_waiting_days());
      ImGui::Text("Average unloading delay: %.2f days", stats.average_unloading_delay_days());
      ImGui::Text("Maximum unloading delay: %d days", stats.maximum_unloading_delay_days());
      ImGui::Text("Total penalties: %.2f", stats.total_penalties());
      ImGui::Text("Net revenue: %.2f", stats.net_revenue());
      ImGui::Text("Service rate: %.2f", stats.service_rate());
      ImGui::Text("Final rating: %.1f", simulation->rating());
      ImGui::EndChild();
    }

    ImGui::End();

    if (simulation && !simulationFinished) {
      int daysToRun = 0;
      if (finishRequested) {
        daysToRun = simulation->config().simulation_days - simulation->current_day() + 1;
      } else if (advanceRequested) {
        daysToRun = simulationStep;
      } else if (isRunning && glfwGetTime() - lastStepTime >= 0.5) {
        daysToRun = simulationStep;
        lastStepTime = glfwGetTime();
      }

      for (int i = 0; i < daysToRun; ++i) {
        if (simulation->current_day() > simulation->config().simulation_days) {
          break;
        }
        simulation->GenerateRandomParametersForDay(simulation->current_day(), 7);
        lastReport = simulation->SimulateDay();
        reportHistory.push_back(lastReport);
        hasReport = true;
      }

      if (simulation->current_day() > simulation->config().simulation_days || finishRequested) {
        simulation->Finalize();
        simulationFinished = true;
        isRunning = false;
      }
    }

    ImGui::Render();
    int displayW = 0;
    int displayH = 0;
    glfwGetFramebufferSize(window, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
