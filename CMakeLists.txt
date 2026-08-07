cmake_minimum_required(VERSION 3.16)
project(secure_telemetry_gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# 1. COMPILER WARNINGS & HARDENING
# ------------------------------------------------------------------------------
if(MSVC)
  add_compile_options(/W4 /WX)
else()
  add_compile_options(
    -Wall -Wextra -Wpedantic -Werror
    -Wconversion -Wshadow -Wnon-virtual-dtor
    -Wnull-dereference -fstack-protector-strong -fPIE
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
find_package(spdlog REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

# ------------------------------------------------------------------------------
# 3. BUILD GATEWAY CORE LIBRARY
# ------------------------------------------------------------------------------
add_library(${PROJECT_NAME}_core
  src/utils/logger.cpp
  src/utils/config_manager.cpp
  src/ingestion/telemetry_ingestion_engine.cpp
  src/security/security_engine.cpp
  src/storage/storage_engine.cpp
)

target_include_directories(${PROJECT_NAME}_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}_core PUBLIC
  rclcpp::rclcpp
  spdlog::spdlog
  yaml-cpp
  nlohmann_json::nlohmann_json
  OpenSSL::Crypto
  SQLite::SQLite3
  Threads::Threads
)

# ------------------------------------------------------------------------------
# 4. INSTALLATION RULES
# ------------------------------------------------------------------------------
install(TARGETS ${PROJECT_NAME}_core
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config/
  DESTINATION share/${PROJECT_NAME}/config
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp spdlog yaml-cpp nlohmann_json OpenSSL SQLite3 Threads)

ament_package()