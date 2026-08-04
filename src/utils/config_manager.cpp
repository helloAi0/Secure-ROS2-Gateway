#include "secure_telemetry_gateway/utils/config_manager.hpp"
#include "secure_telemetry_gateway/utils/logger.hpp"
#include <fstream>

namespace secure_telemetry_gateway {
namespace utils {

ConfigManager& ConfigManager::instance() {
  static ConfigManager instance;
  return instance;
}

bool ConfigManager::load_file(const std::string& config_file_path) {
  std::lock_guard<std::mutex> lock(mutex_);

  try {
    std::ifstream file(config_file_path);
    if (!file.is_open()) {
      LOG_ERROR("ConfigManager: Failed to open configuration file at '{}'", config_file_path);
      is_loaded_ = false;
      return false;
    }

    root_node_ = YAML::Load(file);
    is_loaded_ = true;
    LOG_INFO("ConfigManager: Successfully loaded configuration file from '{}'", config_file_path);
    return true;

  } catch (const YAML::Exception& ex) {
    LOG_ERROR("ConfigManager: YAML parsing exception in '{}': {}", config_file_path, ex.what());
    is_loaded_ = false;
    return false;
  } catch (const std::exception& ex) {
    LOG_ERROR("ConfigManager: General error loading configuration file: {}", ex.what());
    is_loaded_ = false;
    return false;
  }
}

bool ConfigManager::is_loaded() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return is_loaded_;
}

YAML::Node ConfigManager::navigate_path(const std::string& key_path) const {
  std::stringstream ss(key_path);
  std::string segment;
  YAML::Node current_node = YAML::Clone(root_node_);

  while (std::getline(ss, segment, '.')) {
    if (!current_node.IsDefined() || !current_node.IsMap()) {
      return YAML::Node(); // Return undefined node
    }
    current_node = current_node[segment];
  }

  return current_node;
}

} // namespace utils
} // namespace secure_telemetry_gateway