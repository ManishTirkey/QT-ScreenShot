#include "Logger.h"
#include <QStandardPaths>
#include <QDebug>

Logger* Logger::m_instance = nullptr;
QMutex Logger::m_mutex;

Logger* Logger::instance()
{
    if (!m_instance) {
        QMutexLocker locker(&m_mutex);
        if (!m_instance) {
            m_instance = new Logger();
        }
    }
    return m_instance;
}

Logger::Logger(QObject *parent) : QObject(parent),
    m_logLevel(Info),
    m_logToFile(true),
    m_logToConsole(true)
{
    openLogFile();
}

Logger::~Logger()
{
    closeLogFile();
}

void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
}

void Logger::setLogToFile(bool enabled)
{
    m_logToFile = enabled;
    if (enabled && !m_logFile.isOpen()) {
        openLogFile();
    } else if (!enabled && m_logFile.isOpen()) {
        closeLogFile();
    }
}

void Logger::setLogToConsole(bool enabled)
{
    m_logToConsole = enabled;
}

void Logger::log(LogLevel level, const QString& message)
{
    if (level < m_logLevel)
        return;
        
    QString formattedMessage = formatMessage(level, message);
    
    if (m_logToConsole) {
        qDebug() << formattedMessage;
    }
    
    if (m_logToFile && m_logFile.isOpen()) {
        QMutexLocker locker(&m_mutex);
        m_logStream << formattedMessage << Qt::endl;
        m_logStream.flush();
    }
}

void Logger::debug(const QString& message)
{
    log(Debug, message);
}

void Logger::info(const QString& message)
{
    log(Info, message);
}

void Logger::warning(const QString& message)
{
    log(Warning, message);
}

void Logger::error(const QString& message)
{
    log(Error, message);
}

void Logger::fatal(const QString& message)
{
    log(Fatal, message);
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO";
        case Warning: return "WARNING";
        case Error:   return "ERROR";
        case Fatal:   return "FATAL";
        default:      return "UNKNOWN";
    }
}

QString Logger::formatMessage(LogLevel level, const QString& message)
{
    return QString("[%1] [%2] %3")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"))
        .arg(levelToString(level))
        .arg(message);
}

void Logger::openLogFile()
{
    if (m_logFile.isOpen())
        return;
        
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(logDir);
    
    QString logFilePath = logDir + "/catchandhold.log";
    m_logFile.setFileName(logFilePath);
    
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logStream.setDevice(&m_logFile);
        info("Log file opened: " + logFilePath);
    } else {
        qDebug() << "Failed to open log file:" << logFilePath;
    }
}

void Logger::closeLogFile()
{
    if (m_logFile.isOpen()) {
        info("Log file closed");
        m_logStream.flush();
        m_logFile.close();
    }
} 