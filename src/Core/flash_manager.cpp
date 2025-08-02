#include "flash_manager.h"
#include <algorithm>
#include <fstream>
#include <iostream>

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
    current_status_ = FlashStatus::FLASHING;

    for (size_t i = 0; i < firmware_data_.size(); i += 256) {
        std::vector<uint8_t> page(firmware_data_.begin() + i,
                                  firmware_data_.begin() + std::min(firmware_data_.size(), i + 256));
        if (!device_interface_->write_page(i, page)) {
            current_status_ = FlashStatus::ERROR;
            set_error("Flash error at page: " + std::to_string(i / 256));
            return false;
        }
        update_progress({i, static_cast<uint32_t>(firmware_data_.size()), 100.0 * static_cast<double>(i) / firmware_data_.size(), "Flashing", current_status_});
    }

    current_status_ = FlashStatus::COMPLETE;
    return true;
}

bool FlashManager::verify_firmware() {
    current_status_ = FlashStatus::VERIFYING;
    bool result = device_interface_->verify_flash(firmware_data_);
    current_status_ = result ? FlashStatus::COMPLETE : FlashStatus::ERROR;
    return result;
}

bool FlashManager::erase_device() {
    return device_interface_->erase_chip();
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

} // namespace SamFlash

