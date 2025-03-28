#ifndef SCREENSHOTTOOL_H
#define SCREENSHOTTOOL_H

#include <QWidget>
#include <QScreen>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>

class ScreenshotTool : public QWidget
{
    Q_OBJECT

public:
    ScreenshotTool(QWidget *parent = nullptr);
    ~ScreenshotTool();

public slots:
    void startCapture();
    void saveScreenshot();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPoint startPos;
    QPoint endPos;
    QPixmap fullScreenPixmap;
    QPixmap capturedPixmap;
    bool isCapturing;
    bool hasCapture;
    
    void captureFullScreen();
    QString generateFileName();
};

#endif // SCREENSHOTTOOL_H 