#ifndef SAMSUNG_STRATEGY_H
#define SAMSUNG_STRATEGY_H

#include "iflash_strategy.h"
#include "samsung_flasher.h"
#include <algorithm>

namespace SamFlash {

class SamsungStrategy : public IFlashStrategy {
public:
    SamsungStrategy() = default;
    ~SamsungStrategy() override = default;
    
    // Strategy initialization
    bool initialize(std::shared_ptr<IDeviceInterface> device_interface, const FlashConfig& config) override {
        device_interface_ = device_interface;
        config_ = config;
        last_error_.clear();
        
        std::cout << "SamsungStrategy: Initialized for Samsung device flashing" << std::endl;
        return true;
    }
    
    void cleanup() override {
        std::cout << "SamsungStrategy: Cleanup completed" << std::endl;
        device_interface_.reset();
    }
    
    // Main flashing operations
    bool erase_device() override {
        std::cout << "SamsungStrategy: Starting device erase..." << std::endl;
        
        if (!device_interface_) {
            last_error_ = "Device interface not initialized";
            return false;
        }
        
        // Use Samsung-specific erase
        bool result = device_interface_->erase_chip();
        
        EnhancedFlashProgress progress;
        progress.bytes_written = 0;
        progress.total_bytes = 1; // Erase is a single operation
        progress.percentage = result ? 100.0 : 0.0;
        progress.current_operation = "Erasing device";
        progress.status = result ? FlashStatus::COMPLETE : FlashStatus::ERROR;
        progress.current_partition = "Samsung main";
        progress.total_partitions = 1;
        progress.completed_partitions = result ? 1 : 0;
        
        PartitionProgress partition_progress;
        partition_progress.partition_name = "Samsung main";
        partition_progress.partition_id = 0;
        partition_progress.partition_size = 1;
        partition_progress.partition_percentage = result ? 100.0 : 0.0;
        partition_progress.current_operation = "Erasing";
        partition_progress.status = result ? FlashStatus::COMPLETE : FlashStatus::ERROR;
        progress.partition_progress.push_back(partition_progress);
        
        update_progress(progress);
        
        if (result) {
            std::cout << "SamsungStrategy: Erase completed successfully" << std::endl;
        } else {
            last_error_ = "Failed to erase Samsung device";
            std::cout << "SamsungStrategy: Erase failed" << std::endl;
        }
        
        return result;
    }
    
    bool write_firmware(const std::vector<uint8_t>& firmware_data) override {
        std::cout << "SamsungStrategy: Starting firmware write..." << std::endl;
        
        if (!device_interface_) {
            last_error_ = "Device interface not initialized";
            return false;
        }
        
        // Ensure PIT is parsed before writing
        auto samsung_flasher = dynamic_cast<SamsungFlasher*>(device_interface_.get());
        if (samsung_flasher && samsung_flasher->get_pit_entries().empty()) {
            samsung_flasher->parse_pit();
            samsung_flasher->map_partitions();
        }
        
        const size_t chunk_size = 1024; // Samsung-specific chunk size
        size_t total_chunks = (firmware_data.size() + chunk_size - 1) / chunk_size;
        
        EnhancedFlashProgress progress;
        progress.total_bytes = firmware_data.size();
        progress.current_operation = "Writing firmware";
        progress.status = FlashStatus::FLASHING;
        progress.current_partition = "Samsung main";
        progress.total_partitions = 1;
        progress.completed_partitions = 0;
        
        PartitionProgress partition_progress;
        partition_progress.partition_name = "Samsung main";
        partition_progress.partition_id = 0;
        partition_progress.partition_size = firmware_data.size();
        partition_progress.current_operation = "Writing";
        partition_progress.status = FlashStatus::FLASHING;
        progress.partition_progress.push_back(partition_progress);
        
        for (size_t i = 0; i < firmware_data.size(); i += chunk_size) {
            std::vector<uint8_t> chunk(firmware_data.begin() + i, firmware_data.begin() + std::min(firmware_data.size(), i + chunk_size));
            
            if (!device_interface_->write_page(i, chunk)) {
                last_error_ = "Write error at address: " + std::to_string(i);
                return false;
            }
            
            progress.bytes_written = i + chunk.size();
            progress.percentage = 100.0 * static_cast<double>(progress.bytes_written) / firmware_data.size();
            progress.partition_progress[0].bytes_written = progress.bytes_written;
            progress.partition_progress[0].partition_percentage = progress.percentage;
            
            update_progress(progress);
        }
        
        progress.status = FlashStatus::COMPLETE;
        progress.completed_partitions = 1;
        progress.partition_progress[0].status = FlashStatus::COMPLETE;
        update_progress(progress);
        
        std::cout << "SamsungStrategy: Firmware write completed successfully" << std::endl;
        return true;
    }
    
    bool verify_firmware(const std::vector<uint8_t>& expected_data) override {
        std::cout << "SamsungStrategy: Starting firmware verification..." << std::endl;
        
        if (!device_interface_) {
            last_error_ = "Device interface not initialized";
            return false;
        }
        
        auto samsung_flasher = dynamic_cast<SamsungFlasher*>(device_interface_.get());
        if (!samsung_flasher) {
            last_error_ = "Invalid device interface for Samsung strategy";
            return false;
        }
        
        EnhancedFlashProgress progress;
        progress.total_bytes = expected_data.size();
        progress.current_operation = "Verifying firmware";
        progress.status = FlashStatus::VERIFYING;
        progress.current_partition = "Samsung main";
        progress.total_partitions = 1;
        progress.completed_partitions = 0;
        
        PartitionProgress partition_progress;
        partition_progress.partition_name = "Samsung main";
        partition_progress.bytes_written = 0;
        partition_progress.partition_size = expected_data.size();
        partition_progress.current_operation = "Verifying";
        partition_progress.status = FlashStatus::VERIFYING;
        progress.partition_progress.push_back(partition_progress);
        
        if (samsung_flasher->verify_flash(expected_data)) {
            progress.bytes_written = expected_data.size();
            progress.percentage = 100.0;
            progress.status = FlashStatus::COMPLETE;
            progress.completed_partitions = 1;
            progress.partition_progress[0].bytes_written = expected_data.size();
            progress.partition_progress[0].partition_percentage = 100.0;
            progress.partition_progress[0].status = FlashStatus::COMPLETE;
            update_progress(progress);
            std::cout << "SamsungStrategy: Firmware verification completed successfully" << std::endl;
            return true;
        } else {
            last_error_ = "Firmware verification failed for Samsung device";
            std::cout << "SamsungStrategy: Firmware verification failed" << std::endl;
            return false;
        }
    }
    
    // Progress reporting
    void set_progress_callback(std::function<void(const EnhancedFlashProgress&)> callback) override {
        progress_callback_ = callback;
    }
    
    // Strategy information
    std::string get_strategy_name() const override {
        return "SamsungStrategy";
    }
    
    std::vector<std::string> get_supported_device_signatures() const override {
        return {"samsung_signature"};
    }
    
    // Error handling
    std::string get_last_error() const override {
        return last_error_;
    }
    
    void clear_error() override {
        last_error_.clear();
    }
    
    // Device-specific validation
    bool is_compatible_with_device(const DeviceInfo& device_info) const override {
        return device_info.manufacturer == "Samsung";
    }
};

} // namespace SamFlash

#endif // SAMSUNG_STRATEGY_H

