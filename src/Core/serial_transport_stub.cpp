#include "serial_transport.h"
#include <thread>
#include <chrono>

#ifndef HAVE_LIBSERIALPORT

namespace SamFlash {

SerialTransport::SerialTransport() : port_(nullptr), is_open_(false) {}

SerialTransport::~SerialTransport() {
    if (is_open_) {
        close();
    }
}

std::vector<SerialPortInfo> SerialTransport::enumerate_ports() {
    std::vector<SerialPortInfo> ports;
    
    // Windows stub ports
    SerialPortInfo com1;
    com1.port_name = "COM1";
    com1.description = "Communications Port (COM1)";
    com1.manufacturer = "Microsoft";
    ports.push_back(com1);
    
    SerialPortInfo com3;
    com3.port_name = "COM3";
    com3.description = "USB Serial Port (COM3)";
    com3.manufacturer = "FTDI";
    ports.push_back(com3);
    
    return ports;
}

bool SerialTransport::open(const std::string& port_name, const SerialConfig& config) {
    if (is_open_) {
        last_error_ = "Port already open";
        return false;
    }
    
    // Simulate opening delay
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    config_ = config;
    is_open_ = true;
    
    return true;
}

bool SerialTransport::close() {
    if (!is_open_) return true;
    
    is_open_ = false;
    return true;
}

bool SerialTransport::is_open() const {
    return is_open_;
}

bool SerialTransport::configure(const SerialConfig& config) {
    config_ = config;
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
    
    // Simulate write delay
    std::this_thread::sleep_for(std::chrono::milliseconds(size / 100 + 1));
    
    return true;
}

std::vector<uint8_t> SerialTransport::read(size_t max_bytes) {
    std::vector<uint8_t> data;
    size_t bytes_read;
    if (read(data.data(), max_bytes, bytes_read)) {
        data.resize(bytes_read);
    }
    return data;
}

bool SerialTransport::read(uint8_t* buffer, size_t size, size_t& bytes_read) {
    if (!check_port_open()) return false;
    
    // Simulate read delay
    std::this_thread::sleep_for(std::chrono::milliseconds(size / 100 + 1));
    
    // Generate dummy data
    for (size_t i = 0; i < size; ++i) {
        buffer[i] = static_cast<uint8_t>(i & 0xFF);
    }
    bytes_read = size;
    
    return true;
}

bool SerialTransport::write_bulk(const std::vector<uint8_t>& data, 
                               std::function<void(const TransferProgress&)> progress_callback) {
    if (!check_port_open()) return false;
    
    const size_t chunk_size = 1024;
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
    
    const size_t chunk_size = 1024;
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
    return true;
}

bool SerialTransport::drain() {
    return true;
}

size_t SerialTransport::bytes_available() {
    return 0;
}

bool SerialTransport::clear_buffers() {
    return true;
}

bool SerialTransport::set_dtr(bool state) {
    return true;
}

bool SerialTransport::set_rts(bool state) {
    return true;
}

bool SerialTransport::get_cts() {
    return true;
}

bool SerialTransport::get_dsr() {
    return true;
}

bool SerialTransport::get_dcd() {
    return true;
}

bool SerialTransport::get_ri() {
    return false;
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
    SerialPortInfo info;
    info.port_name = "COM3";
    info.description = "Stub Serial Port"; 
    info.manufacturer = "Stub";
    return info;
}

bool SerialTransport::check_port_open() {
    if (!is_open_) {
        last_error_ = "Port is not open";
        return false;
    }
    return true;
}

} // namespace SamFlash

#endif // !HAVE_LIBSERIALPORT
