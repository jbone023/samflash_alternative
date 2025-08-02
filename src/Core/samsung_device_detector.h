#ifndef SAMSUNG_DEVICE_DETECTOR_H
#define SAMSUNG_DEVICE_DETECTOR_H

#include "device_interface.h"
#include "serial_transport.h"
#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace SamFlash {

// Samsung-specific USB VID/PID combinations for common SoCs
struct SamsungSoCInfo {
    uint16_t vid;
    uint16_t pid;
    std::string soc_name;
    std::string board_name;
    uint32_t default_flash_size;
    uint32_t default_page_size;
    std::string flash_layout;
    std::vector<std::string> supported_protocols;
};

// Samsung device identifiers found via serial descriptor parsing
struct SamsungDeviceIdentifier {
    std::string device_signature;
    std::string bootloader_version;
    std::string chip_id;
    std::string security_version;
    bool download_mode_available;
    bool odin_mode_available;
};

// Flash layout information
struct SamsungFlashLayout {
    std::string partition_name;
    uint32_t start_address;
    uint32_t size;
    std::string partition_type;
    bool is_critical;
};

class SamsungDeviceDetector {
public:
    SamsungDeviceDetector();
    ~SamsungDeviceDetector();
    
    // Main device detection methods
    std::vector<DeviceInfo> scan_for_samsung_devices();
    std::vector<DeviceInfo> scan_for_locked_samsung_devices();
    bool identify_samsung_device(const SerialPortInfo& port_info, DeviceInfo& device_info);
    bool identify_locked_samsung_device(uint16_t vid, uint16_t pid, const std::string& port_name, DeviceInfo& device_info);
    
    // USB VID/PID scanning
    bool is_samsung_vid_pid(uint16_t vid, uint16_t pid);
    bool is_samsung_download_mode(uint16_t vid, uint16_t pid);
    bool is_samsung_odin_mode(uint16_t vid, uint16_t pid);
    SamsungSoCInfo get_soc_info(uint16_t vid, uint16_t pid);
    
    // Windows-specific USB device enumeration for locked devices
    std::vector<std::pair<uint16_t, uint16_t>> enumerate_usb_devices();
    std::string get_device_port_from_vid_pid(uint16_t vid, uint16_t pid);
    
    // Serial descriptor parsing
    bool parse_device_descriptor(const std::string& port_name, SamsungDeviceIdentifier& identifier);
    bool query_device_info(const std::string& port_name, DeviceInfo& device_info);
    
    // Flash layout detection
    std::vector<SamsungFlashLayout> detect_flash_layout(const std::string& device_signature);
    uint32_t detect_flash_size(const std::string& device_signature);
    
    // Protocol detection
    std::vector<std::string> detect_supported_protocols(const std::string& port_name);
    bool test_odin_protocol(const std::string& port_name);
    bool test_download_mode(const std::string& port_name);
    
    // Device information queries
    std::string get_chip_id(const std::string& port_name);
    std::string get_bootloader_version(const std::string& port_name);
    std::string get_security_version(const std::string& port_name);
    
    // Error handling
    std::string get_last_error() const { return last_error_; }
    void clear_error() { last_error_.clear(); }

private:
    // Known Samsung SoC database
    void initialize_soc_database();
    std::map<std::pair<uint16_t, uint16_t>, SamsungSoCInfo> soc_database_;
    
    // Flash layout database
    void initialize_flash_layouts();
    std::map<std::string, std::vector<SamsungFlashLayout>> flash_layouts_;
    
    // Communication helpers
    bool send_command(const std::string& port_name, const std::vector<uint8_t>& command, 
                     std::vector<uint8_t>& response, uint32_t timeout_ms = 3000);
    bool establish_connection(const std::string& port_name);
    void close_connection(const std::string& port_name);
    
    // Protocol-specific detection
    bool detect_odin_mode(const std::string& port_name);
    bool detect_download_mode(const std::string& port_name);
    bool detect_fastboot_mode(const std::string& port_name);
    
    // Response parsing
    bool parse_chip_id_response(const std::vector<uint8_t>& response, std::string& chip_id);
    bool parse_bootloader_response(const std::vector<uint8_t>& response, std::string& version);
    bool parse_flash_info_response(const std::vector<uint8_t>& response, uint32_t& flash_size);
    
    // Member variables
    std::string last_error_;
    std::map<std::string, std::unique_ptr<SerialTransport>> active_connections_;
};

// Samsung-specific constants
namespace SamsungConstants {
    // Common Samsung USB VIDs (Vendor IDs)
    constexpr uint16_t SAMSUNG_VID = 0x04e8;
    constexpr uint16_t SAMSUNG_SEMICONDUCTOR_VID = 0x144d;
    
    // Common Samsung PIDs for different SoCs/modes
    constexpr uint16_t EXYNOS_DOWNLOAD_MODE = 0x1234;
    constexpr uint16_t EXYNOS_ODIN_MODE = 0x6860;
    constexpr uint16_t SNAPDRAGON_DOWNLOAD_MODE = 0x685d;
    constexpr uint16_t MEDIATEK_PRELOADER = 0x0003;
    
    // Protocol command bytes
    constexpr uint8_t ODIN_HANDSHAKE[] = {0x18, 0x00, 0x00, 0x00};
    constexpr uint8_t DOWNLOAD_MODE_HANDSHAKE[] = {0x02, 0x00, 0x00, 0x00};
    constexpr uint8_t CHIP_ID_QUERY[] = {0x16, 0x00, 0x00, 0x00};
    constexpr uint8_t BOOTLOADER_VERSION_QUERY[] = {0x17, 0x00, 0x00, 0x00};
    constexpr uint8_t FLASH_INFO_QUERY[] = {0x1A, 0x00, 0x00, 0x00};
    
    // Response signatures
    constexpr uint8_t ODIN_RESPONSE_OK[] = {0x18, 0x00, 0x00, 0x00, 0x4F, 0x4B, 0x41, 0x59};
    constexpr uint8_t DOWNLOAD_RESPONSE_OK[] = {0x02, 0x00, 0x00, 0x00, 0x52, 0x45, 0x41, 0x44, 0x59};
}

} // namespace SamFlash

#endif // SAMSUNG_DEVICE_DETECTOR_H
