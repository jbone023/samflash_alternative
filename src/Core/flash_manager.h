#ifndef FLASH_MANAGER_H
#define FLASH_MANAGER_H

#include "device_interface.h"
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include "iflash_strategy.h"

namespace SamFlash {

struct FlashConfig {
    bool verify_after_write = true;
    bool erase_before_write = true;
    uint32_t retry_count = 3;
    uint32_t timeout_ms = 5000;
    bool enable_progress_reporting = true;
};

class FlashManager {
public:
    FlashManager();
    ~FlashManager();
    
    // Configuration
    void set_config(const FlashConfig& config);
    FlashConfig get_config() const;
    
    // Device management
    std::vector<DeviceInfo> scan_devices();
    bool connect_device(const std::string& device_id);
    bool disconnect_device();
    DeviceInfo get_connected_device() const;
    
    // Firmware operations
    bool load_firmware_file(const std::string& file_path);
    bool flash_firmware();
    bool verify_firmware();
    bool erase_device();
    
    // Progress and status
void set_progress_callback(std::function<void(const FlashProgress&)> callback);
    void select_strategy();
    FlashStatus get_status() const;
    double get_progress_percentage() const;
    
    // Error handling
    std::string get_last_error() const;
    void clear_error();
    
    // Utility functions
    std::vector<uint8_t> read_device_flash(uint32_t start_address, uint32_t size);
    bool write_device_flash(uint32_t start_address, const std::vector<uint8_t>& data);
    
private:
    void set_error(const std::string& error);
    void update_progress(const FlashProgress& progress);
    bool validate_firmware_data();
    
std::unique_ptr<IDeviceInterface> device_interface_;
    std::unique_ptr<IFlashStrategy> flash_strategy_;
    std::vector<uint8_t> firmware_data_;
    FlashConfig config_;
    
    mutable std::mutex status_mutex_;
    std::atomic<FlashStatus> current_status_;
    std::atomic<double> progress_percentage_;
    std::string last_error_;
    
    std::function<void(const FlashProgress&)> progress_callback_;
};

} // namespace SamFlash

#endif // FLASH_MANAGER_H
