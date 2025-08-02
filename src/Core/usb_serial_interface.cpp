#include "usb_serial_interface.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace SamFlash {

USBSerialInterface::USBSerialInterface() 
    : transport_(std::make_unique<SerialTransport>()), connected_(false), status_(FlashStatus::IDLE) {
}

USBSerialInterface::~USBSerialInterface() {
    if (connected_) {
        disconnect();
    }
}

std::vector<DeviceInfo> USBSerialInterface::discover_devices() {
    std::vector<DeviceInfo> devices;
    
    // Use real serial port enumeration
    auto ports = SerialTransport::enumerate_ports();
    
    for (const auto& port : ports) {
        DeviceInfo device;
        device.id = port.port_name;
        device.name = port.description.empty() ? "Serial Device" : port.description;
        device.manufacturer = port.manufacturer.empty() ? "Unknown" : port.manufacturer;
        device.type = DeviceType::USB_SERIAL;
        device.port_or_address = port.port_name;
        device.flash_size = 1024 * 1024; // Default 1MB - would be detected on connection
        device.page_size = 256; // Default page size
        device.is_connected = false;
        
        // Filter for likely microcontroller programmer devices
        if (port.manufacturer.find("FTDI") != std::string::npos ||
            port.manufacturer.find("Microchip") != std::string::npos ||
            port.manufacturer.find("Atmel") != std::string::npos ||
            port.description.find("USB Serial") != std::string::npos) {
            devices.push_back(device);
        }
    }
    
    return devices;
}

bool USBSerialInterface::connect(const std::string& device_id) {
    if (connected_) {
        last_error_ = "Already connected to a device";
        return false;
    }
    
    status_ = FlashStatus::CONNECTING;
    
    // Configure serial port for SAM-BA protocol (typical settings)
    SerialConfig config;
    config.baud_rate = 115200;
    config.data_bits = 8;
    config.parity = SerialParity::NONE;
    config.stop_bits = SerialStopBits::ONE;
    config.flow_control = SerialFlowControl::NONE;
    config.read_timeout = std::chrono::milliseconds(2000);
    config.write_timeout = std::chrono::milliseconds(2000);
    
    // Open the serial port
    if (!transport_->open(device_id, config)) {
        last_error_ = "Failed to open serial port: " + transport_->get_last_error();
        status_ = FlashStatus::ERROR;
        return false;
    }
    
    // Try to enter programming mode
    if (!enter_programming_mode()) {
        transport_->close();
        status_ = FlashStatus::ERROR;
        return false;
    }
    
    connected_ = true;
    status_ = FlashStatus::CONNECTED;
    device_id_ = device_id;
    port_name_ = device_id;
    
    // Store device info
    current_device_info_.id = device_id;
    current_device_info_.port_or_address = device_id;
    current_device_info_.is_connected = true;
    
    return true;
}

bool USBSerialInterface::disconnect() {
    if (!connected_) {
        return true;
    }
    
    // Try to exit programming mode gracefully
    exit_programming_mode();
    
    // Close the transport
    if (transport_->is_open()) {
        transport_->close();
    }
    
    connected_ = false;
    status_ = FlashStatus::DISCONNECTED;
    device_id_.clear();
    port_name_.clear();
    
    return true;
}

bool USBSerialInterface::is_connected() const {
    return connected_;
}

DeviceInfo USBSerialInterface::get_device_info() const {
    DeviceInfo info;
    if (connected_) {
        info.id = device_id_;
        info.name = "SAM Device";
        info.manufacturer = "Microchip";
        info.type = DeviceType::USB_SERIAL;
        info.port_or_address = "COM3";
        info.flash_size = 1024 * 1024;
        info.page_size = 256;
        info.is_connected = true;
    }
    return info;
}

std::string USBSerialInterface::get_device_signature() {
    if (!connected_) {
        last_error_ = "Device not connected";
        return "";
    }
    
    // Simulate reading device signature
    return "0x1E9502"; // Example signature
}

bool USBSerialInterface::erase_chip() {
    if (!connected_) {
        last_error_ = "Device not connected";
        return false;
    }
    
    status_ = FlashStatus::FLASHING;
    
    // Simulate chip erase
    for (int i = 0; i <= 100; i += 10) {
        if (progress_callback_) {
            FlashProgress progress;
            progress.bytes_written = 0;
            progress.total_bytes = 0;
            progress.percentage = i;
            progress.current_operation = "Erasing chip";
            progress.status = FlashStatus::FLASHING;
            progress_callback_(progress);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    status_ = FlashStatus::CONNECTED;
    return true;
}

bool USBSerialInterface::erase_page(uint32_t address) {
    if (!connected_) {
        last_error_ = "Device not connected";
        return false;
    }
    
    // Simulate page erase
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return true;
}

bool USBSerialInterface::write_page(uint32_t address, const std::vector<uint8_t>& data) {
    if (!connected_) {
        last_error_ = "Device not connected";
        return false;
    }
    
    if (data.size() > 256) {
        last_error_ = "Page size exceeds maximum (256 bytes)";
        return false;
    }
    
    // Simulate page write
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return true;
}

std::vector<uint8_t> USBSerialInterface::read_page(uint32_t address, uint32_t size) {
    std::vector<uint8_t> data;
    
    if (!connected_) {
        last_error_ = "Device not connected";
        return data;
    }
    
    // Simulate reading data
    data.resize(size);
    for (uint32_t i = 0; i < size; ++i) {
        data[i] = (address + i) & 0xFF; // Dummy data
    }
    
    return data;
}

bool USBSerialInterface::verify_flash(const std::vector<uint8_t>& expected_data, uint32_t start_address) {
    if (!connected_) {
        last_error_ = "Device not connected";
        return false;
    }
    
    status_ = FlashStatus::VERIFYING;
    
    // Simulate verification process
    for (size_t i = 0; i < expected_data.size(); i += 256) {
        size_t chunk_size = std::min(size_t(256), expected_data.size() - i);
        auto read_data = read_page(start_address + i, chunk_size);
        
        // Compare data (simplified)
        for (size_t j = 0; j < chunk_size; ++j) {
            if (read_data[j] != expected_data[i + j]) {
                last_error_ = "Verification failed at address " + std::to_string(start_address + i + j);
                status_ = FlashStatus::ERROR;
                return false;
            }
        }
        
        if (progress_callback_) {
            FlashProgress progress;
            progress.bytes_written = i + chunk_size;
            progress.total_bytes = expected_data.size();
            progress.percentage = (double(i + chunk_size) / expected_data.size()) * 100.0;
            progress.current_operation = "Verifying";
            progress.status = FlashStatus::VERIFYING;
            progress_callback_(progress);
        }
    }
    
    status_ = FlashStatus::COMPLETE;
    return true;
}

void USBSerialInterface::set_progress_callback(std::function<void(const FlashProgress&)> callback) {
    progress_callback_ = callback;
}

FlashStatus USBSerialInterface::get_status() const {
    return status_;
}

std::string USBSerialInterface::get_last_error() const {
    return last_error_;
}

void USBSerialInterface::clear_error() {
    last_error_.clear();
}

// Protocol helper implementations
bool USBSerialInterface::send_command(const std::vector<uint8_t>& command) {
    if (!transport_->is_open()) {
        last_error_ = "Transport not open";
        return false;
    }
    
    if (!transport_->write(command)) {
        last_error_ = "Failed to send command: " + transport_->get_last_error();
        return false;
    }
    
    return true;
}

std::vector<uint8_t> USBSerialInterface::receive_response(size_t expected_size) {
    if (!transport_->is_open()) {
        last_error_ = "Transport not open";
        return std::vector<uint8_t>();
    }
    
    if (expected_size > 0) {
        return transport_->read(expected_size);
    } else {
        // Read whatever is available
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Give time for response
        size_t available = transport_->bytes_available();
        if (available > 0) {
            return transport_->read(available);
        }
    }
    
    return std::vector<uint8_t>();
}

bool USBSerialInterface::wait_for_response_with_timeout(std::chrono::milliseconds timeout) {
    auto start = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start < timeout) {
        if (transport_->bytes_available() > 0) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return false;
}

// SAM-BA protocol implementations (simplified examples)
bool USBSerialInterface::enter_programming_mode() {
    // Clear any existing data
    transport_->clear_buffers();
    
    // Send autobaud detection character sequence
    std::vector<uint8_t> autobaud = {'#'};
    if (!send_command(autobaud)) {
        last_error_ = "Failed to send autobaud character";
        return false;
    }
    
    // Wait for response
    if (!wait_for_response_with_timeout(std::chrono::milliseconds(1000))) {
        last_error_ = "No response to autobaud character";
        return false;
    }
    
    // Read response
    auto response = receive_response();
    if (response.empty() || response[0] != '\r') {
        last_error_ = "Invalid autobaud response";
        return false;
    }
    
    // Send version command to verify communication
    std::vector<uint8_t> version_cmd = {'V', '#'};
    if (!send_command(version_cmd)) {
        last_error_ = "Failed to send version command";
        return false;
    }
    
    if (!wait_for_response_with_timeout(std::chrono::milliseconds(1000))) {
        last_error_ = "No response to version command";
        return false;
    }
    
    auto version_response = receive_response();
    if (version_response.empty()) {
        last_error_ = "Empty version response";
        return false;
    }
    
    return true;
}

bool USBSerialInterface::exit_programming_mode() {
    if (!transport_->is_open()) {
        return true; // Already closed
    }
    
    // Send go command to exit programming mode
    std::vector<uint8_t> go_cmd = {'G', '0', '0', '0', '0', '0', '0', '0', '0', '#'};
    send_command(go_cmd); // Don't check return - device may reset immediately
    
    return true;
}

std::vector<uint8_t> USBSerialInterface::create_read_command(uint32_t address, uint32_t size) {
    std::vector<uint8_t> cmd;
    cmd.push_back('w'); // Read word command
    
    // Convert address to hex string
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << address;
    std::string addr_str = ss.str();
    
    for (char c : addr_str) {
        cmd.push_back(static_cast<uint8_t>(c));
    }
    
    cmd.push_back(',');
    
    // Convert size to hex string
    ss.str("");
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << size;
    std::string size_str = ss.str();
    
    for (char c : size_str) {
        cmd.push_back(static_cast<uint8_t>(c));
    }
    
    cmd.push_back('#');
    
    return cmd;
}

std::vector<uint8_t> USBSerialInterface::create_write_command(uint32_t address, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> cmd;
    cmd.push_back('S'); // Send file command
    
    // Convert address to hex string
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << address;
    std::string addr_str = ss.str();
    
    for (char c : addr_str) {
        cmd.push_back(static_cast<uint8_t>(c));
    }
    
    cmd.push_back(',');
    
    // Convert size to hex string
    ss.str("");
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << data.size();
    std::string size_str = ss.str();
    
    for (char c : size_str) {
        cmd.push_back(static_cast<uint8_t>(c));
    }
    
    cmd.push_back('#');
    
    return cmd;
}

std::vector<uint8_t> USBSerialInterface::create_erase_command(uint32_t address) {
    std::vector<uint8_t> cmd;
    cmd.push_back('E'); // Erase command
    
    // Convert address to hex string
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << address;
    std::string addr_str = ss.str();
    
    for (char c : addr_str) {
        cmd.push_back(static_cast<uint8_t>(c));
    }
    
    cmd.push_back('#');
    
    return cmd;
}

} // namespace SamFlash
