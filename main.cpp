#include <QApplication>
#include "ScreenshotTool.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Screenshot Tool");
    app.setQuitOnLastWindowClosed(false); // Keep running even when all windows are closed
    
    ScreenshotTool tool;
    // Don't start capture immediately, just initialize the tool
    
    return app.exec();
} 
