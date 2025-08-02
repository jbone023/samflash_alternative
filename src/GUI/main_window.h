#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QAction>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QApplication>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtCore/QThread>
#include <QtCore/QMutex>

#include "../Core/flash_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    // Device management
    void refresh_devices();
    void connect_device();
    void disconnect_device();
    void on_device_list_selection_changed();
    
    // File operations
    void select_firmware_file();
    void clear_firmware_file();
    
    // Flash operations
    void start_flash_operation();
    void stop_flash_operation();
    void flash_firmware();
    void verify_firmware();
    void erase_device();
    
    // UI updates
    void update_ui_state();
    void on_progress_update(const SamFlash::FlashProgress& progress);
    
    // Theme and settings
    void toggle_theme();
    void apply_theme(bool dark_theme);
    void save_settings();
    void load_settings();
    
    // Menu actions
    void show_about();
    void show_preferences();
    void clear_log();
    void save_log();

signals:
    void device_connection_changed(bool connected);
    void flash_operation_started();
    void flash_operation_completed();

private:
    // UI Setup
    void setup_ui();
    void setup_menu_bar();
    void setup_tool_bar();
    void setup_status_bar();
    void setup_central_widget();
    void setup_device_pane();
    void setup_firmware_pane();
    void setup_control_pane();
    void setup_progress_pane();
    void setup_log_pane();
    void setup_connections();
    
    // Helper functions
    void log_message(const QString& message, const QString& level = "INFO");
    void update_device_info();
    void update_device_list();
    void set_ui_enabled(bool enabled);
    QString format_file_size(qint64 bytes);
    QString get_theme_stylesheet(bool dark_theme);
    
    // FlashManager bridge for MVC separation
    void bridge_flash_manager_signals();
    
    // Core functionality
    std::unique_ptr<SamFlash::FlashManager> flash_manager_;
    QThread* flash_thread_;
    QMutex ui_mutex_;
    
    // Main layout
    QWidget* central_widget_;
    QSplitter* main_splitter_;
    QSplitter* left_splitter_;
    
    // Device list pane (left)
    QGroupBox* device_group_;
    QVBoxLayout* device_layout_;
    QListWidget* device_list_;
    QPushButton* refresh_devices_button_;
    QPushButton* connect_device_button_;
    QPushButton* disconnect_device_button_;
    
    // Device info section
    QGroupBox* device_info_group_;
    QGridLayout* device_info_layout_;
    QLabel* device_name_label_;
    QLabel* device_type_label_;
    QLabel* manufacturer_label_;
    QLabel* flash_size_label_;
    QLabel* page_size_label_;
    QLabel* connection_status_label_;
    
    // Firmware file picker (center top)
    QGroupBox* firmware_group_;
    QVBoxLayout* firmware_layout_;
    QHBoxLayout* firmware_file_layout_;
    QLabel* firmware_file_label_;
    QPushButton* browse_firmware_button_;
    QPushButton* clear_firmware_button_;
    QLabel* firmware_info_label_;
    
    // Control buttons (center middle)
    QGroupBox* control_group_;
    QHBoxLayout* control_layout_;
    QPushButton* start_button_;
    QPushButton* stop_button_;
    QPushButton* flash_button_;
    QPushButton* verify_button_;
    QPushButton* erase_button_;
    
    // Progress section (center bottom)
    QGroupBox* progress_group_;
    QVBoxLayout* progress_layout_;
    QProgressBar* progress_bar_;
    QLabel* progress_label_;
    QLabel* progress_details_label_;
    
    // Log console (bottom)
    QGroupBox* log_group_;
    QVBoxLayout* log_layout_;
    QTextEdit* log_console_;
    QHBoxLayout* log_controls_layout_;
    QPushButton* clear_log_button_;
    QPushButton* save_log_button_;
    QLabel* log_level_label_;
    QComboBox* log_level_combo_;
    
    // Menu bar
    QMenuBar* menu_bar_;
    QMenu* file_menu_;
    QMenu* device_menu_;
    QMenu* view_menu_;
    QMenu* tools_menu_;
    QMenu* help_menu_;
    
    // Actions
    QAction* open_firmware_action_;
    QAction* save_log_action_;
    QAction* exit_action_;
    QAction* refresh_devices_action_;
    QAction* connect_action_;
    QAction* disconnect_action_;
    QAction* toggle_theme_action_;
    QAction* preferences_action_;
    QAction* about_action_;
    
    // Tool bar
    QToolBar* main_toolbar_;
    
    // Status bar components
    QLabel* status_label_;
    QLabel* device_status_label_;
    QLabel* theme_label_;
    
    // Timers
    QTimer* ui_update_timer_;
    QTimer* device_scan_timer_;
    
    // Settings and state
    QSettings* settings_;
    QString selected_firmware_file_;
    bool is_dark_theme_;
    bool flash_operation_running_;
    SamFlash::DeviceInfo current_device_;
};

#endif // MAIN_WINDOW_H
