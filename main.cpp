#include <QApplication>
#include "ScreenshotTool.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    ScreenshotTool tool;
    tool.startCapture();
    
    return app.exec();
} 