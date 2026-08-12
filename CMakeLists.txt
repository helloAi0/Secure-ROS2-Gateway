cmake_minimum_required(VERSION 3.8)
project(secure_telemetry_gateway)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(
  include
  ${OPENSSL_INCLUDE_DIR}
  ${SQLite3_INCLUDE_DIRS}
)

# Core Library
add_library(secure_telemetry_gateway_core SHARED
  src/security/security_engine.cpp
  src/ingestion/telemetry_ingestion_engine.cpp
  src/storage/storage_engine.cpp
  src/ros/gateway_node.cpp
  src/utils/logger.cpp
)

target_link_libraries(secure_telemetry_gateway_core
  OpenSSL::Crypto
  OpenSSL::SSL
  ${SQLite3_LIBRARIES}
  nlohmann_json::nlohmann_json
)

ament_target_dependencies(secure_telemetry_gateway_core
  rclcpp
  sensor_msgs
  geometry_msgs
)

# Gateway Node Executable
add_executable(secure_telemetry_gateway_node src/main.cpp)
target_link_libraries(secure_telemetry_gateway_node secure_telemetry_gateway_core)
ament_target_dependencies(secure_telemetry_gateway_node rclcpp)

# Mock Publisher Executable
add_executable(mock_telemetry_publisher src/mock_telemetry_publisher.cpp)
target_link_libraries(mock_telemetry_publisher secure_telemetry_gateway_core)
ament_target_dependencies(mock_telemetry_publisher rclcpp)

# Install Targets
install(TARGETS
  secure_telemetry_gateway_core
  secure_telemetry_gateway_node
  mock_telemetry_publisher
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

# Unit Testing Targets
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_security_engine test/test_security_engine.cpp)
  target_link_libraries(test_security_engine secure_telemetry_gateway_core)

  ament_add_gtest(test_storage_engine test/test_storage_engine.cpp)
  target_link_libraries(test_storage_engine secure_telemetry_gateway_core)
endif()

ament_package()