cmake_minimum_required(VERSION 3.20)

project(sea_port_simulation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

target_include_directories(sea_port_lib PUBLIC include)
target_compile_options(sea_port_lib PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(sea_port src/main.cpp)
target_link_libraries(sea_port PRIVATE sea_port_lib)
