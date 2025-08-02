#include "main_window.h"
#include <QtCore/QDateTime>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidgetItem>
#include <QtGui/QCloseEvent>
#include <QtGui/QShowEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , flash_manager_(std::make_unique<SamFlash::FlashManager>())
    , flash_thread_(nullptr)
    , settings_(new QSettings("SamFlash", "Alternative", this))
    , is_dark_theme_(false)
    , flash_operation_running_(false) {
    
    // Load settings first
    load_settings();
    
    // Setup UI components
    setup_ui();
    setup_connections();
    
    // Bridge FlashManager signals for MVC separation
    bridge_flash_manager_signals();
    
    // Setup timers
    ui_update_timer_ = new QTimer(this);
    connect(ui_update_timer_, &QTimer::timeout, this, &MainWindow::update_ui_state);
    ui_update_timer_->start(100); // Update every 100ms
    
    device_scan_timer_ = new QTimer(this);
    connect(device_scan_timer_, &QTimer::timeout, this, &MainWindow::refresh_devices);
    device_scan_timer_->start(5000); // Scan every 5 seconds
    
    // Initial setup
    refresh_devices();
    apply_theme(is_dark_theme_);
    
    log_message("SamFlash Alternative started", "INFO");
}

MainWindow::~MainWindow() {
    save_settings();
    if (flash_thread_ && flash_thread_->isRunning()) {
        flash_thread_->quit();
        flash_thread_->wait(3000);
    }
}

void MainWindow::setup_ui() {
    setWindowTitle("SamFlash Alternative v1.0");
    setMinimumSize(800, 600);
    
    central_widget_ = new QWidget(this);
    setCentralWidget(central_widget_);
    main_layout_ = new QVBoxLayout(central_widget_);
    
    // Device selection section
    device_layout_ = new QHBoxLayout();
    device_combo_ = new QComboBox();
    refresh_button_ = new QPushButton("Refresh");
    connect_button_ = new QPushButton("Connect");
    disconnect_button_ = new QPushButton("Disconnect");
    
    device_layout_->addWidget(new QLabel("Device:"));
    device_layout_->addWidget(device_combo_);
    device_layout_->addWidget(refresh_button_);
    device_layout_->addWidget(connect_button_);
    device_layout_->addWidget(disconnect_button_);
    device_layout_->addStretch();
    
    main_layout_->addLayout(device_layout_);
    
    // Device info section
    info_layout_ = new QGridLayout();
    device_name_label_ = new QLabel("N/A");
    device_type_label_ = new QLabel("N/A");
    flash_size_label_ = new QLabel("N/A");
    connection_status_label_ = new QLabel("Disconnected");
    
    info_layout_->addWidget(new QLabel("Device Name:"), 0, 0);
    info_layout_->addWidget(device_name_label_, 0, 1);
    info_layout_->addWidget(new QLabel("Type:"), 1, 0);
    info_layout_->addWidget(device_type_label_, 1, 1);
    info_layout_->addWidget(new QLabel("Flash Size:"), 2, 0);
    info_layout_->addWidget(flash_size_label_, 2, 1);
    info_layout_->addWidget(new QLabel("Status:"), 3, 0);
    info_layout_->addWidget(connection_status_label_, 3, 1);
    
    main_layout_->addLayout(info_layout_);
    
    // Firmware operations section
    firmware_layout_ = new QHBoxLayout();
    firmware_file_label_ = new QLabel("No file selected");
    browse_button_ = new QPushButton("Browse...");
    flash_button_ = new QPushButton("Flash");
    verify_button_ = new QPushButton("Verify");
    erase_button_ = new QPushButton("Erase");
    
    firmware_layout_->addWidget(new QLabel("Firmware:"));
    firmware_layout_->addWidget(firmware_file_label_);
    firmware_layout_->addWidget(browse_button_);
    firmware_layout_->addWidget(flash_button_);
    firmware_layout_->addWidget(verify_button_);
    firmware_layout_->addWidget(erase_button_);
    
    main_layout_->addLayout(firmware_layout_);
    
    // Progress section
    progress_bar_ = new QProgressBar();
    progress_label_ = new QLabel("Ready");
    
    main_layout_->addWidget(progress_bar_);
    main_layout_->addWidget(progress_label_);
    
    // Log section
    main_layout_->addWidget(new QLabel("Log:"));
    log_text_ = new QTextEdit();
    log_text_->setMaximumHeight(200);
    log_text_->setReadOnly(true);
    main_layout_->addWidget(log_text_);
    
    // Status bar
    statusBar()->showMessage("Ready");
    
    update_ui_state();
}

void MainWindow::setup_connections() {
    connect(refresh_button_, &QPushButton::clicked, this, &MainWindow::refresh_devices);
    connect(connect_button_, &QPushButton::clicked, this, &MainWindow::connect_device);
    connect(disconnect_button_, &QPushButton::clicked, this, &MainWindow::disconnect_device);
    connect(browse_button_, &QPushButton::clicked, this, &MainWindow::select_firmware_file);
    connect(flash_button_, &QPushButton::clicked, this, &MainWindow::flash_firmware);
    connect(verify_button_, &QPushButton::clicked, this, &MainWindow::verify_firmware);
    connect(erase_button_, &QPushButton::clicked, this, &MainWindow::erase_device);
}

void MainWindow::refresh_devices() {
    device_combo_->clear();
    auto devices = flash_manager_->scan_devices();
    
    for (const auto& device : devices) {
        QString device_string = QString("%1 (%2)")
            .arg(QString::fromStdString(device.name))
            .arg(QString::fromStdString(device.id));
        device_combo_->addItem(device_string, QString::fromStdString(device.id));
    }
    
    log_message(QString("Found %1 device(s)").arg(devices.size()));
}

void MainWindow::connect_device() {
    if (device_combo_->currentIndex() < 0) {
        QMessageBox::warning(this, "Warning", "Please select a device first.");
        return;
    }
    
    QString device_id = device_combo_->currentData().toString();
    bool success = flash_manager_->connect_device(device_id.toStdString());
    
    if (success) {
        log_message("Connected to device: " + device_id);
        update_device_info();
    } else {
        QString error = QString::fromStdString(flash_manager_->get_last_error());
        QMessageBox::critical(this, "Connection Error", "Failed to connect: " + error);
        log_message("Connection failed: " + error);
    }
}

void MainWindow::disconnect_device() {
    bool success = flash_manager_->disconnect_device();
    if (success) {
        log_message("Device disconnected");
        update_device_info();
    }
}

void MainWindow::select_firmware_file() {
    QString file = QFileDialog::getOpenFileName(
        this, 
        "Select Firmware File", 
        "", 
        "Firmware Files (*.bin *.hex *.elf);;All Files (*)"
    );
    
    if (!file.isEmpty()) {
        selected_firmware_file_ = file;
        firmware_file_label_->setText(QFileInfo(file).fileName());
        
        bool success = flash_manager_->load_firmware_file(file.toStdString());
        if (success) {
            log_message("Loaded firmware file: " + file);
        } else {
            QString error = QString::fromStdString(flash_manager_->get_last_error());
            QMessageBox::critical(this, "File Error", "Failed to load file: " + error);
            log_message("Failed to load file: " + error);
        }
    }
}

void MainWindow::flash_firmware() {
    if (selected_firmware_file_.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please select a firmware file first.");
        return;
    }
    
    // Disable UI during flashing
    flash_button_->setEnabled(false);
    verify_button_->setEnabled(false);
    erase_button_->setEnabled(false);
    
    log_message("Starting firmware flash...");
    
    // Start flashing in a separate thread (simplified for demo)
    bool success = flash_manager_->flash_firmware();
    
    if (success) {
        log_message("Firmware flashed successfully");
    } else {
        QString error = QString::fromStdString(flash_manager_->get_last_error());
        QMessageBox::critical(this, "Flash Error", "Failed to flash: " + error);
        log_message("Flash failed: " + error);
    }
    
    // Re-enable UI
    flash_button_->setEnabled(true);
    verify_button_->setEnabled(true);
    erase_button_->setEnabled(true);
}

void MainWindow::verify_firmware() {
    log_message("Starting firmware verification...");
    
    bool success = flash_manager_->verify_firmware();
    
    if (success) {
        log_message("Firmware verification successful");
        QMessageBox::information(this, "Success", "Firmware verification completed successfully.");
    } else {
        QString error = QString::fromStdString(flash_manager_->get_last_error());
        QMessageBox::critical(this, "Verification Error", "Verification failed: " + error);
        log_message("Verification failed: " + error);
    }
}

void MainWindow::erase_device() {
    int ret = QMessageBox::question(
        this, 
        "Confirm Erase", 
        "Are you sure you want to erase the device? This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No
    );
    
    if (ret == QMessageBox::Yes) {
        log_message("Starting device erase...");
        
        bool success = flash_manager_->erase_device();
        
        if (success) {
            log_message("Device erased successfully");
        } else {
            QString error = QString::fromStdString(flash_manager_->get_last_error());
            QMessageBox::critical(this, "Erase Error", "Failed to erase: " + error);
            log_message("Erase failed: " + error);
        }
    }
}

void MainWindow::update_ui_state() {
    auto status = flash_manager_->get_status();
    bool connected = (status != SamFlash::FlashStatus::IDLE && status != SamFlash::FlashStatus::DISCONNECTED);
    bool busy = (status == SamFlash::FlashStatus::FLASHING || 
                 status == SamFlash::FlashStatus::VERIFYING ||
                 status == SamFlash::FlashStatus::CONNECTING);
    
    connect_button_->setEnabled(!connected && device_combo_->currentIndex() >= 0);
    disconnect_button_->setEnabled(connected);
    flash_button_->setEnabled(connected && !busy && !selected_firmware_file_.isEmpty());
    verify_button_->setEnabled(connected && !busy);
    erase_button_->setEnabled(connected && !busy);
    
    // Update status labels
    switch (status) {
        case SamFlash::FlashStatus::IDLE:
            connection_status_label_->setText("Idle");
            break;
        case SamFlash::FlashStatus::CONNECTING:
            connection_status_label_->setText("Connecting...");
            break;
        case SamFlash::FlashStatus::CONNECTED:
            connection_status_label_->setText("Connected");
            break;
        case SamFlash::FlashStatus::FLASHING:
            connection_status_label_->setText("Flashing...");
            break;
        case SamFlash::FlashStatus::VERIFYING:
            connection_status_label_->setText("Verifying...");
            break;
        case SamFlash::FlashStatus::COMPLETE:
            connection_status_label_->setText("Complete");
            break;
        case SamFlash::FlashStatus::ERROR:
            connection_status_label_->setText("Error");
            break;
        case SamFlash::FlashStatus::DISCONNECTED:
            connection_status_label_->setText("Disconnected");
            break;
    }
}

void MainWindow::on_progress_update(const SamFlash::FlashProgress& progress) {
    progress_bar_->setValue(static_cast<int>(progress.percentage));
    progress_label_->setText(QString::fromStdString(progress.current_operation));
    
    QString status_msg = QString("%1 - %2% (%3/%4 bytes)")
        .arg(QString::fromStdString(progress.current_operation))
        .arg(progress.percentage, 0, 'f', 1)
        .arg(progress.bytes_written)
        .arg(progress.total_bytes);
    
    statusBar()->showMessage(status_msg);
}

void MainWindow::log_message(const QString& message) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    log_text_->append(QString("[%1] %2").arg(timestamp, message));
}

void MainWindow::update_device_info() {
    auto device_info = flash_manager_->get_connected_device();
    
    if (!device_info.id.empty()) {
        device_name_label_->setText(QString::fromStdString(device_info.name));
        device_type_label_->setText(QString::fromStdString("USB/Serial")); // Simplified
        flash_size_label_->setText(QString("%1 KB").arg(device_info.flash_size / 1024));
    } else {
        device_name_label_->setText("N/A");
        device_type_label_->setText("N/A");
        flash_size_label_->setText("N/A");
    }
}

#include "main_window.moc"
