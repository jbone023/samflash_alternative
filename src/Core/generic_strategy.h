#ifndef GENERIC_STRATEGY_H
#define GENERIC_STRATEGY_H

#include "iflash_strategy.h"
#include <iostream>
#include <algorithm>

namespace SamFlash {

class GenericStrategy : public IFlashStrategy {
public:
    GenericStrategy() = default;
    ~GenericStrategy() override = default;
    
    // Strategy initialization
    bool initialize(std::shared_ptr<IDeviceInterface> device_interface, const FlashConfig& config) override {
        device_interface_ = device_interface;
        config_ = config;
        last_error_.clear();
        
        std::cout << "GenericStrategy: Initialized for generic device flashing" << std::endl;
        return true;
    }
    
    void cleanup() override {
        std::cout << "GenericStrategy: Cleanup completed" << std::endl;
        device_interface_.reset();
    }
    
    // Main flashing operations
    bool erase_device() override {
        std::cout << "GenericStrategy: Starting device erase..." << std::endl;
        
        if (!device_interface_) {
            last_error_ = "Device interface not initialized";
            return false;
        }
        
        // Report erase progress
        EnhancedFlashProgress progress;
        progress.bytes_written = 0;
        progress.total_bytes = 1; // Erase is a single operation
        progress.percentage = 0.0;
        progress.current_operation = "Erasing device";
        progress.status = FlashStatus::FLASHING;
        progress.current_partition = "main";
        progress.total_partitions = 1;
        progress.completed_partitions = 0;
        
        // Add partition progress
        PartitionProgress partition_progress;
        partition_progress.partition_name = "main";
        partition_progress.partition_id = 0;
        partition_progress.bytes_written = 0;
        partition_progress.partition_size = 1;
        partition_progress.partition_percentage = 0.0;
        partition_progress.current_operation = "Erasing";
        partition_progress.status = FlashStatus::FLASHING;
        progress.partition_progress.push_back(partition_progress);
        
        update_progress(progress);
        
        bool result = device_interface_->erase_chip();
        
        if (result) {
            progress.percentage = 100.0;
            progress.status = FlashStatus::COMPLETE;
            progress.completed_partitions = 1;
            progress.partition_progress[0].partition_percentage = 100.0;
            progress.partition_progress[0].status = FlashStatus::COMPLETE;
            update_progress(progress);
            std::cout << "GenericStrategy: Device erase completed successfully" << std::endl;
        } else {
            last_error_ = "Failed to erase device";
            std::cout << "GenericStrategy: Device erase failed" << std::endl;
        }
        
        return result;
    }
    
    bool write_firmware(const std::vector<uint8_t>& firmware_data) override {
        std::cout << "GenericStrategy: Starting firmware write..." << std::endl;
        
        if (!device_interface_) {
            last_error_ = "Device interface not initialized";
            return false;
        }
        
        if (firmware_data.empty()) {
            last_error_ = "No firmware data to write";
            return false;
        }
        
        const size_t page_size = 256; // Standard page size
        size_t total_pages = (firmware_data.size() + page_size - 1) / page_size;
        
        EnhancedFlashProgress progress;
        progress.total_bytes = firmware_data.size();
        progress.current_operation = "Writing firmware";
        progress.status = FlashStatus::FLASHING;
        progress.current_partition = "main";
        progress.total_partitions = 1;
        progress.completed_partitions = 0;
        
        // Initialize partition progress
        PartitionProgress partition_progress;
        partition_progress.partition_name = "main";
        partition_progress.partition_id = 0;
        partition_progress.partition_size = firmware_data.size();
        partition_progress.current_operation = "Writing";
        partition_progress.status = FlashStatus::FLASHING;
        progress.partition_progress.push_back(partition_progress);
        
        for (size_t i = 0; i < firmware_data.size(); i += page_size) {
            std::vector<uint8_t> page(firmware_data.begin() + i,
                                    firmware_data.begin() + std::min(firmware_data.size(), i + page_size));
            
            if (!device_interface_->write_page(i, page)) {
                last_error_ = "Write error at address: " + std::to_string(i);
                return false;
            }
            
            // Update progress
            progress.bytes_written = i + page.size();
            progress.percentage = 100.0 * static_cast<double>(progress.bytes_written) / firmware_data.size();
            
            // Update partition progress
            progress.partition_progress[0].bytes_written = progress.bytes_written;
            progress.partition_progress[0].partition_percentage = progress.percentage;
            
            update_progress(progress);
        }
        
        progress.status = FlashStatus::COMPLETE;
        progress.completed_partitions = 1;
        progress.partition_progress[0].status = FlashStatus::COMPLETE;
        update_progress(progress);
        
        std::cout << "GenericStrategy: Firmware write completed successfully" << std::endl;
        return true;
    }
    
    bool verify_firmware(const std::vector<uint8_t>& expected_data) override {
        std::cout << "GenericStrategy: Starting firmware verification..." << std::endl;
        
        if (!device_interface_) {
            last_error_ = "Device interface not initialized";
            return false;
        }
        
        EnhancedFlashProgress progress;
        progress.bytes_written = 0;
        progress.total_bytes = expected_data.size();
        progress.percentage = 0.0;
        progress.current_operation = "Verifying firmware";
        progress.status = FlashStatus::VERIFYING;
        progress.current_partition = "main";
        progress.total_partitions = 1;
        progress.completed_partitions = 0;
        
        // Initialize partition progress
        PartitionProgress partition_progress;
        partition_progress.partition_name = "main";
        partition_progress.partition_id = 0;
        partition_progress.bytes_written = 0;
        partition_progress.partition_size = expected_data.size();
        partition_progress.partition_percentage = 0.0;
        partition_progress.current_operation = "Verifying";
        partition_progress.status = FlashStatus::VERIFYING;
        progress.partition_progress.push_back(partition_progress);
        
        update_progress(progress);
        
        bool result = device_interface_->verify_flash(expected_data);
        
        if (result) {
            progress.bytes_written = expected_data.size();
            progress.percentage = 100.0;
            progress.status = FlashStatus::COMPLETE;
            progress.completed_partitions = 1;
            progress.partition_progress[0].bytes_written = expected_data.size();
            progress.partition_progress[0].partition_percentage = 100.0;
            progress.partition_progress[0].status = FlashStatus::COMPLETE;
            update_progress(progress);
            std::cout << "GenericStrategy: Firmware verification completed successfully" << std::endl;
        } else {
            last_error_ = "Firmware verification failed";
            std::cout << "GenericStrategy: Firmware verification failed" << std::endl;
        }
        
        return result;
    }
    
    // Progress reporting
    void set_progress_callback(std::function<void(const EnhancedFlashProgress&)> callback) override {
        progress_callback_ = callback;
    }
    
    // Strategy information
    std::string get_strategy_name() const override {
        return "GenericStrategy";
    }
    
    std::vector<std::string> get_supported_device_signatures() const override {
        return {"generic", "usb_serial", "default"};
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
        // Generic strategy is compatible with all non-Samsung devices
        return device_info.manufacturer != "Samsung";
    }
};

} // namespace SamFlash

#endif // GENERIC_STRATEGY_H
