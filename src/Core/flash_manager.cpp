#include "flash_manager.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include "samsung_flasher.h"
#include "generic_strategy.h"
#include "samsung_strategy.h"

namespace SamFlash {

FlashManager::FlashManager()
    : current_status_(FlashStatus::IDLE), progress_percentage_(0.0) {
    // Initialize with USB Serial interface by default
    device_interface_ = DeviceInterfaceFactory::create_interface(DeviceType::USB_SERIAL);
}

FlashManager::~FlashManager() {
    disconnect_device();
}

void FlashManager::set_config(const FlashConfig& config) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    config_ = config;
}

FlashConfig FlashManager::get_config() const {
    std::lock_guard<std::mutex> lock(status_mutex_);
    return config_;
}

std::vector<DeviceInfo> FlashManager::scan_devices() {
    return device_interface_->discover_devices();
}

bool FlashManager::connect_device(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(status_mutex_);
    if (device_interface_->connect(device_id)) {
        // Check if it's a Samsung device
        if(device_interface_->get_device_signature() == "samsung_signature") {
            device_interface_ = std::make_unique<SamsungFlasher>();
            device_interface_->connect(device_id);
        }
        select_strategy();
        current_status_ = FlashStatus::CONNECTED;
        return true;
    }
    set_error("Failed to connect to device");
    return false;
}

bool FlashManager::disconnect_device() {
    std::lock_guard<std::mutex> lock(status_mutex_);
    if (device_interface_->disconnect()) {
        current_status_ = FlashStatus::DISCONNECTED;
        return true;
    }
    set_error("Failed to disconnect from device");
    return false;
}

DeviceInfo FlashManager::get_connected_device() const {
    return device_interface_->get_device_info();
}

bool FlashManager::load_firmware_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        set_error("Failed to open firmware file: " + file_path);
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file data
    firmware_data_.resize(file_size);
    if (!file.read(reinterpret_cast<char*>(firmware_data_.data()), file_size)) {
        set_error("Failed to read firmware file: " + file_path);
        return false;
    }
    
    file.close();
    return validate_firmware_data();
}

bool FlashManager::flash_firmware() {
    if (!flash_strategy_) {
        set_error("No flashing strategy selected");
        return false;
    }
    return flash_strategy_->write_firmware(firmware_data_);
}

bool FlashManager::verify_firmware() {
    if (!flash_strategy_) {
        set_error("No flashing strategy selected");
        return false;
    }
    return flash_strategy_->verify_firmware(firmware_data_);
}

bool FlashManager::erase_device() {
    if (!flash_strategy_) {
        set_error("No flashing strategy selected");
        return false;
    }
    return flash_strategy_->erase_device();
}

void FlashManager::set_progress_callback(std::function<void(const FlashProgress&)> callback) {
    progress_callback_ = callback;
}

FlashStatus FlashManager::get_status() const {
    return current_status_;
}

double FlashManager::get_progress_percentage() const {
    return progress_percentage_;
}

std::string FlashManager::get_last_error() const {
    return last_error_;
}

void FlashManager::clear_error() {
    last_error_.clear();
}

std::vector<uint8_t> FlashManager::read_device_flash(uint32_t start_address, uint32_t size) {
    return device_interface_->read_page(start_address, size);
}

bool FlashManager::write_device_flash(uint32_t start_address, const std::vector<uint8_t>& data) {
    return device_interface_->write_page(start_address, data);
}

void FlashManager::set_error(const std::string& error) {
    last_error_ = error;
}

void FlashManager::update_progress(const FlashProgress& progress) {
    if (progress_callback_) {
        progress_callback_(progress);
    }
    progress_percentage_ = progress.percentage;
}

bool FlashManager::validate_firmware_data() {
    return !firmware_data_.empty();
}

void FlashManager::select_strategy() {
    if (!device_interface_) {
        return;
    }
    
    DeviceInfo device_info = device_interface_->get_device_info();
    std::cout << "Selecting strategy for device: " << device_info.manufacturer << std::endl;
    
    if (device_info.manufacturer == "Samsung") {
        flash_strategy_ = std::make_unique<SamsungStrategy>();
        std::cout << "Selected SamsungStrategy" << std::endl;
    } else {
        flash_strategy_ = std::make_unique<GenericStrategy>();
        std::cout << "Selected GenericStrategy" << std::endl;
    }
    
    // Initialize the strategy with the device interface and config
    flash_strategy_->initialize(device_interface_, config_);
    
    // Set up progress callback bridge
    flash_strategy_->set_progress_callback([this](const EnhancedFlashProgress& enhanced_progress) {
        // Convert EnhancedFlashProgress to FlashProgress for compatibility
        FlashProgress legacy_progress;
        legacy_progress.bytes_written = enhanced_progress.bytes_written;
        legacy_progress.total_bytes = enhanced_progress.total_bytes;
        legacy_progress.percentage = enhanced_progress.percentage;
        legacy_progress.current_operation = enhanced_progress.current_operation;
        legacy_progress.status = enhanced_progress.status;
        
        update_progress(legacy_progress);
    });
}

} // namespace SamFlash

