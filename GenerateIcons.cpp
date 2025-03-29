#include <QApplication>
#include <QPainter>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Create icons directory
    QDir().mkpath("icons");
    
    // Create the main icon (256x256)
    QPixmap pixmap(256, 256);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw camera icon
    painter.setPen(Qt::NoPen);
    
    // Background
    painter.setBrush(QColor(41, 128, 185));  // Nice blue
    painter.drawRoundedRect(32, 32, 192, 192, 20, 20);
    
    // Camera body
    painter.setBrush(QColor(52, 73, 94));    // Dark blue
    painter.drawRoundedRect(48, 64, 160, 128, 16, 16);
    
    // Lens
    painter.setBrush(QColor(236, 240, 241)); // Light gray
    painter.drawEllipse(96, 96, 64, 64);
    
    // Flash
    painter.setPen(QPen(QColor(241, 196, 15), 8)); // Yellow
    painter.drawLine(176, 48, 192, 80);
    
    // Save PNG
    pixmap.save("icons/app_icon.png");
    
    // Save ICO (Windows)
    QIcon icon(pixmap);
    QFile iconFile("icons/app_icon.ico");
    if (iconFile.open(QIODevice::WriteOnly)) {
        pixmap.save(&iconFile, "ICO");
        iconFile.close();
    }
    
    return 0;
} 