#pragma once

#include <iostream>
#include <string>

namespace EntityEngine
{

    enum class LogLevel
    {
        Trace,
        Info,
        Warn,
        Error,
        Critical
    };

    class Log
    {
    public:
        static void SetLevel(LogLevel level);

        static void Trace(const std::string &msg);
        static void Info(const std::string &msg);
        static void Warn(const std::string &msg);
        static void Error(const std::string &msg);
        static void Critical(const std::string &msg);

        static LogLevel s_MinLevel;
    };

// Macros para hacer los logs más cómodos (y fáciles de desactivar en Release)
#define EE_LOG_TRACE(msg) ::EntityEngine::Log::Trace(msg)
#define EE_LOG_INFO(msg) ::EntityEngine::Log::Info(msg)
#define EE_LOG_WARN(msg) ::EntityEngine::Log::Warn(msg)
#define EE_LOG_ERROR(msg) ::EntityEngine::Log::Error(msg)
#define EE_LOG_CRITICAL(msg) ::EntityEngine::Log::Critical(msg)

} // namespace EntityEngine