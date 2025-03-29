#include <QApplication>
#include <QPainter>
#include <QPixmap>

void generateIcons()
{
    // Create directories if they don't exist
    QDir().mkpath("icons");
    
    // Sizes for different icon resolutions
    QVector<int> sizes = {16, 32, 48, 64, 128, 256};
    
    for (int size : sizes) {
        QPixmap pixmap(size, size);
        pixmap.fill(Qt::transparent);
        
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        
        // Scale values based on size
        int margin = size / 8;
        int bodySize = size - (margin * 2);
        int lensSize = bodySize / 2;
        int flashWidth = size / 32;
        
        // Background
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(41, 128, 185));
        painter.drawRoundedRect(margin, margin, bodySize, bodySize, size/12, size/12);
        
        // Camera body
        painter.setBrush(QColor(52, 73, 94));
        int bodyMargin = size / 5;
        painter.drawRoundedRect(bodyMargin, bodyMargin + (size/8), 
                              size - (bodyMargin*2), size - (bodyMargin*2), size/16, size/16);
        
        // Lens
        painter.setBrush(QColor(236, 240, 241));
        int lensX = (size - lensSize) / 2;
        int lensY = (size - lensSize) / 2;
        painter.drawEllipse(lensX, lensY, lensSize, lensSize);
        
        // Flash
        painter.setPen(QPen(QColor(241, 196, 15), flashWidth));
        painter.drawLine(size * 0.7, size * 0.2, size * 0.75, size * 0.3);
        
        // Save PNG for each size
        pixmap.save(QString("icons/app_icon_%1.png").arg(size));
        
        // For the main app icon
        if (size == 256) {
            pixmap.save("icons/app_icon.png");
        }
    }
    
    // Create ICO file (Windows)
    QIcon icon;
    for (int size : sizes) {
        icon.addFile(QString("icons/app_icon_%1.png").arg(size));
    }
    
    // Save as ICO
    QFile iconFile("icons/app_icon.ico");
    if (iconFile.open(QIODevice::WriteOnly)) {
        icon.pixmap(256, 256).save(&iconFile, "ICO");
        iconFile.close();
    }
} 