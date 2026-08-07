#include "secure_telemetry_gateway/storage/storage_engine.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>

namespace secure_telemetry_gateway {
namespace storage {

using json = nlohmann::json;

StorageEngine::StorageEngine(std::string db_path)
    : db_path_(std::move(db_path)) {}

StorageEngine::~StorageEngine() {
  close();
}

bool StorageEngine::init() {
  std::lock_guard<std::mutex> lock(db_mutex_);

  if (is_initialized_) {
    return true;
  }

  // Ensure parent directory exists before opening file
  std::filesystem::path path(db_path_);
  if (path.has_parent_path()) {
    std::filesystem::create_directories(path.parent_path());
  }

  int rc = sqlite3_open(db_path_.c_str(), &db_);
  if (rc != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to open SQLite database at '{}': {}",
              db_path_, sqlite3_errmsg(db_));
    close();
    return false;
  }

  if (!enable_wal_mode() || !create_schema()) {
    close();
    return false;
  }

  // Prepare insertion statement once for repeated reuse
  const char* insert_sql = 
      "INSERT INTO telemetry_records "
      "(robot_id, sensor_type, timestamp_ns, sequence_id, payload_json) "
      "VALUES (?, ?, ?, ?, ?);";

  rc = sqlite3_prepare_v2(db_, insert_sql, -1, &insert_stmt_, nullptr);
  if (rc != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to prepare insert statement: {}", sqlite3_errmsg(db_));
    close();
    return false;
  }

  is_initialized_ = true;
  LOG_INFO("StorageEngine: Database initialized successfully at '{}' (WAL mode active)", db_path_);
  return true;
}

bool StorageEngine::enable_wal_mode() {
  char* err_msg = nullptr;
  int rc = sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to enable WAL mode: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }

  rc = sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to set synchronous mode: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

bool StorageEngine::create_schema() {
  const char* schema_sql = 
      "CREATE TABLE IF NOT EXISTS telemetry_records ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "robot_id TEXT NOT NULL,"
      "sensor_type TEXT NOT NULL,"
      "timestamp_ns INTEGER NOT NULL,"
      "sequence_id INTEGER NOT NULL,"
      "payload_json TEXT NOT NULL,"
      "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
      ");"
      "CREATE INDEX IF NOT EXISTS idx_robot_timestamp "
      "ON telemetry_records(robot_id, timestamp_ns);";

  char* err_msg = nullptr;
  int rc = sqlite3_exec(db_, schema_sql, nullptr, nullptr, &err_msg);
  if (rc != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to create database schema: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }

  return true;
}

bool StorageEngine::insert_telemetry(const models::TelemetryData& data) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  if (!is_initialized_) {
    LOG_ERROR("StorageEngine: Cannot insert, database not initialized");
    return false;
  }

  sqlite3_reset(insert_stmt_);
  sqlite3_clear_bindings(insert_stmt_);

  // Serialize payload map to JSON string
  json payload_json = data.payload_values;
  std::string payload_str = payload_json.dump();

  // Bind values to prepared statement parameters
  sqlite3_bind_text(insert_stmt_, 1, data.robot_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insert_stmt_, 2, data.sensor_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(insert_stmt_, 3, static_cast<sqlite3_int64>(data.timestamp_ns));
  sqlite3_bind_int64(insert_stmt_, 4, static_cast<sqlite3_int64>(data.sequence_id));
  sqlite3_bind_text(insert_stmt_, 5, payload_str.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(insert_stmt_) != SQLITE_DONE) {
    LOG_ERROR("StorageEngine: Failed to execute insert statement: {}", sqlite3_errmsg(db_));
    return false;
  }

  return true;
}

bool StorageEngine::insert_batch(const std::vector<models::TelemetryData>& batch) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  if (!is_initialized_ || batch.empty()) {
    return false;
  }

  char* err_msg = nullptr;
  if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to begin transaction: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }

  for (const auto& data : batch) {
    sqlite3_reset(insert_stmt_);
    sqlite3_clear_bindings(insert_stmt_);

    json payload_json = data.payload_values;
    std::string payload_str = payload_json.dump();

    sqlite3_bind_text(insert_stmt_, 1, data.robot_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insert_stmt_, 2, data.sensor_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(insert_stmt_, 3, static_cast<sqlite3_int64>(data.timestamp_ns));
    sqlite3_bind_int64(insert_stmt_, 4, static_cast<sqlite3_int64>(data.sequence_id));
    sqlite3_bind_text(insert_stmt_, 5, payload_str.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(insert_stmt_) != SQLITE_DONE) {
      LOG_ERROR("StorageEngine: Batch insertion error: {}. Rolling back.", sqlite3_errmsg(db_));
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
      return false;
    }
  }

  if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to commit transaction: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }

  LOG_TRACE("StorageEngine: Successfully inserted batch of {} records", batch.size());
  return true;
}

std::vector<models::TelemetryData> StorageEngine::query_telemetry(
    const std::string& robot_id, uint64_t limit) {

  std::lock_guard<std::mutex> lock(db_mutex_);
  std::vector<models::TelemetryData> results;

  if (!is_initialized_) {
    return results;
  }

  const char* query_sql = 
      "SELECT robot_id, sensor_type, timestamp_ns, sequence_id, payload_json "
      "FROM telemetry_records WHERE robot_id = ? "
      "ORDER BY timestamp_ns DESC LIMIT ?;";

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db_, query_sql, -1, &stmt, nullptr) != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to prepare query statement: {}", sqlite3_errmsg(db_));
    return results;
  }

  sqlite3_bind_text(stmt, 1, robot_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt, 2, static_cast<sqlite3_int64>(limit));

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    models::TelemetryData data;
    data.robot_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    data.sensor_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    data.timestamp_ns = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));
    data.sequence_id = static_cast<uint64_t>(sqlite3_column_int64(stmt, 3));

    const char* payload_raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    if (payload_raw) {
      try {
        json parsed = json::parse(payload_raw);
        for (auto& [k, v] : parsed.items()) {
          if (v.is_number()) {
            data.payload_values[k] = v.get<double>();
          }
        }
      } catch (const json::exception&) {
        // Skip corrupted payload map entries
      }
    }

    results.push_back(std::move(data));
  }

  sqlite3_finalize(stmt);
  return results;
}

bool StorageEngine::purge_older_than(uint64_t timestamp_ns) {
  std::lock_guard<std::mutex> lock(db_mutex_);

  if (!is_initialized_) {
    return false;
  }

  const char* purge_sql = "DELETE FROM telemetry_records WHERE timestamp_ns < ?;";
  sqlite3_stmt* stmt = nullptr;

  if (sqlite3_prepare_v2(db_, purge_sql, -1, &stmt, nullptr) != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to prepare purge statement: {}", sqlite3_errmsg(db_));
    return false;
  }

  sqlite3_bind_int64(stmt, 1, static_cast<sqlite3_int64>(timestamp_ns));

  bool success = (sqlite3_step(stmt) == SQLITE_DONE);
  if (success) {
    int deleted_rows = sqlite3_changes(db_);
    LOG_INFO("StorageEngine: Purged {} historical records older than timestamp_ns {}",
             deleted_rows, timestamp_ns);
  } else {
    LOG_ERROR("StorageEngine: Purge operation failed: {}", sqlite3_errmsg(db_));
  }

  sqlite3_finalize(stmt);
  return success;
}

void StorageEngine::close() {
  std::lock_guard<std::mutex> lock(db_mutex_);

  if (insert_stmt_) {
    sqlite3_finalize(insert_stmt_);
    insert_stmt_ = nullptr;
  }

  if (db_) {
    sqlite3_close_v2(db_);
    db_ = nullptr;
  }

  is_initialized_ = false;
}

} // namespace storage
} // namespace secure_telemetry_gateway