#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main() {
  // Create window
  if (!glfwInit()) return 1;

  GLFWwindow* window = glfwCreateWindow(1400, 900, "Sea Port", nullptr, nullptr);

  if (!window) return 1;

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Create ImGui
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

  // Simulation state
  bool simulationStarted = false;
  bool simulationFinished = false;

  // Settings
  int cranesType1 = 2;
  int cranesType2 = 3;
  int cranesType3 = 1;

  int arrivalMin = -2;
  int arrivalMax = 2;

  int unloadingMin = 0;
  int unloadingMax = 3;

  int simulationStep = 1;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main window
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::Begin("Sea Port", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    ImGui::SetWindowFontScale(1.6f);
    ImGui::Text("SEA PORT");
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Separator();
    ImGui::Spacing();

    // Initial settings
    if (!simulationStarted) {
      ImGui::Text("SETTINGS");
      ImGui::Spacing();

      // Large settings window
      ImGui::BeginChild("Settings", ImVec2(0, 0), true);

      ImGui::Text("Number of cranes");
      ImGui::InputInt("Crane type 1", &cranesType1);
      ImGui::InputInt("Crane type 2", &cranesType2);
      ImGui::InputInt("Crane type 3", &cranesType3);

      ImGui::Spacing();

      ImGui::Text("Arrival deviation");
      ImGui::InputInt("Minimum##arrival", &arrivalMin);
      ImGui::InputInt("Maximum##arrival", &arrivalMax);

      ImGui::Spacing();

      ImGui::Text("Unloading delay");
      ImGui::InputInt("Minimum##unloading", &unloadingMin);
      ImGui::InputInt("Maximum##unloading", &unloadingMax);

      ImGui::Spacing();

      ImGui::Text("Simulation step");
      ImGui::InputInt("Days", &simulationStep);

      if (simulationStep < 1) simulationStep = 1;

      ImGui::Spacing();
      ImGui::Spacing();

      if (ImGui::Button("START SIMULATION", ImVec2(250, 50))) {
        simulationStarted = true;
      }

      ImGui::EndChild();
    }

    // Simulation
    if (simulationStarted) {
      // Small settings
      ImGui::Text("SETTINGS");

      ImGui::BeginChild("SmallSettings", ImVec2(0, 120), true);

      ImGui::Text("Cranes: %d / %d / %d", cranesType1, cranesType2, cranesType3);

      ImGui::Text("Arrival deviation: %d ... %d", arrivalMin, arrivalMax);

      ImGui::Text("Unloading delay: %d ... %d", unloadingMin, unloadingMax);

      ImGui::Text("Step: %d day(s)", simulationStep);

      ImGui::EndChild();

      ImGui::Spacing();

      ImGui::Text("SIMULATION");

      // Simulation window
      ImGui::BeginChild("Simulation", ImVec2(0, 0), true);

      ImGui::Text("QUEUES");
      ImGui::Separator();

      ImGui::Spacing();
      ImGui::Spacing();

      ImGui::Text("SHIPS");
      ImGui::Separator();

      ImGui::Text("Arrivals:");

      ImGui::Spacing();
      ImGui::Spacing();

      ImGui::Text("COMPLETED UNLOADINGS");
      ImGui::Separator();

      ImGui::Columns(5, "Unloadings");

      ImGui::Text("Ship");
      ImGui::NextColumn();

      ImGui::Text("Arrival");
      ImGui::NextColumn();

      ImGui::Text("Waiting");
      ImGui::NextColumn();

      ImGui::Text("Start");
      ImGui::NextColumn();

      ImGui::Text("Duration");
      ImGui::NextColumn();

      ImGui::Separator();

      ImGui::Columns(1);

      ImGui::Spacing();
      ImGui::Spacing();

      if (ImGui::Button("STEP", ImVec2(100, 40))) {
        // One simulation step
      }

      ImGui::SameLine();

      if (ImGui::Button("RUN", ImVec2(100, 40))) {
        // Run simulation
      }

      ImGui::SameLine();

      if (ImGui::Button("STOP", ImVec2(100, 40))) {
        // Stop simulation
      }

      ImGui::SameLine();

      if (ImGui::Button("FINISH SIMULATION", ImVec2(200, 40))) {
        simulationFinished = true;
      }

      ImGui::EndChild();
    }

    // Statistics
    if (simulationFinished) {
      ImGui::Spacing();
      ImGui::Text("STATISTICS");

      ImGui::BeginChild("Statistics", ImVec2(0, 250), true);

      ImGui::Text("Unloaded ships: ");
      ImGui::Text("Average queue length: ");
      ImGui::Text("Average waiting time: ");
      ImGui::Text("Maximum unloading delay: ");
      ImGui::Text("Average unloading delay: ");
      ImGui::Text("Total penalty: ");

      ImGui::EndChild();
    }

    ImGui::End();

    // Render
    ImGui::Render();

    int displayW, displayH;
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