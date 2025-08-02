#include <gtest/gtest.h>
#include <functional>
#include <Core/flash_manager.h>
#include <Core/device_interface.h>

using namespace SamFlash;

TEST(FlashManagerTest, SetAndGetConfig) {
    FlashManager manager;
    FlashConfig config = {false, false, 5, 10000, false};
    manager.set_config(config);

    FlashConfig retrieved_config = manager.get_config();
    EXPECT_EQ(retrieved_config.verify_after_write, config.verify_after_write);
    EXPECT_EQ(retrieved_config.erase_before_write, config.erase_before_write);
    EXPECT_EQ(retrieved_config.retry_count, config.retry_count);
    EXPECT_EQ(retrieved_config.timeout_ms, config.timeout_ms);
    EXPECT_EQ(retrieved_config.enable_progress_reporting, config.enable_progress_reporting);
}

TEST(FlashManagerTest, LoadFirmwareFile) {
    FlashManager manager;
    bool result = manager.load_firmware_file("test_firmware.bin"); // Ensure this file exists in the test environment
    EXPECT_TRUE(result);
}

TEST(DeviceInterfaceFactoryTest, CreateUSBSerialInterface) {
    auto interface = DeviceInterfaceFactory::create_interface(DeviceType::USB_SERIAL);
    EXPECT_NE(interface, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

