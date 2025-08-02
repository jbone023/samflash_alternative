#ifndef IFLASH_STRATEGY_H
#define IFLASH_STRATEGY_H

#include "device_interface.h"
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace SamFlash {

// Forward declarations
struct FlashProgress;
struct FlashConfig;
struct PartitionInfo;

// Enhanced progress structure to include partition-level status
struct PartitionProgress {
    std::string partition_name;
    uint32_t partition_id;
    uint32_t bytes_written;
    uint32_t partition_size;
    double partition_percentage;
    std::string current_operation; // "Erasing", "Writing", "Verifying"
    FlashStatus status;
};

struct EnhancedFlashProgress : public FlashProgress {
    std::vector<PartitionProgress> partition_progress;
    std::string current_partition;
    uint32_t total_partitions;
    uint32_t completed_partitions;
};

// Strategy interface for different flashing protocols
class IFlashStrategy {
public:
    virtual ~IFlashStrategy() = default;
    
    // Strategy initialization
    virtual bool initialize(std::shared_ptr<IDeviceInterface> device_interface, const FlashConfig& config) = 0;
    virtual void cleanup() = 0;
    
    // Main flashing operations
    virtual bool erase_device() = 0;
    virtual bool write_firmware(const std::vector<uint8_t>& firmware_data) = 0;
    virtual bool verify_firmware(const std::vector<uint8_t>& expected_data) = 0;
    
    // Progress reporting
    virtual void set_progress_callback(std::function<void(const EnhancedFlashProgress&)> callback) = 0;
    
    // Strategy information
    virtual std::string get_strategy_name() const = 0;
    virtual std::vector<std::string> get_supported_device_signatures() const = 0;
    
    // Error handling
    virtual std::string get_last_error() const = 0;
    virtual void clear_error() = 0;
    
    // Device-specific validation
    virtual bool is_compatible_with_device(const DeviceInfo& device_info) const = 0;
    
protected:
    // Helper methods for concrete strategies
    virtual void update_progress(const EnhancedFlashProgress& progress) {
        if (progress_callback_) {
            progress_callback_(progress);
        }
    }
    
    std::function<void(const EnhancedFlashProgress&)> progress_callback_;
    std::shared_ptr<IDeviceInterface> device_interface_;
    FlashConfig config_;
    std::string last_error_;
};

} // namespace SamFlash

#endif // IFLASH_STRATEGY_H
