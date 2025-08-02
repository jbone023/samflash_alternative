#include "serial_transport.h"
#include <thread>
#include <chrono>

#ifdef HAVE_LIBSERIALPORT

namespace SamFlash {

SerialTransport::SerialTransport() : port_(nullptr), is_open_(false) {}

SerialTransport::~SerialTransport() {
    if (is_open_) {
        close();
    }
}

std::vector<SerialPortInfo> SerialTransport::enumerate_ports() {
    std::vector<SerialPortInfo> ports;
    
    struct sp_port** port_list;
    if (sp_list_ports(&port_list) == SP_OK) {
        for (int i = 0; port_list[i] != nullptr; ++i) {
            SerialPortInfo info;
            info.port_name = sp_get_port_name(port_list[i]);
            info.description = sp_get_port_description(port_list[i]);
            info.manufacturer = sp_get_port_manufacturer(port_list[i]);

            ports.push_back(info);
        }
        sp_free_port_list(port_list);
    }
    
    return ports;
}

bool SerialTransport::open(const std::string& port_name, const SerialConfig& config) {
    if (sp_get_port_by_name(port_name.c_str(), &port_) != SP_OK) {
        last_error_ = "Failed to find port " + port_name;
        return false;
    }
    if (sp_open(port_, SP_MODE_READ_WRITE) != SP_OK) {
        last_error_ = "Failed to open port " + port_name;
        return false;
    }
    is_open_ = configure(config);
    if (!is_open_) {
        sp_close(port_);
    }
    return is_open_;
}

bool SerialTransport::close() {
    if (!is_open_) return false;
    is_open_ = false;
    sp_close(port_);
    return true;
}

bool SerialTransport::is_open() const {
    return is_open_;
}

bool SerialTransport::configure(const SerialConfig& set_config) {
    config_ = set_config;
    if (sp_set_baudrate(port_, config_.baud_rate) != SP_OK) {
        set_error_from_result(SP_ERR_ARG);
        return false;
    }
    if (sp_set_bits(port_, config_.data_bits) != SP_OK) {
        set_error_from_result(SP_ERR_ARG);
        return false;
    }
    if (sp_set_parity(port_, convert_parity(config_.parity)) != SP_OK) {
        set_error_from_result(SP_ERR_ARG);
        return false;
    }
    if (sp_set_stopbits(port_, convert_stop_bits(config_.stop_bits)) != SP_OK) {
        set_error_from_result(SP_ERR_ARG);
        return false;
    }
    if (sp_set_flowcontrol(port_, convert_flow_control(config_.flow_control)) != SP_OK) {
        set_error_from_result(SP_ERR_ARG);
        return false;
    }
    return true;
}

SerialConfig SerialTransport::get_config() const {
    return config_;
}

bool SerialTransport::write(const std::vector<uint8_t>& data) {
    return write(data.data(), data.size());
}

bool SerialTransport::write(const uint8_t* data, size_t size) {
    if (!check_port_open()) return false;

    size_t bytes_written = 0;
    auto start = std::chrono::steady_clock::now();
    auto timeout = config_.write_timeout;
    while (bytes_written < size) {
        int result = sp_blocking_write(port_, data + bytes_written, size - bytes_written, timeout.count());
        if (result < 0) {
            set_error_from_result(result);
            return false;
        }
        bytes_written += result;
        if (std::chrono::steady_clock::now() - start > timeout) {
            last_error_ = "Timeout during write operation";
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> SerialTransport::read(size_t max_bytes) {
    std::vector<uint8_t> data(max_bytes);
    size_t bytes_read;
    if (!read(data.data(), max_bytes, bytes_read)) {
        data.resize(0);
    } else {
        data.resize(bytes_read);
    }
    return data;
}

bool SerialTransport::read(uint8_t* buffer, size_t size, size_t& bytes_read) {
    bytes_read = 0;
    if (!check_port_open()) return false;

    auto timeout = config_.read_timeout;
    auto start = std::chrono::steady_clock::now();
    while (bytes_read < size) {
        int result = sp_blocking_read(port_, buffer + bytes_read, size - bytes_read, timeout.count());
        if (result < 0) {
            set_error_from_result(result);
            return false;
        }
        bytes_read += result;
        if (std::chrono::steady_clock::now() - start > timeout) {
            last_error_ = "Timeout during read operation";
            return false;
        }
    }
    return true;
}

bool SerialTransport::write_bulk(const std::vector<uint8_t>& data, 
                               std::function<void(const TransferProgress&)> progress_callback) {
    if (!check_port_open()) return false;
    
    const size_t chunk_size = 1024; // 1KB chunks for progress reporting
    const size_t total_bytes = data.size();
    size_t bytes_written = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (bytes_written < total_bytes) {
        size_t current_chunk = std::min(chunk_size, total_bytes - bytes_written);
        
        if (!write(data.data() + bytes_written, current_chunk)) {
            return false;
        }
        
        bytes_written += current_chunk;
        
        if (progress_callback) {
            TransferProgress progress;
            progress.bytes_transferred = bytes_written;
            progress.total_bytes = total_bytes;
            progress.percentage = (double(bytes_written) / total_bytes) * 100.0;
            progress.operation = "Writing";
            progress.start_time = start_time;
            progress.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);
            
            if (progress.elapsed_time.count() > 0 && bytes_written > 0) {
                double bytes_per_ms = double(bytes_written) / progress.elapsed_time.count();
                double remaining_bytes = total_bytes - bytes_written;
                progress.estimated_remaining_seconds = remaining_bytes / (bytes_per_ms * 1000.0);
            } else {
                progress.estimated_remaining_seconds = 0.0;
            }
            
            progress_callback(progress);
        }
    }
    
    return true;
}

std::vector<uint8_t> SerialTransport::read_bulk(size_t expected_bytes,
                                              std::function<void(const TransferProgress&)> progress_callback) {
    std::vector<uint8_t> data;
    if (!check_port_open()) return data;
    
    const size_t chunk_size = 1024; // 1KB chunks for progress reporting
    data.reserve(expected_bytes);
    size_t bytes_read = 0;
    auto start_time = std::chrono::steady_clock::now();
    
    while (bytes_read < expected_bytes) {
        size_t current_chunk = std::min(chunk_size, expected_bytes - bytes_read);
        auto chunk_data = read(current_chunk);
        
        if (chunk_data.empty()) {
            last_error_ = "Failed to read expected data";
            return std::vector<uint8_t>();
        }
        
        data.insert(data.end(), chunk_data.begin(), chunk_data.end());
        bytes_read += chunk_data.size();
        
        if (progress_callback) {
            TransferProgress progress;
            progress.bytes_transferred = bytes_read;
            progress.total_bytes = expected_bytes;
            progress.percentage = (double(bytes_read) / expected_bytes) * 100.0;
            progress.operation = "Reading";
            progress.start_time = start_time;
            progress.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time);
            
            if (progress.elapsed_time.count() > 0 && bytes_read > 0) {
                double bytes_per_ms = double(bytes_read) / progress.elapsed_time.count();
                double remaining_bytes = expected_bytes - bytes_read;
                progress.estimated_remaining_seconds = remaining_bytes / (bytes_per_ms * 1000.0);
            } else {
                progress.estimated_remaining_seconds = 0.0;
            }
            
            progress_callback(progress);
        }
    }
    
    return data;
}

bool SerialTransport::flush() {
    return (sp_flush(port_, SP_BUF_BOTH) == SP_OK);
}

bool SerialTransport::drain() {
    return true;  // libserialport doesn't directly support draining
}

size_t SerialTransport::bytes_available() {
    return sp_input_waiting(port_);
}

bool SerialTransport::clear_buffers() {
    return flush();
}

bool SerialTransport::set_dtr(bool state) {
    return (sp_set_dtr(port_, state) == SP_OK);
}

bool SerialTransport::set_rts(bool state) {
    return (sp_set_rts(port_, state) == SP_OK);
}

bool SerialTransport::get_cts() {
    return (sp_get_cts(port_) == SP_OK);
}

bool SerialTransport::get_dsr() {
    return (sp_get_dsr(port_) == SP_OK);
}

bool SerialTransport::get_dcd() {
    return (sp_get_dcd(port_) == SP_OK);
}

bool SerialTransport::get_ri() {
    return (sp_get_ri(port_) == SP_OK);
}

void SerialTransport::set_read_timeout(std::chrono::milliseconds timeout) {
    config_.read_timeout = timeout;
}

void SerialTransport::set_write_timeout(std::chrono::milliseconds timeout) {
    config_.write_timeout = timeout;
}

std::chrono::milliseconds SerialTransport::get_read_timeout() const {
    return config_.read_timeout;
}

std::chrono::milliseconds SerialTransport::get_write_timeout() const {
    return config_.write_timeout;
}

std::string SerialTransport::get_last_error() const {
    return last_error_;
}

void SerialTransport::clear_error() {
    last_error_.clear();
}

SerialPortInfo SerialTransport::get_port_info() const {
    return SerialPortInfo{};  // Could be implemented to fetch current port details
}

sp_parity SerialTransport::convert_parity(SerialParity parity) const {
    switch (parity) {
        case SerialParity::NONE: return SP_PARITY_NONE;
        case SerialParity::ODD: return SP_PARITY_ODD;
        case SerialParity::EVEN: return SP_PARITY_EVEN;
        case SerialParity::MARK: return SP_PARITY_MARK;
        case SerialParity::SPACE: return SP_PARITY_SPACE;
    }
    return SP_PARITY_UNKNOWN;
}

sp_stopbits SerialTransport::convert_stop_bits(SerialStopBits stop_bits) const {
    switch (stop_bits) {
        case SerialStopBits::ONE: return SP_STOPBITS_ONE;
        case SerialStopBits::ONE_HALF: return SP_STOPBITS_ONE_POINT_FIVE;
        case SerialStopBits::TWO: return SP_STOPBITS_TWO;
    }
    return SP_STOPBITS_UNKNOWN;
}

sp_flowcontrol SerialTransport::convert_flow_control(SerialFlowControl flow_control) const {
    switch (flow_control) {
        case SerialFlowControl::NONE: return SP_FLOWCONTROL_NONE;
        case SerialFlowControl::XON_XOFF: return SP_FLOWCONTROL_XONXOFF;
        case SerialFlowControl::RTS_CTS: return SP_FLOWCONTROL_RTSCTS;
        case SerialFlowControl::DTR_DSR: return SP_FLOWCONTROL_DTRDSR;
    }
    return SP_FLOWCONTROL_UNKNOWN;
}

void SerialTransport::set_error_from_result(sp_return result) {
    switch (result) {
        case SP_OK: last_error_ = "No error"; break;
        case SP_ERR_ARG: last_error_ = "Invalid argument"; break;
        case SP_ERR_FAIL: last_error_ = "Operation failed"; break;
        case SP_ERR_SUPP: last_error_ = "Operation not supported"; break;
        case SP_ERR_MEM: last_error_ = "Memory allocation failed"; break;
        default: last_error_ = "Unknown error"; break;
    }
}

bool SerialTransport::check_port_open() {
    if (!is_open_) {
        last_error_ = "Port is not open";
        return false;
    }
    return true;
}

} // namespace SamFlash

#endif // HAVE_LIBSERIALPORT

// Include stub implementation if libserialport is not available
#ifndef HAVE_LIBSERIALPORT
#include "serial_transport_stub.cpp"
#endif

