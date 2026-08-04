#ifndef SECURE_TELEMETRY_GATEWAY_UTILS_CONFIG_MANAGER_HPP_
#define SECURE_TELEMETRY_GATEWAY_UTILS_CONFIG_MANAGER_HPP_

#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <memory>
#include <yaml-cpp/yaml.h>

namespace secure_telemetry_gateway {
namespace utils {

/**
 * @brief Thread-safe Singleton Configuration Manager.
 * Loads YAML configurations and provides type-safe parameter retrieval via dot-notation.
 */
class ConfigManager {
public:
  /**
   * @brief Access the global Singleton instance.
   * Guaranteed thread-safe in C++11 and later (Meyers' Singleton).
   */
  static ConfigManager& instance();

  // Prevent copy and move assignment/construction
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;
  ConfigManager(ConfigManager&&) = delete;
  ConfigManager& operator=(ConfigManager&&) = delete;

  /**
   * @brief Loads and parses a YAML configuration file from disk.
   * @param config_file_path System file path to the .yaml configuration file.
   * @return true if file was loaded successfully, false on syntax or file access error.
   */
  bool load_file(const std::string& config_file_path);

  /**
   * @brief Retrieves a parameter value by dot-notation key path.
   * @tparam T Target data type (int, double, std::string, bool, etc.).
   * @param key_path Dot-separated path string (e.g., "gateway.telemetry.queue_capacity").
   * @param default_value Fallback value returned if key is missing or invalid.
   * @return T Requested parameter value or fallback default value.
   */
  template <typename T>
  T get(const std::string& key_path, const T& default_value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_loaded_) {
      return default_value;
    }

    try {
      YAML::Node node = navigate_path(key_path);
      if (node.IsDefined() && !node.IsNull()) {
        return node.as<T>();
      }
    } catch (const std::exception&) {
      // Fallback cleanly on type casting mismatch or invalid node
    }

    return default_value;
  }

  /**
   * @brief Checks whether a YAML file has been successfully loaded.
   */
  [[nodiscard]] bool is_loaded() const;

private:
  ConfigManager() = default;
  ~ConfigManager() = default;

  /**
   * @brief Traverses nested YAML nodes using a dot-notation key path string.
   * @param key_path Dot-separated path (e.g., "gateway.logging.log_level").
   * @return YAML::Node Target node or undefined node if path does not exist.
   */
  YAML::Node navigate_path(const std::string& key_path) const;

  mutable std::mutex mutex_;
  YAML::Node root_node_;
  bool is_loaded_{false};
};

} // namespace utils
} // namespace secure_telemetry_gateway

#endif // SECURE_TELEMETRY_GATEWAY_UTILS_CONFIG_MANAGER_HPP_