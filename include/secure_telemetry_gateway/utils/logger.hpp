#ifndef SECURE_TELEMETRY_GATEWAY_UTILS_LOGGER_HPP_
#define SECURE_TELEMETRY_GATEWAY_UTILS_LOGGER_HPP_

#include <memory>
#include <string>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

namespace secure_telemetry_gateway {
namespace utils {

/**
 * @brief Thread-safe, multi-sink Logging Subsystem wrapping spdlog.
 */
class Logger {
public:
  /**
   * @brief Initializes the global logging engine.
   * @param logger_name Name identifier for the logger instance.
   * @param log_file_path System file path where rotating logs are saved.
   * @param max_file_size_mb Maximum size per log file in Megabytes.
   * @param max_files Maximum number of rotated log files to retain on disk.
   */
  static void init(
      const std::string& logger_name = "gateway",
      const std::string& log_file_path = "logs/gateway.log",
      size_t max_file_size_mb = 10,
      size_t max_files = 5);

  /**
   * @brief Retrieves the raw spdlog logger pointer.
   * @return std::shared_ptr<spdlog::logger> Shared pointer to active spdlog instance.
   */
  static std::shared_ptr<spdlog::logger>& get_logger();

  /**
   * @brief Flushes pending log messages to disk immediately.
   */
  static void flush();

private:
  static std::shared_ptr<spdlog::logger> logger_instance_;
};

} // namespace utils
} // namespace secure_telemetry_gateway

// ------------------------------------------------------------------------------
// CONVENIENCE LOGGING MACROS
// ------------------------------------------------------------------------------
#define LOG_TRACE(...)    ::secure_telemetry_gateway::utils::Logger::get_logger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::secure_telemetry_gateway::utils::Logger::get_logger()->debug(__VA_ARGS__)
#define LOG_INFO(...)     ::secure_telemetry_gateway::utils::Logger::get_logger()->info(__VA_ARGS__)
#define LOG_WARN(...)     ::secure_telemetry_gateway::utils::Logger::get_logger()->warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::secure_telemetry_gateway::utils::Logger::get_logger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::secure_telemetry_gateway::utils::Logger::get_logger()->critical(__VA_ARGS__)

#endif // SECURE_TELEMETRY_GATEWAY_UTILS_LOGGER_HPP_