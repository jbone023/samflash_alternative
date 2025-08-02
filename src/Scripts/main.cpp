#include <iostream>
#include <fstream>
#include <filesystem>
#include "Core/flash_manager.h"
#include "Core/device_interface.h"
#include "cli_utils.h"
#include <CLI/CLI.hpp>

using namespace SamFlash;
using namespace SamFlash::CLI;

// Command handlers
int handle_scan(bool json_output) {
    ProgressReporter reporter(json_output);
    FlashManager manager;
    
    reporter.report_scan_start();
    auto devices = manager.scan_devices();
    reporter.report_scan_complete(devices);
    
    return 0;
}

int handle_flash(const std::string& firmware_file, const std::string& device_id, bool json_output, bool verify, bool erase) {
    ProgressReporter reporter(json_output);
    FlashManager manager;
    
    // Configure flash settings
    FlashConfig config = manager.get_config();
    config.verify_after_write = verify;
    config.erase_before_write = erase;
    manager.set_config(config);
    
    // Set up progress callback
    manager.set_progress_callback([&reporter](const FlashProgress& progress) {
        reporter.report_flash_progress(progress);
    });
    
    // Load firmware
    if (!manager.load_firmware_file(firmware_file)) {
        reporter.report_flash_complete(false, "Failed to load firmware: " + manager.get_last_error());
        return 1;
    }
    
    // Connect to device if specified
    if (!device_id.empty()) {
        if (!manager.connect_device(device_id)) {
            reporter.report_flash_complete(false, "Failed to connect to device: " + manager.get_last_error());
            return 1;
        }
    } else {
        // Auto-detect first available device
        auto devices = manager.scan_devices();
        if (devices.empty()) {
            reporter.report_flash_complete(false, "No devices found");
            return 1;
        }
        if (!manager.connect_device(devices[0].id)) {
            reporter.report_flash_complete(false, "Failed to connect to device: " + manager.get_last_error());
            return 1;
        }
    }
    
    reporter.report_flash_start(device_id.empty() ? "auto" : device_id, firmware_file);
    
    // Flash firmware
    if (manager.flash_firmware()) {
        reporter.report_flash_complete(true, "Flashing completed successfully");
        return 0;
    } else {
        reporter.report_flash_complete(false, "Flashing failed: " + manager.get_last_error());
        return 1;
    }
}

int handle_verify(const std::string& firmware_file, const std::string& device_id, bool json_output) {
    ProgressReporter reporter(json_output);
    FlashManager manager;
    
    // Load firmware for verification
    if (!manager.load_firmware_file(firmware_file)) {
        reporter.report_verify_complete(false);
        return 1;
    }
    
    // Connect to device
    if (!device_id.empty()) {
        if (!manager.connect_device(device_id)) {
            reporter.report_verify_complete(false);
            return 1;
        }
    } else {
        auto devices = manager.scan_devices();
        if (devices.empty() || !manager.connect_device(devices[0].id)) {
            reporter.report_verify_complete(false);
            return 1;
        }
    }
    
    // Verify firmware
    bool success = manager.verify_firmware();
    reporter.report_verify_complete(success);
    
    return success ? 0 : 1;
}

int handle_erase(const std::string& device_id, bool json_output) {
    ProgressReporter reporter(json_output);
    FlashManager manager;
    
    // Connect to device
    if (!device_id.empty()) {
        if (!manager.connect_device(device_id)) {
            reporter.report_erase_complete(false);
            return 1;
        }
    } else {
        auto devices = manager.scan_devices();
        if (devices.empty() || !manager.connect_device(devices[0].id)) {
            reporter.report_erase_complete(false);
            return 1;
        }
    }
    
    // Erase device
    bool success = manager.erase_device();
    reporter.report_erase_complete(success);
    
    return success ? 0 : 1;
}

int handle_batch(const std::string& batch_file, bool json_output) {
    ProgressReporter reporter(json_output);
    
    // Parse YAML job file
    BatchJob batch_job;
    try {
        batch_job = Utils::parse_yaml_job(batch_file);
    } catch (const std::exception& e) {
        reporter.report_flash_complete(false, "Failed to parse batch file: " + std::string(e.what()));
        return 1;
    }
    
    if (!Utils::validate_yaml_job(batch_job)) {
        reporter.report_flash_complete(false, "Invalid batch job configuration");
        return 1;
    }
    
    FlashManager manager;
    int successful_jobs = 0;
    int failed_jobs = 0;
    
    // Process each job
    for (const auto& job : batch_job.jobs) {
        // Configure manager for this job
        FlashConfig config = manager.get_config();
        config.verify_after_write = job.verify;
        config.erase_before_write = job.erase;
        config.retry_count = job.retry_count;
        config.timeout_ms = job.timeout_ms;
        manager.set_config(config);
        
        // Load firmware
        if (!manager.load_firmware_file(job.firmware_file)) {
            failed_jobs++;
            continue;
        }
        
        // Find matching devices
        auto all_devices = manager.scan_devices();
        auto target_devices = Utils::filter_devices(all_devices, job.device_filter);
        
        if (target_devices.empty()) {
            failed_jobs++;
            continue;
        }
        
        // Flash each matching device
        for (const auto& device : target_devices) {
            if (manager.connect_device(device.id) && manager.flash_firmware()) {
                successful_jobs++;
            } else {
                failed_jobs++;
            }
            manager.disconnect_device();
        }
    }
    
    reporter.report_batch_summary(batch_job.jobs.size(), successful_jobs, failed_jobs);
    return failed_jobs > 0 ? 1 : 0;
}

int handle_script(const std::string& yaml_file, bool json_output) {
    // This is an alias for batch processing with enhanced YAML job support
    return handle_batch(yaml_file, json_output);
}

int main(int argc, char** argv) {
    CLI::App app{"SamFlash CLI - Modern firmware flashing tool", "samflash"};
    app.require_subcommand(1);
    
    // Global options
    bool json_output = false;
    app.add_flag("--json,-j", json_output, "Enable JSON output for CI/CD integration");
    
    // Scan command
    auto scan_cmd = app.add_subcommand("scan", "Scan for connected devices");
    scan_cmd->callback([&]() {
        return handle_scan(json_output);
    });
    
    // Flash command
    auto flash_cmd = app.add_subcommand("flash", "Flash firmware to device");
    std::string flash_file;
    std::string flash_device_id;
    bool flash_verify = true;
    bool flash_erase = true;
    
    flash_cmd->add_option("--file,-f", flash_file, "Firmware file to flash")
        ->required()
        ->check(CLI::ExistingFile);
    flash_cmd->add_option("--device,-d", flash_device_id, "Target device ID (auto-detect if not specified)");
    flash_cmd->add_flag("--no-verify", flash_verify, "Skip verification after flashing")
        ->default_val(true)
        ->transform([](bool flag) { return !flag; });
    flash_cmd->add_flag("--no-erase", flash_erase, "Skip erase before flashing")
        ->default_val(true)
        ->transform([](bool flag) { return !flag; });
    
    flash_cmd->callback([&]() {
        return handle_flash(flash_file, flash_device_id, json_output, flash_verify, flash_erase);
    });
    
    // Verify command
    auto verify_cmd = app.add_subcommand("verify", "Verify firmware on device");
    std::string verify_file;
    std::string verify_device_id;
    
    verify_cmd->add_option("--file,-f", verify_file, "Firmware file to verify against")
        ->required()
        ->check(CLI::ExistingFile);
    verify_cmd->add_option("--device,-d", verify_device_id, "Target device ID (auto-detect if not specified)");
    
    verify_cmd->callback([&]() {
        return handle_verify(verify_file, verify_device_id, json_output);
    });
    
    // Erase command
    auto erase_cmd = app.add_subcommand("erase", "Erase device flash memory");
    std::string erase_device_id;
    
    erase_cmd->add_option("--device,-d", erase_device_id, "Target device ID (auto-detect if not specified)");
    
    erase_cmd->callback([&]() {
        return handle_erase(erase_device_id, json_output);
    });
    
    // Batch command
    auto batch_cmd = app.add_subcommand("batch", "Execute batch operations from device list");
    std::string batch_list;
    
    batch_cmd->add_option("--list,-l", batch_list, "YAML file containing batch job definitions")
        ->required()
        ->check(CLI::ExistingFile);
    
    batch_cmd->callback([&]() {
        return handle_batch(batch_list, json_output);
    });
    
    // Script command (enhanced YAML job processing)
    auto script_cmd = app.add_subcommand("script", "Execute scripting jobs from YAML files");
    std::string script_file;
    
    script_cmd->add_option("file", script_file, "YAML job file to execute")
        ->required()
        ->check(CLI::ExistingFile);
    
    script_cmd->callback([&]() {
        return handle_script(script_file, json_output);
    });
    
    try {
        app.parse(argc, argv);
        return 0;
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    } catch (const std::exception& e) {
        if (json_output) {
            JsonOutput error_output;
            error_output.success = false;
            error_output.error = e.what();
            error_output.timestamp = Utils::get_timestamp();
            std::cout << Utils::serialize_json(error_output) << std::endl;
        } else {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return 1;
    }
}
