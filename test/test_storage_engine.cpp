#include <gtest/gtest.h>
#include "secure_telemetry_gateway/storage/storage_engine.hpp"

using namespace secure_telemetry_gateway::storage;
using namespace secure_telemetry_gateway::models;

class StorageEngineTest : public ::testing::Test {
protected:
  void SetUp() override {
    db_path = ":memory:"; // In-memory database for isolated unit tests
    
    // Pass db_path to the constructor, then call init()
    storage = std::make_unique<StorageEngine>(db_path);
    ASSERT_TRUE(storage->init()) << "Failed to initialize SQLite database.";
  }

  void TearDown() override {
    if (storage) {
      storage->close();
    }
  }

  std::string db_path;
  std::unique_ptr<StorageEngine> storage;
};

TEST_F(StorageEngineTest, InsertAndQueryTelemetry) {
  TelemetryData data;
  data.robot_id = "amr_test_node";
  data.sensor_type = "battery";
  data.timestamp_ns = 1700000000000000000ULL;
  data.sequence_id = 42;
  data.payload_values["voltage"] = 24.5;

  bool insert_ok = storage->insert_telemetry(data);
  EXPECT_TRUE(insert_ok);

  auto results = storage->query_telemetry("amr_test_node", 10);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].robot_id, "amr_test_node");
  EXPECT_EQ(results[0].sequence_id, 42u);
}