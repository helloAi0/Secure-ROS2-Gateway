#ifndef SECURE_TELEMETRY_GATEWAY_STORAGE_STORAGE_ENGINE_HPP_
#define SECURE_TELEMETRY_GATEWAY_STORAGE_STORAGE_ENGINE_HPP_

#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include <sqlite3.h>
#include "secure_telemetry_gateway/models/telemetry_data.hpp"

namespace secure_telemetry_gateway {
namespace storage {

/**
 * @brief Thread-safe SQLite3 Database Storage Engine for edge telemetry persistence.
 */
class StorageEngine {
public:
  /**
   * @brief Constructs the storage engine targeting a database file path.
   * @param db_path System path to the SQLite3 database file.
   */
  explicit StorageEngine(std::string db_path = "data/telemetry_history.db");

  /**
   * @brief Destructor ensures safe finalization of prepared statements and DB handle.
   */
  ~StorageEngine();

  // Prevent copy and assignment
  StorageEngine(const StorageEngine&) = delete;
  StorageEngine& operator=(const StorageEngine&) = delete;

  /**
   * @brief Opens the SQLite connection, enables WAL mode, and creates tables/indexes.
   * @return true on successful initialization, false on error.
   */
  bool init();

  /**
   * @brief Inserts a single TelemetryData frame into the database using prepared statements.
   * @param data Telemetry frame to insert.
   * @return true on success, false on failure.
   */
  bool insert_telemetry(const models::TelemetryData& data);

  /**
   * @brief Inserts a batch of TelemetryData frames inside a single database transaction.
   * @param batch Vector of telemetry frames.
   * @return true if entire batch committed successfully, false otherwise.
   */
  bool insert_batch(const std::vector<models::TelemetryData>& batch);

  /**
   * @brief Queries recent telemetry records for a specific robot ID.
   * @param robot_id Target robot identifier string.
   * @param limit Maximum number of records to return.
   * @return std::vector<models::TelemetryData> Retrieved telemetry frames.
   */
  std::vector<models::TelemetryData> query_telemetry(const std::string& robot_id, uint64_t limit = 100);

  /**
   * @brief Purges telemetry records older than a nanosecond timestamp threshold.
   * @param timestamp_ns Cutoff nanosecond timestamp.
   * @return true on successful purge, false on failure.
   */
  bool purge_older_than(uint64_t timestamp_ns);

  /**
   * @brief Explicitly closes prepared statements and database connection.
   */
  void close();

private:
  bool enable_wal_mode();
  bool create_schema();

  std::string db_path_;
  sqlite3* db_{nullptr};
  sqlite3_stmt* insert_stmt_{nullptr};
  mutable std::mutex db_mutex_;
  bool is_initialized_{false};
};

} // namespace storage
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_STORAGE_STORAGE_ENGINE_HPP_