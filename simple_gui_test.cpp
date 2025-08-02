#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStatusBar>

class SimpleMainWindow : public QMainWindow {
    Q_OBJECT

public:
    SimpleMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("SamFlash Alternative v1.0 - Qt6 GUI");
        setMinimumSize(1200, 800);
        
        setupUI();
    }

private slots:
    void onRefreshClicked() {
        device_list_->addItem("Samsung Galaxy (COM3)");
        device_list_->addItem("Test Device (COM4)");
        log_console_->append("[INFO] Found 2 device(s)");
    }
    
    void onConnectClicked() {
        log_console_->append("[INFO] Connecting to selected device...");
        progress_bar_->setValue(25);
    }
    
    void onFlashClicked() {
        log_console_->append("[INFO] Starting firmware flash...");
        progress_bar_->setValue(75);
    }

private:
    void setupUI() {
        // Main widget and layout
        auto central_widget = new QWidget(this);
        setCentralWidget(central_widget);
        
        auto main_layout = new QHBoxLayout(central_widget);
        
        // Left panel - Device list
        auto device_group = new QGroupBox("Devices", this);
        auto device_layout = new QVBoxLayout(device_group);
        
        device_list_ = new QListWidget(this);
        device_layout->addWidget(device_list_);
        
        auto refresh_btn = new QPushButton("Refresh Devices", this);
        connect(refresh_btn, &QPushButton::clicked, this, &SimpleMainWindow::onRefreshClicked);
        device_layout->addWidget(refresh_btn);
        
        auto connect_btn = new QPushButton("Connect", this);
        connect(connect_btn, &QPushButton::clicked, this, &SimpleMainWindow::onConnectClicked);
        device_layout->addWidget(connect_btn);
        
        main_layout->addWidget(device_group);
        
        // Right panel - Controls and log
        auto right_layout = new QVBoxLayout();
        
        // Firmware section
        auto firmware_group = new QGroupBox("Firmware", this);
        auto firmware_layout = new QHBoxLayout(firmware_group);
        
        firmware_file_label_ = new QLabel("No file selected", this);
        firmware_layout->addWidget(firmware_file_label_);
        
        auto browse_btn = new QPushButton("Browse...", this);
        firmware_layout->addWidget(browse_btn);
        
        right_layout->addWidget(firmware_group);
        
        // Control buttons
        auto control_group = new QGroupBox("Flash Operations", this);
        auto control_layout = new QHBoxLayout(control_group);
        
        auto flash_btn = new QPushButton("Flash Firmware", this);
        connect(flash_btn, &QPushButton::clicked, this, &SimpleMainWindow::onFlashClicked);
        control_layout->addWidget(flash_btn);
        
        auto verify_btn = new QPushButton("Verify", this);
        control_layout->addWidget(verify_btn);
        
        auto erase_btn = new QPushButton("Erase", this);
        control_layout->addWidget(erase_btn);
        
        right_layout->addWidget(control_group);
        
        // Progress bar
        auto progress_group = new QGroupBox("Progress", this);
        auto progress_layout = new QVBoxLayout(progress_group);
        
        progress_bar_ = new QProgressBar(this);
        progress_layout->addWidget(progress_bar_);
        
        auto progress_label = new QLabel("Ready", this);
        progress_layout->addWidget(progress_label);
        
        right_layout->addWidget(progress_group);
        
        // Log console
        auto log_group = new QGroupBox("Console Log", this);
        auto log_layout = new QVBoxLayout(log_group);
        
        log_console_ = new QTextEdit(this);
        log_console_->setReadOnly(true);
        log_console_->append("[INFO] SamFlash Alternative started");
        log_layout->addWidget(log_console_);
        
        right_layout->addWidget(log_group);
        
        main_layout->addLayout(right_layout);
        
        // Status bar
        statusBar()->showMessage("Ready");
    }

private:
    QListWidget* device_list_;
    QLabel* firmware_file_label_;
    QProgressBar* progress_bar_;
    QTextEdit* log_console_;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    SimpleMainWindow window;
    window.show();
    
    return app.exec();
}

#include "simple_gui_test.moc"
