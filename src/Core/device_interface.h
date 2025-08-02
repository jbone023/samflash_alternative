#ifndef DEVICE_INTERFACE_H
#define DEVICE_INTERFACE_H

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace SamFlash {

enum class DeviceType {
    USB_SERIAL,
    JTAG,
    SWD,
    NETWORK
};

enum class FlashStatus {
    IDLE,
    CONNECTING,
    CONNECTED,
    FLASHING,
    VERIFYING,
    COMPLETE,
    ERROR,
    DISCONNECTED
};

struct DeviceInfo {
    std::string id;
    std::string name;
    std::string manufacturer;
    DeviceType type;
    std::string port_or_address;
    uint32_t flash_size;
    uint32_t page_size;
    bool is_connected;
};

struct FlashProgress {
    uint32_t bytes_written;
    uint32_t total_bytes;
    double percentage;
    std::string current_operation;
    FlashStatus status;
};

class IDeviceInterface {
public:
    virtual ~IDeviceInterface() = default;
    
    // Device discovery and connection
    virtual std::vector<DeviceInfo> discover_devices() = 0;
    virtual bool connect(const std::string& device_id) = 0;
    virtual bool disconnect() = 0;
    virtual bool is_connected() const = 0;
    
    // Device information
    virtual DeviceInfo get_device_info() const = 0;
    virtual std::string get_device_signature() = 0;
    
    // Flash operations
    virtual bool erase_chip() = 0;
    virtual bool erase_page(uint32_t address) = 0;
    virtual bool write_page(uint32_t address, const std::vector<uint8_t>& data) = 0;
    virtual std::vector<uint8_t> read_page(uint32_t address, uint32_t size) = 0;
    virtual bool verify_flash(const std::vector<uint8_t>& expected_data, uint32_t start_address = 0) = 0;
    
    // Progress and status
    virtual void set_progress_callback(std::function<void(const FlashProgress&)> callback) = 0;
    virtual FlashStatus get_status() const = 0;
    
    // Error handling
    virtual std::string get_last_error() const = 0;
    virtual void clear_error() = 0;
};

// Factory for creating device interfaces
class DeviceInterfaceFactory {
public:
    static std::unique_ptr<IDeviceInterface> create_interface(DeviceType type);
    static std::vector<DeviceType> get_supported_types();
};

} // namespace SamFlash

#endif // DEVICE_INTERFACE_H
