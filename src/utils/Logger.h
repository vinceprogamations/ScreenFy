#pragma once
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <chrono>

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error,
    Fatal
};

struct LogEntry {
    LogLevel level;
    std::string tag;
    std::string message;
    std::chrono::system_clock::time_point timestamp;
};

class Logger {
public:
    static Logger& Get();

    void Initialize(const std::string& logFilePath = "");
    void Log(LogLevel level, const std::string& tag, const std::string& message);

    // Métodos utilitários convenientes
    void Debug(const std::string& tag, const std::string& message);
    void Info(const std::string& tag, const std::string& message);
    void Warn(const std::string& tag, const std::string& message);
    void Error(const std::string& tag, const std::string& message);
    void Fatal(const std::string& tag, const std::string& message);

    // Recuperar histórico em memória para UI
    std::vector<LogEntry> GetRecentLogs(size_t maxCount = 500);
    void ClearMemoryLogs();

    // Diagnóstico inteligente do sistema
    std::string GenerateDiagnosticReport();
    std::string GetLogFilePath() const;

    static const char* LevelToString(LogLevel level);

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::mutex m_mutex;
    std::deque<LogEntry> m_memoryLogs;
    size_t m_maxMemoryLogs = 1000;
    std::string m_logFilePath;
    void* m_fileHandle = nullptr;
    bool m_initialized = false;
};

#define LOG_DEBUG(tag, msg) Logger::Get().Debug(tag, msg)
#define LOG_INFO(tag, msg)  Logger::Get().Info(tag, msg)
#define LOG_WARN(tag, msg)  Logger::Get().Warn(tag, msg)
#define LOG_ERROR(tag, msg) Logger::Get().Error(tag, msg)
#define LOG_FATAL(tag, msg) Logger::Get().Fatal(tag, msg)
