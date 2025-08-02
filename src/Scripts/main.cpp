#include <iostream>
#include "Core/flash_manager.h"
#include "Core/device_interface.h"

int main() {
    std::cout << "Welcome to SamFlashCLI!" << std::endl;
    
    // Demonstrate FlashManager functionality
    SamFlash::FlashManager manager;
    
    // Test configuration
    SamFlash::FlashConfig config;
    config.verify_after_write = false;
    config.retry_count = 5;
    config.timeout_ms = 10000;
    manager.set_config(config);
    
    auto retrieved_config = manager.get_config();
    std::cout << "Config set successfully - Retry count: " << retrieved_config.retry_count << std::endl;
    
    // Test device discovery
    auto devices = manager.scan_devices();
    std::cout << "Found " << devices.size() << " device(s)" << std::endl;
    
    if (!devices.empty()) {
        // Test firmware file loading
        if (manager.load_firmware_file("test_firmware.bin")) {
            std::cout << "Firmware loaded successfully!" << std::endl;
        } else {
            std::cout << "Failed to load firmware: " << manager.get_last_error() << std::endl;
        }
        std::cout << "First device: " << devices[0].name << " (" << devices[0].id << ")" << std::endl;
        
        // Test connection
        if (manager.connect_device(devices[0].id)) {
            std::cout << "Connected to device successfully!" << std::endl;
            
            // Test device info
            auto device_info = manager.get_connected_device();
            std::cout << "Device info - Flash size: " << device_info.flash_size << " bytes" << std::endl;
            
            manager.disconnect_device();
            std::cout << "Disconnected from device" << std::endl;
        } else {
            std::cout << "Failed to connect: " << manager.get_last_error() << std::endl;
        }
    }
    
    // Test DeviceInterfaceFactory
    auto supported_types = SamFlash::DeviceInterfaceFactory::get_supported_types();
    std::cout << "Supported interface types: " << supported_types.size() << std::endl;
    
    auto interface = SamFlash::DeviceInterfaceFactory::create_interface(SamFlash::DeviceType::USB_SERIAL);
    if (interface) {
        std::cout << "USB Serial interface created successfully!" << std::endl;
    }
    
    std::cout << "FlashManager and DeviceInterfaceFactory tests completed!" << std::endl;
    return 0;
}
