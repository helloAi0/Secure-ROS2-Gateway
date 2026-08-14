#include "secure_telemetry_gateway/storage/storage_engine.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"
#include <filesystem>
#include <thread>
#include <sstream>

namespace secure_telemetry_gateway {
namespace storage {

StorageEngine::StorageEngine(std::string db_path)
    : db_path_(std::move(db_path)) {}

StorageEngine::~StorageEngine() {
  close();
}

void StorageEngine::ensure_initialized() {
  std::call_once(init_flag_, [this]() {
    init_internal();
  });
}

bool StorageEngine::init() {
  ensure_initialized();
  return is_initialized_.load();
}

bool StorageEngine::init_internal() {
  std::lock_guard<std::mutex> lock(db_mutex_);

  if (is_initialized_) {
    return true;
  }

  init_failed_ = false;

  // Ensure parent directory exists before opening file
  std::filesystem::path path(db_path_);
  if (path.has_parent_path()) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
  }

  int rc = sqlite3_open(db_path_.c_str(), &db_);
  if (rc != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to open SQLite database at '{}': {}",
              db_path_, sqlite3_errmsg(db_));
    init_failed_ = true;
    close_internal();
    return false;
  }

  if (!enable_wal_mode() || !create_schema()) {
    init_failed_ = true;
    close_internal();
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
    init_failed_ = true;
    close_internal();
    return false;
  }

  is_initialized_ = true;
  LOG_INFO("StorageEngine: Database initialized successfully at '{}' (WAL mode active)", db_path_);
  return true;
}

bool StorageEngine::is_ready() const {
  return is_initialized_.load();
}

bool StorageEngine::insert_telemetry(const models::TelemetryData& data) {
  ensure_initialized();

  if (!is_initialized_.load()) {
    LOG_ERROR("StorageEngine: Cannot insert telemetry; database connection initialization failed.");
    return false;
  }

  std::lock_guard<std::mutex> lock(db_mutex_);

  sqlite3_reset(insert_stmt_);
  sqlite3_clear_bindings(insert_stmt_);

  // Convert the map to a JSON string manually
  std::string payload_json = "{";
  bool first = true;
  for (const auto& kv : data.payload_values) {
    if (!first) payload_json += ",";
    payload_json += "\"" + kv.first + "\":" + std::to_string(kv.second);
    first = false;
  }
  payload_json += "}";

  sqlite3_bind_text(insert_stmt_, 1, data.robot_id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(insert_stmt_, 2, data.sensor_type.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(insert_stmt_, 3, static_cast<sqlite3_int64>(data.timestamp_ns));
  sqlite3_bind_int64(insert_stmt_, 4, static_cast<sqlite3_int64>(data.sequence_id));
  
  // Use the newly created JSON string
  sqlite3_bind_text(insert_stmt_, 5, payload_json.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(insert_stmt_) != SQLITE_DONE) {
    LOG_ERROR("StorageEngine: Insert telemetry step failed: {}", sqlite3_errmsg(db_));
    return false;
  }

  return true;
}

bool StorageEngine::insert_batch(const std::vector<models::TelemetryData>& batch) {
  ensure_initialized();

  if (!is_initialized_.load()) {
    LOG_ERROR("StorageEngine: Cannot insert batch; database connection initialization failed.");
    return false;
  }

  if (batch.empty()) {
    return true;
  }

  std::lock_guard<std::mutex> lock(db_mutex_);

  char* err_msg = nullptr;
  if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
    LOG_ERROR("StorageEngine: Failed to begin transaction: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }

  bool success = true;
  for (const auto& data : batch) {
    sqlite3_reset(insert_stmt_);
    sqlite3_clear_bindings(insert_stmt_);

    // Convert the map to a JSON string manually for each item in the batch
    std::string payload_json = "{";
    bool first = true;
    for (const auto& kv : data.payload_values) {
      if (!first) payload_json += ",";
      payload_json += "\"" + kv.first + "\":" + std::to_string(kv.second);
      first = false;
    }
    payload_json += "}";

    sqlite3_bind_text(insert_stmt_, 1, data.robot_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(insert_stmt_, 2, data.sensor_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(insert_stmt_, 3, static_cast<sqlite3_int64>(data.timestamp_ns));
    sqlite3_bind_int64(insert_stmt_, 4, static_cast<sqlite3_int64>(data.sequence_id));
    
    // Use the newly created JSON string
    sqlite3_bind_text(insert_stmt_, 5, payload_json.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(insert_stmt_) != SQLITE_DONE) {
      LOG_ERROR("StorageEngine: Batch item insert failed: {}", sqlite3_errmsg(db_));
      success = false;
      break;
    }
  }

  if (success) {
    if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err_msg) != SQLITE_OK) {
      LOG_ERROR("StorageEngine: Failed to commit transaction: {}", err_msg ? err_msg : "unknown");
      sqlite3_free(err_msg);
      return false;
    }
  } else {
    sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
  }

  return success;
}

std::vector<models::TelemetryData> StorageEngine::query_telemetry(const std::string& /*robot_id*/, uint64_t /*limit*/) {
  ensure_initialized();

  if (!is_initialized_.load()) {
    LOG_ERROR("StorageEngine: Cannot query telemetry; database connection initialization failed.");
    return {};
  }

  std::lock_guard<std::mutex> lock(db_mutex_);
  return {};
}

bool StorageEngine::purge_older_than(uint64_t /*timestamp_ns*/) {
  ensure_initialized();

  if (!is_initialized_.load()) {
    LOG_ERROR("StorageEngine: Cannot purge data; database connection initialization failed.");
    return false;
  }

  std::lock_guard<std::mutex> lock(db_mutex_);
  return true;
}

void StorageEngine::close_internal() {
  if (insert_stmt_) {
    sqlite3_finalize(insert_stmt_);
    insert_stmt_ = nullptr;
  }

  if (db_) {
    sqlite3_close(db_);
    db_ = nullptr;
  }

  is_initialized_ = false;
}

void StorageEngine::close() {
  std::lock_guard<std::mutex> lock(db_mutex_);
  close_internal();
  init_failed_ = false;
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
    LOG_ERROR("StorageEngine: Failed to create schema: {}", err_msg ? err_msg : "unknown");
    sqlite3_free(err_msg);
    return false;
  }
  return true;
}

} // namespace storage
} // namespace secure_telemetry_gateway