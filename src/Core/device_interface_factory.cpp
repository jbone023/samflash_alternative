#include "device_interface.h"
#include "usb_serial_interface.h"
#include "samsung_flasher.h"
#include <unordered_map>
#include <memory>
#include <functional>

namespace SamFlash {

// Factory registration map
using InterfaceCreator = std::function<std::unique_ptr<IDeviceInterface>()>;
using InterfaceRegistry = std::unordered_map<DeviceType, InterfaceCreator>;

// Registry with compile-time registration
static InterfaceRegistry& get_registry() {
    static InterfaceRegistry registry;
    
    // Register known interfaces
    static bool initialized = false;
    if (!initialized) {
        registry[DeviceType::USB_SERIAL] = []() -> std::unique_ptr<IDeviceInterface> {
            return std::make_unique<USBSerialInterface>();
        };
        
        // Additional interfaces can be registered here
        registry[DeviceType::JTAG] = []() -> std::unique_ptr<IDeviceInterface> {
            return std::make_unique<SamsungFlasher>();
        };
        
        // registry[DeviceType::SWD] = []() -> std::unique_ptr<IDeviceInterface> {
        //     return std::make_unique<SWDInterface>();
        // };
        
        // registry[DeviceType::NETWORK] = []() -> std::unique_ptr<IDeviceInterface> {
        //     return std::make_unique<NetworkInterface>();
        // };
        
        initialized = true;
    }
    
    return registry;
}

std::unique_ptr<IDeviceInterface> DeviceInterfaceFactory::create_interface(DeviceType type) {
    auto& registry = get_registry();
    auto it = registry.find(type);
    
    if (it != registry.end()) {
        return it->second();
    }
    
    return nullptr;
}

std::vector<DeviceType> DeviceInterfaceFactory::get_supported_types() {
    auto& registry = get_registry();
    std::vector<DeviceType> types;
    
    for (const auto& [type, creator] : registry) {
        types.push_back(type);
    }
    
    return types;
}

// Template-based automatic registration system for extensibility
template<typename InterfaceType>
class InterfaceRegistrar {
public:
    InterfaceRegistrar(DeviceType type) {
        get_registry()[type] = []() -> std::unique_ptr<IDeviceInterface> {
            return std::make_unique<InterfaceType>();
        };
    }
};

// Macro for easy registration
#define REGISTER_INTERFACE(InterfaceType, DeviceTypeEnum) \
    static InterfaceRegistrar<InterfaceType> registrar_##InterfaceType(DeviceTypeEnum);

// Register the USB Serial interface (already done above, but shown for example)
// REGISTER_INTERFACE(USBSerialInterface, DeviceType::USB_SERIAL)

} // namespace SamFlash
