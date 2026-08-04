#include "secure_telemetry_gateway/utils/logger.hpp"
#include <vector>
#include <iostream>

namespace secure_telemetry_gateway {
namespace utils {

// Initialize static member variable
std::shared_ptr<spdlog::logger> Logger::logger_instance_ = nullptr;

void Logger::init(
    const std::string& logger_name,
    const std::string& log_file_path,
    size_t max_file_size_mb,
    size_t max_files) {
  
  try {
    // Sink 1: Colored Console Sink (Stdout)
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);

    // Sink 2: Rotating File Sink (Disk Protection)
    // Convert Megabytes to Bytes
    const size_t max_bytes = max_file_size_mb * 1024 * 1024;
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file_path, max_bytes, max_files);
    file_sink->set_level(spdlog::level::trace);

    // Combine both sinks into a multi-sink logger
    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    
    logger_instance_ = std::make_shared<spdlog::logger>(
        logger_name, sinks.begin(), sinks.end());

    // Configure log message format pattern:
    // [YYYY-MM-DD HH:MM:SS.ms] [logger_name] [log_level] [thread_id] message
    logger_instance_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
    logger_instance_->set_level(spdlog::level::trace);

    // Automatically flush logs to disk when an ERROR or CRITICAL log occurs
    logger_instance_->flush_on(spdlog::level::err);

    // Register instance globally in spdlog manager
    spdlog::register_logger(logger_instance_);

  } catch (const spdlog::spdlog_ex& ex) {
    std::cerr << "[CRITICAL] Logger initialization failed: " << ex.what() << std::endl;
  }
}

std::shared_ptr<spdlog::logger>& Logger::get_logger() {
  if (!logger_instance_) {
    // Fallback default initialization if get_logger() is called before init()
    init();
  }
  return logger_instance_;
}

void Logger::flush() {
  if (logger_instance_) {
    logger_instance_->flush();
  }
}

} // namespace utils
} // namespace secure_telemetry_gateway