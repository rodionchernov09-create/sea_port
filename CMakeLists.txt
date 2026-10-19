cmake_minimum_required(VERSION 3.20)

project(sea_port_simulation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================
# BACKEND
# =========================

add_library(sea_port_lib
        src/cargo.cpp
        src/crane.cpp
        src/crane_optimizer.cpp
        src/port_simulation.cpp
        src/random_generator.cpp
        src/sea_port.cpp
        src/ship.cpp
        src/statistics.cpp
        src/types.cpp
        src/weather.cpp
)

target_include_directories(sea_port_lib PUBLIC
        include
)

target_compile_options(sea_port_lib PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
)

add_executable(sea_port
        src/main.cpp
)

target_link_libraries(sea_port PRIVATE
        sea_port_lib
)


option(SEA_PORT_BUILD_GUI "Build the ImGui desktop application" ON)
if(SEA_PORT_BUILD_GUI)
find_package(OpenGL REQUIRED)
find_package(glfw3 QUIET)
if(NOT TARGET glfw)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  add_subdirectory(external/glfw)
endif()

add_executable(sea_port_gui
        frontend/main.cpp
        frontend/port_view.cpp

        external/imgui/imgui.cpp
        external/imgui/imgui_draw.cpp
        external/imgui/imgui_tables.cpp
        external/imgui/imgui_widgets.cpp

        external/imgui/backends/imgui_impl_glfw.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(sea_port_gui PRIVATE
        external/imgui
        external/imgui/backends
)

target_link_libraries(sea_port_gui PRIVATE
        glfw
        OpenGL::GL
        sea_port_lib
)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(integration_test tests/integration_test.cpp)
  target_link_libraries(integration_test PRIVATE sea_port_lib)
  add_test(NAME backend_integration COMMAND integration_test)
endif()
