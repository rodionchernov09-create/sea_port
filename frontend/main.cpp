#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>

#include "imgui.h"

static void DrawCard(const char *title,
                     const char *value,
                     const char *description) {
    ImGui::BeginChild(
        title,
        ImVec2(0, 150),
        true
    );

    ImGui::Text("%s", title);
    ImGui::Spacing();

    ImGui::SetWindowFontScale(1.6f);
    ImGui::Text("%s", value);
    ImGui::SetWindowFontScale(1.0f);

    ImGui::Spacing();
    ImGui::TextDisabled("%s", description);

    ImGui::EndChild();
}

int main() {
    // GLFW
    if (!glfwInit())
        return -1;

    GLFWwindow *window = glfwCreateWindow(
        1400,
        900,
        "Sea Port",
        nullptr,
        nullptr
    );

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Dear ImGui
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Стиль
    ImGui::StyleColorsDark();

    ImGuiStyle &style = ImGui::GetStyle();

    style.WindowRounding = 12.0f;
    style.ChildRounding = 12.0f;
    style.FrameRounding = 8.0f;

    style.WindowPadding = ImVec2(20, 20);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(12, 12);

    // Backend ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Главный цикл
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // =====================================================
        // ГЛАВНОЕ ОКНО
        // =====================================================

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(
            ImGui::GetIO().DisplaySize
        );

        ImGuiWindowFlags flags =
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove;

        ImGui::Begin("MainWindow", nullptr, flags);

        // -----------------------------------------------------
        // HEADER
        // -----------------------------------------------------

        ImGui::Text("SEA PORT");

        ImGui::SameLine();

        float rightPosition =
                ImGui::GetWindowWidth() - 180;

        ImGui::SetCursorPosX(rightPosition);

        ImGui::Text("DAY 0");

        ImGui::Separator();

        ImGui::Spacing();

        // -----------------------------------------------------
        // ВЕРХНИЕ КАРТОЧКИ
        // -----------------------------------------------------

        float width = ImGui::GetContentRegionAvail().x;

        float cardWidth = (width - 20) / 2;

        
        ImGui::End();

        // =====================================================
        // RENDER
        // =====================================================

        ImGui::Render();

        int displayW;
        int displayH;

        glfwGetFramebufferSize(
            window,
            &displayW,
            &displayH
        );

        glViewport(
            0,
            0,
            displayW,
            displayH
        );

        glClearColor(
            0.05f,
            0.06f,
            0.08f,
            1.0f
        );

        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(
            ImGui::GetDrawData()
        );

        glfwSwapBuffers(window);
    }

    // Завершение
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}