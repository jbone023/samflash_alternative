#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtCore/QTimer>

#include "../Core/flash_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void refresh_devices();
    void connect_device();
    void disconnect_device();
    void select_firmware_file();
    void flash_firmware();
    void verify_firmware();
    void erase_device();
    void update_ui_state();
    void on_progress_update(const SamFlash::FlashProgress& progress);

private:
    void setup_ui();
    void setup_connections();
    void log_message(const QString& message);
    void update_device_info();
    
    // Core functionality
    std::unique_ptr<SamFlash::FlashManager> flash_manager_;
    
    // UI Components
    QWidget* central_widget_;
    QVBoxLayout* main_layout_;
    
    // Device selection section
    QHBoxLayout* device_layout_;
    QComboBox* device_combo_;
    QPushButton* refresh_button_;
    QPushButton* connect_button_;
    QPushButton* disconnect_button_;
    
    // Device info section
    QGridLayout* info_layout_;
    QLabel* device_name_label_;
    QLabel* device_type_label_;
    QLabel* flash_size_label_;
    QLabel* connection_status_label_;
    
    // Firmware operations section
    QHBoxLayout* firmware_layout_;
    QLabel* firmware_file_label_;
    QPushButton* browse_button_;
    QPushButton* flash_button_;
    QPushButton* verify_button_;
    QPushButton* erase_button_;
    
    // Progress section
    QProgressBar* progress_bar_;
    QLabel* progress_label_;
    
    // Log section
    QTextEdit* log_text_;
    
    // Status and timer
    QTimer* ui_update_timer_;
    
    QString selected_firmware_file_;
};

#endif // MAIN_WINDOW_H
