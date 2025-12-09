#include "../../include/Core/log.h"

namespace EntityEngine
{
    LogLevel Log::s_MinLevel = LogLevel::Trace;

    void Log::SetLevel(LogLevel level)
    {
        s_MinLevel = level;
    }

    // Convierte el enum de nivel en un texto legible para la consola.
    static const char *LevelToString(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Critical:
            return "CRITICAL";
        default:
            return "UNKNOWN";
        }
    }

    // Selecciona el stream de salida según la severidad.
    static std::ostream &OutputStream(LogLevel level)
    {
        if (level == LogLevel::Error || level == LogLevel::Critical)
            return std::cerr;
        return std::cout;
    }

    // Evalúa si el mensaje debe imprimirse con base en el nivel configurado.
    static bool ShouldLog(LogLevel minLevel, LogLevel msgLevel)
    {
        return static_cast<int>(msgLevel) >= static_cast<int>(minLevel);
    }

    // Formatea y escribe el mensaje en el flujo adecuado.
    static void LogMessage(LogLevel level, const std::string &msg)
    {
        if (!ShouldLog(Log::s_MinLevel, level))
            return;

        auto &os = OutputStream(level);
        os << "[EntityEngine][" << LevelToString(level) << "] " << msg << '\n';
    }

    void Log::Trace(const std::string &msg)
    {
        LogMessage(LogLevel::Trace, msg);
    }
    void Log::Info(const std::string &msg)
    {
        LogMessage(LogLevel::Info, msg);
    }
    void Log::Warn(const std::string &msg)
    {
        LogMessage(LogLevel::Warn, msg);
    }
    void Log::Error(const std::string &msg)
    {
        LogMessage(LogLevel::Error, msg);
    }
    void Log::Critical(const std::string &msg)
    {
        LogMessage(LogLevel::Critical, msg);
    }
}
