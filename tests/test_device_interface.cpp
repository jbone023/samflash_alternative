#include <gtest/gtest.h>
#include <Core/device_interface.h>
#include <Core/usb_serial_interface.h>

using namespace SamFlash;

class DeviceInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        interface = std::make_unique<USBSerialInterface>();
    }

    void TearDown() override {
        if (interface->is_connected()) {
            interface->disconnect();
        }
    }

    std::unique_ptr<IDeviceInterface> interface;
};

TEST_F(DeviceInterfaceTest, InitialState) {
    EXPECT_FALSE(interface->is_connected());
    EXPECT_EQ(interface->get_status(), FlashStatus::IDLE);
}

TEST_F(DeviceInterfaceTest, DiscoverDevices) {
    auto devices = interface->discover_devices();
    EXPECT_GT(devices.size(), 0);
    
    for (const auto& device : devices) {
        EXPECT_FALSE(device.id.empty());
        EXPECT_FALSE(device.name.empty());
        EXPECT_GT(device.flash_size, 0);
        EXPECT_GT(device.page_size, 0);
    }
}

TEST_F(DeviceInterfaceTest, ConnectAndDisconnect) {
    auto devices = interface->discover_devices();
    ASSERT_GT(devices.size(), 0);
    
    const std::string& device_id = devices[0].id;
    
    // Test connection
    EXPECT_TRUE(interface->connect(device_id));
    EXPECT_TRUE(interface->is_connected());
    EXPECT_EQ(interface->get_status(), FlashStatus::CONNECTED);
    
    // Test device info after connection
    DeviceInfo info = interface->get_device_info();
    EXPECT_EQ(info.id, device_id);
    EXPECT_TRUE(info.is_connected);
    
    // Test disconnection
    EXPECT_TRUE(interface->disconnect());
    EXPECT_FALSE(interface->is_connected());
    EXPECT_EQ(interface->get_status(), FlashStatus::DISCONNECTED);
}

TEST_F(DeviceInterfaceTest, ErrorHandling) {
    // Test operations without connection
    EXPECT_FALSE(interface->erase_chip());
    EXPECT_FALSE(interface->get_last_error().empty());
    
    interface->clear_error();
    EXPECT_TRUE(interface->get_last_error().empty());
}

TEST_F(DeviceInterfaceTest, ProgressCallback) {
    bool callback_called = false;
    FlashProgress received_progress;
    
    interface->set_progress_callback([&](const FlashProgress& progress) {
        callback_called = true;
        received_progress = progress;
    });
    
    // Connect first
    auto devices = interface->discover_devices();
    ASSERT_GT(devices.size(), 0);
    EXPECT_TRUE(interface->connect(devices[0].id));
    
    // Perform operation that triggers progress
    interface->erase_chip();
    
    EXPECT_TRUE(callback_called);
    EXPECT_GE(received_progress.percentage, 0.0);
    EXPECT_LE(received_progress.percentage, 100.0);
}

TEST_F(DeviceInterfaceTest, FlashOperations) {
    auto devices = interface->discover_devices();
    ASSERT_GT(devices.size(), 0);
    EXPECT_TRUE(interface->connect(devices[0].id));
    
    // Test page operations
    std::vector<uint8_t> test_data = {0x01, 0x02, 0x03, 0x04};
    EXPECT_TRUE(interface->write_page(0x1000, test_data));
    
    auto read_data = interface->read_page(0x1000, test_data.size());
    EXPECT_EQ(read_data.size(), test_data.size());
    
    // Test verification
    EXPECT_TRUE(interface->verify_flash(test_data, 0x1000));
}

TEST(DeviceInterfaceFactoryTest, SupportedTypes) {
    auto types = DeviceInterfaceFactory::get_supported_types();
    EXPECT_GT(types.size(), 0);
    
    // USB_SERIAL should be supported
    auto it = std::find(types.begin(), types.end(), DeviceType::USB_SERIAL);
    EXPECT_NE(it, types.end());
}

TEST(DeviceInterfaceFactoryTest, CreateInvalidInterface) {
    // Test creating an unsupported interface type
    auto interface = DeviceInterfaceFactory::create_interface(DeviceType::JTAG);
    EXPECT_EQ(interface, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
