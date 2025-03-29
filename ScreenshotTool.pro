QT += core gui widgets

TARGET = ScreenshotTool
TEMPLATE = app

SOURCES += \
    main.cpp \
    ScreenshotTool.cpp \
    Logger.cpp

HEADERS += \
    ScreenshotTool.h \
    Logger.h

CONFIG += c++11

# Remove QHotkey library reference
# include(QHotkey/qhotkey.pri)

# Resources for icons
RESOURCES += \
    resources.qrc

# Application icon
win32:RC_ICONS += icons/app_icon.ico
macx:ICON = icons/app_icon.icns

# Define version information
VERSION = 1.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# Installation
target.path = /usr/local/bin
INSTALLS += target

# Desktop file for Linux
unix:!macx {
    desktop.path = /usr/share/applications
    desktop.files += ScreenshotTool.desktop
    INSTALLS += desktop
    
    icon.path = /usr/share/icons/hicolor/256x256/apps
    icon.files += icons/app_icon.png
    INSTALLS += icon
}

# Add this line if you're using Qt 6
# QT += core5compat