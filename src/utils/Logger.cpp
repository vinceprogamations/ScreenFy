#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "Logger.h"
#include <windows.h>
#include <dxgi.h>
#include <shlobj.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

#pragma comment(lib, "dxgi.lib")

Logger& Logger::Get() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (m_fileHandle) {
        CloseHandle(reinterpret_cast<HANDLE>(m_fileHandle));
        m_fileHandle = nullptr;
    }
}

void Logger::Initialize(const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return;

    if (logFilePath.empty()) {
        char appData[MAX_PATH];
        if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData) == S_OK) {
            std::filesystem::path dir = std::filesystem::path(appData) / "ScreenShare4K" / "logs";
            std::filesystem::create_directories(dir);
            m_logFilePath = (dir / "screenfy.log").string();
        } else {
            std::filesystem::create_directories("logs");
            m_logFilePath = "logs/screenfy.log";
        }
    } else {
        m_logFilePath = logFilePath;
    }

    HANDLE hFile = CreateFileA(
        m_logFilePath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile != INVALID_HANDLE_VALUE) {
        m_fileHandle = reinterpret_cast<void*>(hFile);
    }

    m_initialized = true;

    // Log inicial
    std::string header = "=== SCREENFY INICIALIZADO - SESSAO DE DIAGNOSTICO ===\r\n";
    if (m_fileHandle) {
        DWORD written = 0;
        WriteFile(reinterpret_cast<HANDLE>(m_fileHandle), header.c_str(), static_cast<DWORD>(header.size()), &written, nullptr);
    }
    OutputDebugStringA(header.c_str());
}

const char* Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
    }
    return "UNKNOWN";
}

void Logger::Log(LogLevel level, const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tmNow;
    localtime_s(&tmNow, &timeT);

    std::ostringstream ss;
    ss << std::put_time(&tmNow, "%H:%M:%S") << "." 
       << std::setfill('0') << std::setw(3) << ms.count()
       << " [" << LevelToString(level) << "] [" << tag << "] " << message;

    std::string line = ss.str();
    std::string lineWithEol = line + "\r\n";

    // 1. Buffer em memória para visualização na UI
    LogEntry entry;
    entry.level = level;
    entry.tag = tag;
    entry.message = message;
    entry.timestamp = now;

    if (m_memoryLogs.size() >= m_maxMemoryLogs) {
        m_memoryLogs.pop_front();
    }
    m_memoryLogs.push_back(entry);

    // 2. Ficheiro de Log persistente
    if (m_fileHandle) {
        DWORD written = 0;
        WriteFile(reinterpret_cast<HANDLE>(m_fileHandle), lineWithEol.c_str(), static_cast<DWORD>(lineWithEol.size()), &written, nullptr);
        FlushFileBuffers(reinterpret_cast<HANDLE>(m_fileHandle));
    }

    // 3. Debug Output do Windows
    OutputDebugStringA(lineWithEol.c_str());

    // 4. Console stdout se aplicável
    std::cout << line << std::endl;
}

void Logger::Debug(const std::string& tag, const std::string& message) {
    Log(LogLevel::Debug, tag, message);
}

void Logger::Info(const std::string& tag, const std::string& message) {
    Log(LogLevel::Info, tag, message);
}

void Logger::Warn(const std::string& tag, const std::string& message) {
    Log(LogLevel::Warn, tag, message);
}

void Logger::Error(const std::string& tag, const std::string& message) {
    Log(LogLevel::Error, tag, message);
}

void Logger::Fatal(const std::string& tag, const std::string& message) {
    Log(LogLevel::Fatal, tag, message);
}

std::vector<LogEntry> Logger::GetRecentLogs(size_t maxCount) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (maxCount >= m_memoryLogs.size()) {
        return std::vector<LogEntry>(m_memoryLogs.begin(), m_memoryLogs.end());
    }
    auto startIt = m_memoryLogs.end() - maxCount;
    return std::vector<LogEntry>(startIt, m_memoryLogs.end());
}

void Logger::ClearMemoryLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_memoryLogs.clear();
}

std::string Logger::GetLogFilePath() const {
    return m_logFilePath;
}

std::string Logger::GenerateDiagnosticReport() {
    std::ostringstream ss;
    ss << "=== SCREENFY - RELATÓRIO DE DIAGNÓSTICO DO SISTEMA ===\r\n";

    // 1. Data e Hora
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tmNow;
    localtime_s(&tmNow, &timeT);
    ss << "Data/Hora: " << std::put_time(&tmNow, "%Y-%m-%d %H:%M:%S") << "\r\n";

    // 2. Informações de Memória
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        DWORDLONG totalRAM = memInfo.ullTotalPhys / (1024 * 1024);
        DWORDLONG availRAM = memInfo.ullAvailPhys / (1024 * 1024);
        ss << "RAM Total: " << totalRAM << " MB (Livre: " << availRAM << " MB)\r\n";
    }

    // 3. Monitores e Resoluções
    int monCount = GetSystemMetrics(SM_CMONITORS);
    int primaryW = GetSystemMetrics(SM_CXSCREEN);
    int primaryH = GetSystemMetrics(SM_CYSCREEN);
    ss << "Monitores Detectados: " << monCount << " (Principal: " << primaryW << "x" << primaryH << ")\r\n";

    // 4. Adaptadores de Vídeo / GPUs via DXGI
    IDXGIFactory* pFactory = nullptr;
    if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&pFactory)))) {
        UINT i = 0;
        IDXGIAdapter* pAdapter = nullptr;
        while (pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC desc;
            if (SUCCEEDED(pAdapter->GetDesc(&desc))) {
                char descA[128];
                WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, descA, sizeof(descA), nullptr, nullptr);
                size_t vramMB = desc.DedicatedVideoMemory / (1024 * 1024);
                ss << "GPU [" << i << "]: " << descA << " (VRAM: " << vramMB << " MB)\r\n";
            }
            pAdapter->Release();
            i++;
        }
        pFactory->Release();
    }

    ss << "\r\n=== ÚLTIMOS REGISTOS DO SISTEMA ===\r\n";
    auto logs = GetRecentLogs(50);
    for (const auto& l : logs) {
        auto t = std::chrono::system_clock::to_time_t(l.timestamp);
        std::tm tmLog;
        localtime_s(&tmLog, &t);
        ss << std::put_time(&tmLog, "%H:%M:%S") << " [" << LevelToString(l.level) << "] [" << l.tag << "] " << l.message << "\r\n";
    }

    return ss.str();
}
