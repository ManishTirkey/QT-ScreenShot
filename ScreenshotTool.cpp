#include "ScreenshotTool.h"

ScreenshotTool::ScreenshotTool(QWidget *parent)
    : QWidget(parent), isCapturing(false), hasCapture(false)
{
    // Set window properties for full-screen overlay
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);
}

ScreenshotTool::~ScreenshotTool()
{
}

void ScreenshotTool::startCapture()
{
    // Capture the entire screen
    captureFullScreen();
    
    // Reset state
    isCapturing = false;
    hasCapture = false;
    startPos = QPoint();
    endPos = QPoint();
    
    // Show the tool fullscreen
    showFullScreen();
}

void ScreenshotTool::captureFullScreen()
{
    // Get primary screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return;
    
    // Capture the entire screen
    fullScreenPixmap = screen->grabWindow(0);
}

void ScreenshotTool::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    
    // Draw the screenshot as background
    painter.drawPixmap(0, 0, fullScreenPixmap);
    
    // If we're in the process of selecting an area
    if (isCapturing && !startPos.isNull() && !endPos.isNull()) {
        QRect selectedRect = QRect(startPos, endPos).normalized();
        
        // Create semi-transparent dark overlay for the entire screen
        painter.fillRect(rect(), QColor(0, 0, 0, 100));
        
        // Clear the selected area to show it normally
        painter.drawPixmap(selectedRect, fullScreenPixmap, selectedRect);
        
        // Draw border around selected area
        QPen pen(QColor(0, 174, 255), 2);
        painter.setPen(pen);
        painter.drawRect(selectedRect);
        
        // Display dimensions
        QString dimensions = QString("%1 x %2").arg(selectedRect.width()).arg(selectedRect.height());
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(10);
        painter.setFont(font);
        
        // Create a background for the text
        QRect textRect = painter.fontMetrics().boundingRect(dimensions);
        textRect.adjust(-5, -5, 5, 5);
        
        // Position the text near the selection
        textRect.moveTopLeft(QPoint(selectedRect.right() - textRect.width(), 
                                    selectedRect.bottom() + 5));
        
        // Draw text background
        painter.fillRect(textRect, QColor(255, 255, 255, 200));
        
        // Draw text
        painter.setPen(Qt::black);
        painter.drawText(textRect, Qt::AlignCenter, dimensions);
    }
    
    // If we have completed a capture
    if (hasCapture) {
        // Darken the screen
        painter.fillRect(rect(), QColor(0, 0, 0, 160));
        
        // Show message
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(12);
        painter.setFont(font);
        
        QString message = "Press 'S' to save, 'ESC' to cancel, or 'N' for new selection";
        QRect textRect = painter.fontMetrics().boundingRect(message);
        textRect.adjust(-10, -10, 10, 10);
        textRect.moveCenter(rect().center());
        
        painter.fillRect(textRect, QColor(255, 255, 255, 220));
        painter.setPen(Qt::black);
        painter.drawText(textRect, Qt::AlignCenter, message);
    }
}

void ScreenshotTool::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !hasCapture) {
        startPos = event->pos();
        endPos = startPos;
        isCapturing = true;
        update();
    }
}

void ScreenshotTool::mouseMoveEvent(QMouseEvent *event)
{
    if (isCapturing) {
        endPos = event->pos();
        update();
    }
}

void ScreenshotTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isCapturing) {
        endPos = event->pos();
        isCapturing = false;
        
        // Ensure we have a valid selection
        if (QRect(startPos, endPos).normalized().width() > 10 && 
            QRect(startPos, endPos).normalized().height() > 10) {
            
            QRect captureRect = QRect(startPos, endPos).normalized();
            capturedPixmap = fullScreenPixmap.copy(captureRect);
            hasCapture = true;
        }
        
        update();
    }
}

void ScreenshotTool::keyPressEvent(QKeyEvent *event)
{
    // ESC key to cancel
    if (event->key() == Qt::Key_Escape) {
        close();
    }
    // S key to save
    else if (event->key() == Qt::Key_S && hasCapture) {
        saveScreenshot();
    }
    // N key for new selection
    else if (event->key() == Qt::Key_N && hasCapture) {
        hasCapture = false;
        startPos = QPoint();
        endPos = QPoint();
        update();
    }
}

void ScreenshotTool::saveScreenshot()
{
    if (!hasCapture) return;
    
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Screenshot"),
                                                   generateFileName(),
                                                   tr("Images (*.png *.jpg)"));
    
    if (!fileName.isEmpty()) {
        capturedPixmap.save(fileName);
        close();
    }
}

QString ScreenshotTool::generateFileName()
{
    QString documentsPath = QDir::homePath() + "/Pictures/Screenshots/";
    QDir().mkpath(documentsPath);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    return documentsPath + "Screenshot_" + timestamp + ".png";
} 