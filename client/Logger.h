#pragma once
//
// Logger.h  (reconstructed parent-level support header)
//
// Provides:
//   * Logger        : log-level enum holder (Logger::LOG_ERROR, LOG_WARNING, LOG_INFO, LOG_DEBUG)
//   * blog(...)      : printf-style variadic global logging function -> stdout
//
// Usage discovered in the client:
//   blog(Logger::LOG_ERROR,   "Missing button script %s", name.c_str());
//   blog(Logger::LOG_INFO,    "DMYST INTERNAL BUILD %d", DYMST_VERSION);
//   blog(Logger::LOG_DEBUG,   "render(); took %dms", ttook);
//   blog(Logger::LOG_WARNING, "Received character creation packet at unexpected time");
//
// Compilable standalone under MSVC x86, C++17.
//
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <string>

class Logger
{
public:
    enum Level
    {
        LOG_ERROR = 0,
        LOG_WARNING,
        LOG_INFO,
        LOG_DEBUG
    };

    static const char* levelName(int level)
    {
        switch (level)
        {
        case LOG_ERROR:   return "ERROR";
        case LOG_WARNING: return "WARN";
        case LOG_INFO:    return "INFO";
        case LOG_DEBUG:   return "DEBUG";
        default:          return "LOG";
        }
    }

    // Singleton accessor (used as Logger::instance().getLogPath()).
    static Logger& instance()
    {
        static Logger inst;
        return inst;
    }

    // Path to the on-disk log file (wide string; .c_str() passed to crash handler).
    const std::wstring& getLogPath() const { return m_logPath; }
    void setLogPath(const std::wstring& path) { m_logPath = path; }

private:
    std::wstring m_logPath = L"emberfire.log";
};

//
// blog - printf-style logging to stdout.
//
// First argument is the Logger level; the rest is a printf format string + args.
//
inline void blog(int level, const char* fmt, ...)
{
    std::printf("[%s] ", Logger::levelName(level));

    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);

    std::printf("\n");
    std::fflush(stdout);
}
