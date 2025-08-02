#ifndef USB_SERIAL_INTERFACE_H
#define USB_SERIAL_INTERFACE_H

#include "device_interface.h"
#include "serial_transport.h"
#include <string>
#include <atomic>
#include <memory>

namespace SamFlash {

class USBSerialInterface : public IDeviceInterface {
public:
    USBSerialInterface();
    ~USBSerialInterface() override;
    
    // Device discovery and connection
    std::vector<DeviceInfo> discover_devices() override;
    bool connect(const std::string& device_id) override;
    bool disconnect() override;
    bool is_connected() const override;
    
    // Device information
    DeviceInfo get_device_info() const override;
    std::string get_device_signature() override;
    
    // Flash operations
    bool erase_chip() override;
    bool erase_page(uint32_t address) override;
    bool write_page(uint32_t address, const std::vector<uint8_t>& data) override;
    std::vector<uint8_t> read_page(uint32_t address, uint32_t size) override;
    bool verify_flash(const std::vector<uint8_t>& expected_data, uint32_t start_address = 0) override;
    
    // Progress and status
    void set_progress_callback(std::function<void(const FlashProgress&)> callback) override;
    FlashStatus get_status() const override;
    
    // Error handling
    std::string get_last_error() const override;
    void clear_error() override;

private:
    std::unique_ptr<SerialTransport> transport_;
    std::atomic<bool> connected_;
    std::atomic<FlashStatus> status_;
    std::string device_id_;
    std::string port_name_;
    DeviceInfo current_device_info_;
    std::string last_error_;
    std::function<void(const FlashProgress&)> progress_callback_;
    
    // Protocol helpers
    bool send_command(const std::vector<uint8_t>& command);
    std::vector<uint8_t> receive_response(size_t expected_size = 0);
    bool wait_for_response_with_timeout(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));
    
    // SAM-BA protocol commands (example)
    bool enter_programming_mode();
    bool exit_programming_mode();
    std::vector<uint8_t> create_read_command(uint32_t address, uint32_t size);
    std::vector<uint8_t> create_write_command(uint32_t address, const std::vector<uint8_t>& data);
    std::vector<uint8_t> create_erase_command(uint32_t address);
};

} // namespace SamFlash

#endif // USB_SERIAL_INTERFACE_H
