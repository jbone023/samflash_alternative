#ifndef CLI_UTILS_H
#define CLI_UTILS_H

#include <string>
#include <vector>
#include <map>
#include <yaml-cpp/yaml.h>
#include "Core/device_interface.h"
#include "Core/flash_manager.h"

namespace SamFlash {
namespace CLI {

// JSON output structure for CI/CD integration
struct JsonOutput {
    bool success = false;
    std::string message;
    std::string error;
    std::map<std::string, std::string> data;
    std::vector<DeviceInfo> devices;
    double progress = 0.0;
    std::string timestamp;
};

// YAML job configuration structure
struct FlashJob {
    std::string name;
    std::string firmware_file;
    std::string device_filter;  // Device name/ID pattern
    bool verify = true;
    bool erase = true;
    int retry_count = 3;
    int timeout_ms = 10000;
    std::map<std::string, std::string> extra_config;
};

struct BatchJob {
    std::string version = "1.0";
    std::string description;
    std::vector<FlashJob> jobs;
    std::map<std::string, std::string> global_config;
};

// Utility functions
class Utils {
public:
    // JSON serialization
    static std::string serialize_json(const JsonOutput& output);
    static std::string serialize_devices_json(const std::vector<DeviceInfo>& devices);
    static std::string serialize_progress_json(const FlashProgress& progress);
    
    // YAML parsing
    static BatchJob parse_yaml_job(const std::string& file_path);
    static bool validate_yaml_job(const BatchJob& job);
    
    // Device filtering
    static std::vector<DeviceInfo> filter_devices(
        const std::vector<DeviceInfo>& devices, 
        const std::string& filter
    );
    
    // Progress callback for JSON output
    static void json_progress_callback(const FlashProgress& progress, bool output_json);
    
    // Timestamp generation
    static std::string get_timestamp();
    
    // File validation
    static bool file_exists(const std::string& path);
    static bool is_readable(const std::string& path);
    
private:
    static std::string escape_json_string(const std::string& str);
};

// Progress reporter for batch operations
class ProgressReporter {
public:
    ProgressReporter(bool json_output = false);
    void report_scan_start();
    void report_scan_complete(const std::vector<DeviceInfo>& devices);
    void report_flash_start(const std::string& device_id, const std::string& firmware);
    void report_flash_progress(const FlashProgress& progress);
    void report_flash_complete(bool success, const std::string& message);
    void report_verify_complete(bool success);
    void report_erase_complete(bool success);
    void report_batch_summary(int total_jobs, int successful, int failed);
    
private:
    bool json_output_;
    void output_json(const JsonOutput& output);
    void output_text(const std::string& message);
};

} // namespace CLI
} // namespace SamFlash

#endif // CLI_UTILS_H
