#include "ScreenshotTool.h"
#include "Logger.h"
#include <QGuiApplication>
#include <QWindow>
#include <QShortcut>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QStyle>
#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDialog>
#include <QTimer>

// Implementation of GlobalHotkeyFilter
#ifdef Q_OS_WIN
GlobalHotkeyFilter::GlobalHotkeyFilter(QObject* target, const char* slot)
    : m_target(target), m_slot(slot)
{
    registerHotKeys();
}

GlobalHotkeyFilter::~GlobalHotkeyFilter()
{
    unregisterHotKeys();
}

bool GlobalHotkeyFilter::registerHotKeys()
{
    // Register Print Screen key (clipboard only)
    bool success1 = RegisterHotKey(NULL, HOTKEY_ID_PRINT, 0, VK_SNAPSHOT);

    // Register Ctrl + Print Screen (clipboard + save)
    bool success2 = RegisterHotKey(NULL, HOTKEY_ID_CTRL_PRINT, MOD_CONTROL, VK_SNAPSHOT);

    // Register Shift + Print Screen (clipboard + preview)
    bool success3 = RegisterHotKey(NULL, HOTKEY_ID_SHIFT_PRINT, MOD_SHIFT, VK_SNAPSHOT);

    return success1 && success2 && success3;
}

bool GlobalHotkeyFilter::unregisterHotKeys()
{
    bool success1 = UnregisterHotKey(NULL, HOTKEY_ID_PRINT);
    bool success2 = UnregisterHotKey(NULL, HOTKEY_ID_CTRL_PRINT);
    bool success3 = UnregisterHotKey(NULL, HOTKEY_ID_SHIFT_PRINT);

    return success1 && success2 && success3;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool GlobalHotkeyFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#else
bool GlobalHotkeyFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG* msg = static_cast<MSG*>(message);

    if (msg->message == WM_HOTKEY) {
        switch (msg->wParam) {
        case HOTKEY_ID_PRINT:
            // Print Screen only - clipboard
            QMetaObject::invokeMethod(m_target, "startCapture");
            return true;

        case HOTKEY_ID_CTRL_PRINT:
            // Ctrl + Print Screen - clipboard + save
            QMetaObject::invokeMethod(m_target, "startCaptureAndSave");
            return true;

        case HOTKEY_ID_SHIFT_PRINT:
            // Shift + Print Screen - clipboard + preview
            QMetaObject::invokeMethod(m_target, "startCaptureAndPreview");
            return true;
        }
    }

    return false;
}
#else
// Placeholder implementation for non-Windows platforms
GlobalHotkeyFilter::GlobalHotkeyFilter(QObject* target, const char* slot)
    : m_target(target), m_slot(slot)
{
}

GlobalHotkeyFilter::~GlobalHotkeyFilter()
{
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool GlobalHotkeyFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
#else
bool GlobalHotkeyFilter::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
#endif
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}
#endif

// Implementation of PreviewWindow class
PreviewWindow::PreviewWindow(QWidget *parent) : QLabel(parent)
{
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    setAlignment(Qt::AlignCenter);
    setWindowTitle("Screenshot Preview");
    setFocusPolicy(Qt::StrongFocus); // Needed to receive key events
}

void PreviewWindow::keyPressEvent(QKeyEvent *event)
{
    // Ctrl+S to save
    if (event->key() == Qt::Key_S && event->modifiers() == Qt::ControlModifier) {
        emit saveRequested();
        event->accept();
    }
    // Ctrl+Shift+S to copy to clipboard
    // else if (event->key() == Qt::Key_S &&
    //          event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) {
    //     emit copyToClipboardRequested();
    //     event->accept();
    // }
    else {
        QLabel::keyPressEvent(event);
    }
}

ScreenshotTool::ScreenshotTool(QWidget *parent)
    : QWidget(parent),
      isCapturing(false),
      hasCapture(false),
      previewWindow(nullptr),
      trayIcon(nullptr),
      trayMenu(nullptr),
      settings("ManishTirkey", "CatchAndHold"),
      hotkeyFilter(nullptr),
      autoStartEnabled(false),
      currentCaptureMode(ClipboardOnly)
{
    // Set window properties for full-screen overlay
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setCursor(Qt::CrossCursor);

    // Initialize the preview window
    previewWindow = new PreviewWindow();

    // Connect signals from preview window
    connect(previewWindow, &PreviewWindow::saveRequested,
            this, &ScreenshotTool::saveScreenshot);
    // connect(previewWindow, &PreviewWindow::copyToClipboardRequested,
    //         this, &ScreenshotTool::copyToClipboard);

    // Setup tray icon and shortcuts
    setupTrayIcon();
    setupShortcuts();

    // Load settings
    loadSettings();

    // Setup global hotkey filter
    hotkeyFilter = new GlobalHotkeyFilter(this, "startCapture");
    QGuiApplication::instance()->installNativeEventFilter(hotkeyFilter);

    // Load auto-start setting
    autoStartEnabled = settings.value("AutoStart", false).toBool();
    setAutoStart(autoStartEnabled);

    // Initialize cursor tracker
    cursorTracker = new QTimer(this);
    connect(cursorTracker, &QTimer::timeout, this, &ScreenshotTool::updateCursorPosition);
}

ScreenshotTool::~ScreenshotTool()
{
    delete previewWindow;
    delete trayIcon;
    delete trayMenu;

    // Save settings
    saveSettings();

    // Remove and delete the hotkey filter
    if (hotkeyFilter) {
        QGuiApplication::instance()->removeNativeEventFilter(hotkeyFilter);
        delete hotkeyFilter;
    }
}

void ScreenshotTool::setupTrayIcon()
{
    // Create tray icon with the application icon
    #ifdef Q_OS_WIN
    // On Windows, use the .ico file
    trayIcon = new QSystemTrayIcon(QIcon(":/icons/icons/app_icon.ico"), this);
    #else
    // On other platforms, use the .png file
    trayIcon = new QSystemTrayIcon(QIcon(":/icons/icons/app_icon.png"), this);
    #endif

    // If icon loading fails, fall back to a system icon
    if (trayIcon->icon().isNull()) {
        trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_DesktopIcon));
    }

    // Create tray menu
    trayMenu = new QMenu();

    // Add actions to the menu
    QAction* captureAction = new QAction("Capture Screenshot", this);
    captureAction->setShortcut(QKeySequence(Qt::Key_Print)); // Set Print Screen as shortcut
    captureAction->setShortcutContext(Qt::ApplicationShortcut);
    connect(captureAction, &QAction::triggered, this, &ScreenshotTool::startCapture);

    QAction* shortcutsAction = new QAction("Keyboard Shortcuts", this);
    connect(shortcutsAction, &QAction::triggered, this, &ScreenshotTool::showShortcutsDialog);

    QAction* settingsAction = new QAction("Settings", this);
    connect(settingsAction, &QAction::triggered, this, &ScreenshotTool::showSettingsDialog);

    QAction* aboutAction = new QAction("About", this);
    connect(aboutAction, &QAction::triggered, this, &ScreenshotTool::showAboutDialog);

    QAction* autoStartAction = new QAction("Start with Windows", this);
    autoStartAction->setCheckable(true);
    autoStartAction->setChecked(isAutoStartEnabled());
    connect(autoStartAction, &QAction::triggered, this, [this](bool checked) {
        setAutoStart(checked);
    });

    QAction* quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, this, &ScreenshotTool::quitApplication);

    // Add actions to menu
    trayMenu->addAction(captureAction);
    trayMenu->addSeparator();
    trayMenu->addAction(shortcutsAction);
    trayMenu->addAction(settingsAction);
    trayMenu->addAction(aboutAction);
    trayMenu->addSeparator();
    trayMenu->addAction(autoStartAction);
    trayMenu->addAction(quitAction);

    // Set the menu for the tray icon
    trayIcon->setContextMenu(trayMenu);

    // Show the tray icon
    trayIcon->show();

    // Show a welcome message
    trayIcon->showMessage("Screenshot Tool",
                         "Screenshot Tool is running in the system tray.\n"
                         "Click the tray icon or press Print Screen to capture.",
                         QSystemTrayIcon::Information,
                         3000);

    // Connect tray icon activation signal
    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            startCapture();
        }
    });

    // Add the capture action to the application to enable the shortcut
    this->addAction(captureAction);
}

void ScreenshotTool::setupShortcuts()
{
    // We're now using the native event filter for global hotkeys,
    // so this method can be simplified

    // We'll keep the QShortcut for when the app has focus, as a backup
    QShortcut* printScreenShortcut = new QShortcut(QKeySequence(Qt::Key_Print), this);
    connect(printScreenShortcut, &QShortcut::activated, this, &ScreenshotTool::startCapture);
}

void ScreenshotTool::loadSettings()
{
    // Load any saved settings
    // For example, you could load custom hotkey settings here
}

void ScreenshotTool::saveSettings()
{
    // Save current settings
    // For example, you could save custom hotkey settings here
}

void ScreenshotTool::startCapture()
{
    currentCaptureMode = ClipboardOnly;
    performCapture();
}

void ScreenshotTool::startCaptureAndSave()
{
    currentCaptureMode = ClipboardAndSave;
    performCapture();
}

void ScreenshotTool::startCaptureAndPreview()
{
    currentCaptureMode = ClipboardAndPreview;
    performCapture();
}

void ScreenshotTool::performCapture()
{
    // Capture the entire screen
    captureFullScreen();

    // Reset state
    hasCapture = false;
    startPos = QPoint();
    endPos = QPoint();

    // Start showing guides and zoom
    isCapturing = true;

    // Map global cursor position to widget coordinates
    currentMousePos = mapFromGlobal(QCursor::pos());

    // Show the tool fullscreen
    showFullScreen();
    setCursor(Qt::CrossCursor);

    // Start tracking cursor
    cursorTracker->start(16); // 60 FPS update rate

    update();
}

void ScreenshotTool::handleCaptureCompletion()
{
    if (!hasCapture) return;

    // Always copy to clipboard first
    copyToClipboard();

    switch (currentCaptureMode) {
    case ClipboardOnly:
        // Already copied to clipboard, nothing more to do
        trayIcon->showMessage("Screenshot Tool",
                              "Screenshot copied to clipboard",
                              QSystemTrayIcon::Information, 2000);
        break;

    case ClipboardAndSave:
        // Copy to clipboard and save file
        saveScreenshot();
        break;

    case ClipboardAndPreview:
        // Copy to clipboard and show preview
        showCapturedImage();
        break;
    }
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

    if (isCapturing) {
        // Create semi-transparent dark overlay
        painter.fillRect(rect(), QColor(0, 0, 0, 100));

        // Draw guide lines first (so they appear under selection)
        QPen dottedPen(QColor(0, 255, 0), 1, Qt::DashLine);
        dottedPen.setDashPattern({5, 5});
        painter.setPen(dottedPen);

        // Draw horizontal and vertical guide lines
        painter.drawLine(0, currentMousePos.y(), width(), currentMousePos.y());
        painter.drawLine(currentMousePos.x(), 0, currentMousePos.x(), height());

        // Draw zoom circle
        drawZoomArea(painter);

        // If selection has started, draw the selection area
        if (!startPos.isNull() && !endPos.isNull()) {
            QRect selectedRect = QRect(startPos, endPos).normalized();

            // Clear the selected area
            painter.drawPixmap(selectedRect, fullScreenPixmap, selectedRect);

            // Draw green highlight
            painter.fillRect(selectedRect, QColor(0, 255, 0, 30));

            // Draw selection dimensions
            QString dimensions = QString("%1 × %2").arg(selectedRect.width()).arg(selectedRect.height());
            QFont font = painter.font();
            font.setBold(true);
            font.setPointSize(10);
            painter.setFont(font);

            QRect textRect = painter.fontMetrics().boundingRect(dimensions);
            textRect.adjust(-5, -5, 5, 5);
            textRect.moveCenter(selectedRect.center());

            painter.fillRect(textRect, QColor(0, 0, 0, 160));
            painter.setPen(Qt::white);
            painter.drawText(textRect, Qt::AlignCenter, dimensions);
        }
    }
}

void ScreenshotTool::drawZoomArea(QPainter& painter)
{
    if (currentMousePos.isNull()) return;

    // Zoom circle parameters
    int zoomRadius = 80;
    int zoomFactor = 6;

    // Always position zoom circle at southeast of cursor (bottom-right)
    QPoint zoomCenter = currentMousePos + QPoint(zoomRadius + 20, zoomRadius + 20);

    // Adjust position if near screen edges
    // Right edge
    if (zoomCenter.x() + zoomRadius > width()) {
        // Move to left of cursor
        zoomCenter.setX(currentMousePos.x() - (zoomRadius + 20));
    }

    // Bottom edge
    if (zoomCenter.y() + zoomRadius > height()) {
        // Move above cursor
        zoomCenter.setY(currentMousePos.y() - (zoomRadius + 20));
    }

    // Left edge (if moved to left and still out of bounds)
    if (zoomCenter.x() - zoomRadius < 0) {
        zoomCenter.setX(zoomRadius + 10);
    }

    // Top edge (if moved up and still out of bounds)
    if (zoomCenter.y() - zoomRadius < 0) {
        zoomCenter.setY(zoomRadius + 10);
    }

    // If we have a selection, ensure zoom circle doesn't overlap with it
    if (!startPos.isNull() && !endPos.isNull()) {
        QRect selectedRect = QRect(startPos, endPos).normalized();
        QRect zoomRect(zoomCenter.x() - zoomRadius, zoomCenter.y() - zoomRadius,
                      zoomRadius * 2, zoomRadius * 2);

        // If zoom circle intersects with selection
        if (selectedRect.intersects(zoomRect)) {
            // Try to move zoom circle to the right of selection
            zoomCenter.setX(selectedRect.right() + zoomRadius + 10);

            // If that puts it off screen, try left side
            if (zoomCenter.x() + zoomRadius > width()) {
                zoomCenter.setX(selectedRect.left() - zoomRadius - 10);
            }

            // If still no good, try above selection
            if (zoomCenter.x() - zoomRadius < 0) {
                zoomCenter.setX(currentMousePos.x() + zoomRadius + 20);
                zoomCenter.setY(selectedRect.top() - zoomRadius - 10);
            }

            // If that's off screen, try below selection
            if (zoomCenter.y() - zoomRadius < 0) {
                zoomCenter.setY(selectedRect.bottom() + zoomRadius + 10);
            }

            // Final fallback: place in nearest corner of screen
            if (zoomCenter.y() + zoomRadius > height()) {
                zoomCenter = QPoint(
                    (currentMousePos.x() < width()/2) ? zoomRadius + 10 : width() - zoomRadius - 10,
                    (currentMousePos.y() < height()/2) ? zoomRadius + 10 : height() - zoomRadius - 10
                );
            }
        }
    }

    // Create zoom circle clipping path
    QPainterPath path;
    path.addEllipse(zoomCenter, zoomRadius, zoomRadius);
    painter.setClipPath(path);

    // Calculate zoom area
    QRect sourceRect(
        currentMousePos.x() - zoomRadius / zoomFactor,
        currentMousePos.y() - zoomRadius / zoomFactor,
        zoomRadius * 2 / zoomFactor,
        zoomRadius * 2 / zoomFactor
    );

    QRect destRect(
        zoomCenter.x() - zoomRadius,
        zoomCenter.y() - zoomRadius,
        zoomRadius * 2,
        zoomRadius * 2
    );

    // Draw zoomed content
    painter.drawPixmap(destRect, fullScreenPixmap, sourceRect);

    // Reset clipping
    painter.setClipping(false);

    // Draw borders
    // Outer dark border for contrast
    QPen pen(QColor(0, 0, 0, 160), 3);
    painter.setPen(pen);
    painter.drawEllipse(zoomCenter, zoomRadius + 1, zoomRadius + 1);

    // Inner white border
    pen.setColor(Qt::white);
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawEllipse(zoomCenter, zoomRadius, zoomRadius);

    // Draw crosshair
    pen.setColor(Qt::red);
    pen.setWidth(1);
    painter.setPen(pen);

    int crossSize = 10;
    painter.drawLine(zoomCenter.x() - crossSize, zoomCenter.y(),
                    zoomCenter.x() + crossSize, zoomCenter.y());
    painter.drawLine(zoomCenter.x(), zoomCenter.y() - crossSize,
                    zoomCenter.x(), zoomCenter.y() + crossSize);

    // Draw coordinates
    QString coords = QString("(%1, %2)").arg(currentMousePos.x()).arg(currentMousePos.y());
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);

    QRect textRect = painter.fontMetrics().boundingRect(coords);
    textRect.adjust(-4, -4, 4, 4);
    textRect.moveCenter(zoomCenter + QPoint(0, zoomRadius + 15));

    painter.fillRect(textRect, QColor(0, 0, 0, 160));
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, coords);
}

void ScreenshotTool::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isCapturing && startPos.isNull()) {
        // Start selection only when left clicking and no selection has started
        startPos = event->pos();
        endPos = startPos;
        update();
    }
}

void ScreenshotTool::mouseMoveEvent(QMouseEvent *event)
{
    // Update cursor position
    currentMousePos = event->pos();

    // Update selection end point if we're dragging
    if (!startPos.isNull() && event->buttons() & Qt::LeftButton) {
        endPos = event->pos();
    }

    update();
}

void ScreenshotTool::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && isCapturing) {
        endPos = event->pos();
        isCapturing = false;

        // Stop cursor tracking
        cursorTracker->stop();

        // Ensure we have a valid selection
        if (QRect(startPos, endPos).normalized().width() > 10 &&
            QRect(startPos, endPos).normalized().height() > 10) {

            QRect captureRect = QRect(startPos, endPos).normalized();
            capturedPixmap = fullScreenPixmap.copy(captureRect);
            hasCapture = true;

            // Hide the overlay immediately for seamless feel
            hide();

            // Now handle the completion (e.g., save dialog for Ctrl+Print)
            handleCaptureCompletion();
        } else {
            // No valid selection, just close
            close();
        }
    }
}


void ScreenshotTool::showCapturedImage()
{
    if (!hasCapture) return;

    // Set the captured image to the preview window
    previewWindow->setPixmap(capturedPixmap);

    // Resize the window to fit the image with some padding
    previewWindow->resize(capturedPixmap.size() + QSize(20, 20));

    // Center the window on screen
    previewWindow->show();
    previewWindow->activateWindow(); // Make sure it gets focus
    previewWindow->setFocus();       // Set keyboard focus
}

void ScreenshotTool::copyToClipboard()
{
    if (!hasCapture) return;

    // Copy the captured image to clipboard
    QGuiApplication::clipboard()->setPixmap(capturedPixmap);
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
}

void ScreenshotTool::saveScreenshot()
{
    if (!hasCapture) return;

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Screenshot"),
                                                   generateFileName(),
                                                   tr("Images (*.png *.jpg)"));

    if (!fileName.isEmpty()) {
        capturedPixmap.save(fileName);
    }
}

QString ScreenshotTool::generateFileName()
{
    QString documentsPath = QDir::homePath() + "/Pictures/Screenshots/";
    QDir().mkpath(documentsPath);

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    return documentsPath + "Screenshot_" + timestamp + ".png";
}

void ScreenshotTool::showShortcutsDialog()
{
    QMessageBox::information(nullptr, "Keyboard Shortcuts",
                             "Global Shortcuts:\n"
                             "- Print Screen: Capture screenshot and copy to clipboard\n"
                             "- Ctrl + Print Screen: Capture, copy to clipboard, and save file\n"
                             "- Shift + Print Screen: Capture, copy to clipboard, and show preview\n\n"
                             "During Capture:\n"
                             "- ESC: Cancel capture\n\n"
                             "In Preview Window:\n"
                             "- Ctrl+S: Save screenshot");
}


void ScreenshotTool::showAboutDialog()
{
    QMessageBox::about(nullptr, "About Screenshot Tool",
        "Screenshot Tool v1.0\n\n"
        "A simple yet powerful screenshot tool with area selection.\n\n"
        "Features:\n"
        "- Area selection with real-time dimensions\n"
        "- Magnifying glass for precision\n"
        "- Quick save and clipboard options\n"
        "- Global hotkey support");
}

void ScreenshotTool::showSettingsDialog()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Settings");

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    // Auto-start checkbox
    QCheckBox* autoStartCheck = new QCheckBox("Start with Windows", dialog);
    autoStartCheck->setChecked(isAutoStartEnabled());
    layout->addWidget(autoStartCheck);

    // Add other settings here...

    // Buttons
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        dialog
    );
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    if (dialog->exec() == QDialog::Accepted) {
        setAutoStart(autoStartCheck->isChecked());
        // Save other settings...
    }

    delete dialog;
}

void ScreenshotTool::quitApplication()
{
    QApplication::quit();
}

void ScreenshotTool::closeEvent(QCloseEvent *event)
{
    // Stop cursor tracking when closing
    cursorTracker->stop();

    // Hide instead of close to keep running in tray
    if (trayIcon && trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void ScreenshotTool::setAutoStart(bool enable)
{
    if (enable == autoStartEnabled) return;

    #ifdef Q_OS_WIN
        QSettings bootUpSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                               QSettings::NativeFormat);

        if (enable) {
            bootUpSettings.setValue("CatchAndHold", getApplicationPath());
            qDebug() << "Auto-start enabled";
        } else {
            bootUpSettings.remove("CatchAndHold");
            qDebug() << "Auto-start disabled";
        }
    #else
        QString startupDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
                           + QLatin1String("/Startup/");
        QString linkName = startupDir + "catchandhold.desktop";

        if (enable) {
            QFile::link(getApplicationPath(), linkName);
            qDebug() << "Auto-start enabled";
        } else {
            QFile::remove(linkName);
            qDebug() << "Auto-start disabled";
        }
    #endif

    autoStartEnabled = enable;
    settings.setValue("AutoStart", enable);

    // Use qDebug instead of LOG_INFO until Logger is properly set up
    qDebug() << "Auto-start" << (enable ? "enabled" : "disabled");
}

bool ScreenshotTool::isAutoStartEnabled() const
{
    return autoStartEnabled;
}

QString ScreenshotTool::getApplicationPath() const
{
    return QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
}

void ScreenshotTool::updateCursorPosition()
{
    if (isCapturing) {
        // Map global cursor position to widget coordinates
        QPoint newPos = mapFromGlobal(QCursor::pos());
        if (newPos != currentMousePos) {
            currentMousePos = newPos;
            update(); // Redraw only if position changed
        }
    }
}
