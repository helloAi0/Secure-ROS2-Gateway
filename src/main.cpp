#include "secure_telemetry_gateway/storage/storage_engine.hpp"
#include <iostream>
#include <thread>
#include <vector>

using secure_telemetry_gateway::storage::StorageEngine;
using secure_telemetry_gateway::models::TelemetryData;

void workerThreadFunction(StorageEngine& storage, int thread_id) {
    TelemetryData sample_data;
    sample_data.robot_id = "robot_1";
    sample_data.sensor_type = "lidar";
    sample_data.timestamp_ns = 1700000000000000000ULL;
    sample_data.sequence_id = 101;

    // insert_telemetry will wait safely via wait_until_ready() if needed
    if (!storage.insert_telemetry(sample_data)) {
        std::cerr << "[Thread " << thread_id << "] Failed to insert telemetry.\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "[Gateway Startup] Initializing components...\n";

    // 1. Create StorageEngine instance
    StorageEngine storage("data/telemetry_history.db");

    // 2. MUST Initialize DB BEFORE spawning threads
    if (!storage.init()) {
        std::cerr << "[Fatal] Could not initialize StorageEngine. Halting startup.\n";
        return 1;
    }

    // 3. Launch worker threads AFTER init succeeds
    std::cout << "[Gateway Startup] Starting worker threads...\n";
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; ++i) {
        workers.emplace_back(workerThreadFunction, std::ref(storage), i);
    }

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}