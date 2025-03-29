#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QMutex>
#include <QDebug>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    static Logger* instance();
    
    void setLogLevel(LogLevel level);
    void setLogToFile(bool enabled);
    void setLogToConsole(bool enabled);
    
public slots:
    void log(LogLevel level, const QString& message);
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);
    void fatal(const QString& message);

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();
    
    static Logger* m_instance;
    static QMutex m_mutex;
    
    QFile m_logFile;
    QTextStream m_logStream;
    LogLevel m_logLevel;
    bool m_logToFile;
    bool m_logToConsole;
    
    QString levelToString(LogLevel level);
    QString formatMessage(LogLevel level, const QString& message);
    void openLogFile();
    void closeLogFile();
};

// Convenience macros
#define LOG_DEBUG(msg) Logger::instance()->debug(msg)
#define LOG_INFO(msg) Logger::instance()->info(msg)
#define LOG_WARNING(msg) Logger::instance()->warning(msg)
#define LOG_ERROR(msg) Logger::instance()->error(msg)
#define LOG_FATAL(msg) Logger::instance()->fatal(msg)

#endif // LOGGER_H 