cmake_minimum_required(VERSION 3.16)
project(secure_telemetry_gateway LANGUAGES CXX)

# Force Modern C++ Standard (C++17 required, C++20 compatible)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for VS Code / clangd IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# 1. COMPILER WARNINGS & SECURITY HARDENING FLAGS
# ------------------------------------------------------------------------------
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Werror
    -Wconversion
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -fstack-protector-strong
    -fPIE
  )
endif()

# ------------------------------------------------------------------------------
# 2. DEPENDENCY DISCOVERY
# ------------------------------------------------------------------------------
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Optional system dependencies (spdlog, yaml-cpp, SQLite3)
find_package(spdlog QUIET)
find_package(yaml-cpp QUIET)
find_package(SQLite3 QUIET)

# ------------------------------------------------------------------------------
# 3. INCLUDE DIRECTORIES & TARGETS SETUP
# ------------------------------------------------------------------------------
# Public header include path
include_directories(
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# ------------------------------------------------------------------------------
# 4. TESTING CONFIGURATION
# ------------------------------------------------------------------------------
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  
  # Placeholder for future core tests registration
  # ament_add_gtest(test_gateway_core tests/test_main.cpp)
endif()

# ------------------------------------------------------------------------------
# 5. AMENT PACKAGE EXPORT & INSTALLATION RULES
# ------------------------------------------------------------------------------
install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

ament_package()