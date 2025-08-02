#include "src/Core/serial_transport.h"
#include <iostream>
#include <iomanip>

using namespace SamFlash;

int main() {
    std::cout << "=== Serial Transport Layer Test ===" << std::endl;
    
    // Test port enumeration
    std::cout << "\n1. Enumerating serial ports:" << std::endl;
    auto ports = SerialTransport::enumerate_ports();
    std::cout << "Found " << ports.size() << " serial ports:" << std::endl;
    
    for (const auto& port : ports) {
        std::cout << "  - " << port.port_name 
                  << " (" << port.description << ")" 
                  << " by " << port.manufacturer << std::endl;
    }
    
    // Test transport creation and basic operations
    std::cout << "\n2. Testing SerialTransport class:" << std::endl;
    SerialTransport transport;
    
    std::cout << "Transport created successfully" << std::endl;
    std::cout << "Is open: " << (transport.is_open() ? "Yes" : "No") << std::endl;
    
    // Test configuration
    SerialConfig config;
    config.baud_rate = 115200;
    config.data_bits = 8;
    config.parity = SerialParity::NONE;
    config.stop_bits = SerialStopBits::ONE;
    
    std::cout << "Default configuration:" << std::endl;
    std::cout << "  Baud rate: " << config.baud_rate << std::endl;
    std::cout << "  Data bits: " << config.data_bits << std::endl;
    std::cout << "  Read timeout: " << config.read_timeout.count() << "ms" << std::endl;
    std::cout << "  Write timeout: " << config.write_timeout.count() << "ms" << std::endl;
    
    // Test opening a port (will use stub on this system)
    if (!ports.empty()) {
        std::cout << "\n3. Testing port open/close:" << std::endl;
        std::cout << "Attempting to open port: " << ports[0].port_name << std::endl;
        
        if (transport.open(ports[0].port_name, config)) {
            std::cout << "Port opened successfully!" << std::endl;
            std::cout << "Is open: " << (transport.is_open() ? "Yes" : "No") << std::endl;
            
            // Test write operation
            std::cout << "\n4. Testing write operation:" << std::endl;
            std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04, 0x05};
            if (transport.write(test_data)) {
                std::cout << "Write successful! Wrote " << test_data.size() << " bytes" << std::endl;
            } else {
                std::cout << "Write failed: " << transport.get_last_error() << std::endl;
            }
            
            // Test read operation
            std::cout << "\n5. Testing read operation:" << std::endl;
            auto read_data = transport.read(5);
            if (!read_data.empty()) {
                std::cout << "Read successful! Read " << read_data.size() << " bytes: ";
                for (uint8_t byte : read_data) {
                    std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') 
                              << static_cast<int>(byte) << " ";
                }
                std::cout << std::dec << std::endl;
            } else {
                std::cout << "Read failed: " << transport.get_last_error() << std::endl;
            }
            
            // Test bulk operations with progress
            std::cout << "\n6. Testing bulk write with progress:" << std::endl;
            std::vector<uint8_t> bulk_data(1024, 0xAA); // 1KB of 0xAA
            
            bool progress_reported = false;
            transport.write_bulk(bulk_data, [&](const TransferProgress& progress) {
                if (!progress_reported) {
                    std::cout << "Progress callback working! " 
                              << progress.percentage << "% complete, "
                              << "operation: " << progress.operation << std::endl;
                    progress_reported = true;
                }
            });
            
            transport.close();
            std::cout << "Port closed successfully" << std::endl;
        } else {
            std::cout << "Failed to open port: " << transport.get_last_error() << std::endl;
        }
    } else {
        std::cout << "No ports available for testing" << std::endl;
    }
    
    std::cout << "\n=== Transport layer test completed ===" << std::endl;
    return 0;
}
