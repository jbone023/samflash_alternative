#ifndef SERIAL_TRANSPORT_H
#define SERIAL_TRANSPORT_H

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

#ifdef HAVE_LIBSERIALPORT
#include <libserialport.h>
#endif

namespace SamFlash {

enum class SerialParity {
    NONE,
    ODD,
    EVEN,
    MARK,
    SPACE
};

enum class SerialStopBits {
    ONE,
    ONE_HALF,
    TWO
};

enum class SerialFlowControl {
    NONE,
    XON_XOFF,
    RTS_CTS,
    DTR_DSR
};

struct SerialConfig {
    int baud_rate = 115200;
    int data_bits = 8;
    SerialParity parity = SerialParity::NONE;
    SerialStopBits stop_bits = SerialStopBits::ONE;
    SerialFlowControl flow_control = SerialFlowControl::NONE;
    std::chrono::milliseconds read_timeout = std::chrono::milliseconds(1000);
    std::chrono::milliseconds write_timeout = std::chrono::milliseconds(1000);
};

struct SerialPortInfo {
    std::string port_name;
    std::string description;
    std::string manufacturer;
    std::string product;
    std::string serial_number;
    uint16_t vendor_id = 0;
    uint16_t product_id = 0;
};

struct TransferProgress {
    size_t bytes_transferred;
    size_t total_bytes;
    double percentage;
    std::string operation;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::milliseconds elapsed_time;
    double estimated_remaining_seconds;
};

class SerialTransport {
public:
    SerialTransport();
    ~SerialTransport();

    // Port management
    static std::vector<SerialPortInfo> enumerate_ports();
    bool open(const std::string& port_name, const SerialConfig& config = SerialConfig{});
    bool close();
    bool is_open() const;
    
    // Configuration
    bool configure(const SerialConfig& config);
    SerialConfig get_config() const;
    
    // I/O operations
    bool write(const std::vector<uint8_t>& data);
    bool write(const uint8_t* data, size_t size);
    std::vector<uint8_t> read(size_t max_bytes = 4096);
    bool read(uint8_t* buffer, size_t size, size_t& bytes_read);
    
    // Bulk operations with progress reporting
    bool write_bulk(const std::vector<uint8_t>& data, 
                   std::function<void(const TransferProgress&)> progress_callback = nullptr);
    std::vector<uint8_t> read_bulk(size_t expected_bytes,
                                  std::function<void(const TransferProgress&)> progress_callback = nullptr);
    
    // Flow control and status
    bool flush();
    bool drain();
    size_t bytes_available();
    bool clear_buffers();
    
    // Signal control
    bool set_dtr(bool state);
    bool set_rts(bool state);
    bool get_cts();
    bool get_dsr();
    bool get_dcd();
    bool get_ri();
    
    // Timeout handling
    void set_read_timeout(std::chrono::milliseconds timeout);
    void set_write_timeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds get_read_timeout() const;
    std::chrono::milliseconds get_write_timeout() const;
    
    // Error handling
    std::string get_last_error() const;
    void clear_error();
    
    // Port information
    SerialPortInfo get_port_info() const;

private:
#ifdef HAVE_LIBSERIALPORT
    struct sp_port* port_;
#else
    void* port_; // Placeholder for stub implementation
#endif
    SerialConfig config_;
    std::string last_error_;
    bool is_open_;
    
    // Helper methods
#ifdef HAVE_LIBSERIALPORT
    sp_parity convert_parity(SerialParity parity) const;
    sp_stopbits convert_stop_bits(SerialStopBits stop_bits) const;
    sp_flowcontrol convert_flow_control(SerialFlowControl flow_control) const;
    SerialParity convert_parity(sp_parity parity) const;
    SerialStopBits convert_stop_bits(sp_stopbits stop_bits) const;
    SerialFlowControl convert_flow_control(sp_flowcontrol flow_control) const;
    
    void set_error_from_result(sp_return result);
#endif
    bool check_port_open();
};

} // namespace SamFlash

#endif // SERIAL_TRANSPORT_H
