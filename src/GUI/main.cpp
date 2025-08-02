#include <QtWidgets/QApplication>
#include "main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("SamFlash Alternative");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SamFlash Alternative Project");
    
    // Create and show main window
    MainWindow window;
    window.show();
    
    return app.exec();
}
