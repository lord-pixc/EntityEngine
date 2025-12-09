#pragma once

#include <iostream>
#include <string>

namespace EntityEngine
{

    /// Niveles de severidad disponibles para los mensajes de log.
    enum class LogLevel
    {
        Trace,
        Info,
        Warn,
        Error,
        Critical
    };

    /**
     * @brief Sistema de logging minimalista para depuración en consola.
     *
     * Emite los mensajes con un prefijo estándar que incluye el nombre del
     * motor y el nivel de severidad. El nivel mínimo es configurable en
     * tiempo de ejecución para silenciar información en builds de producción.
     */
    class Log
    {
    public:
        /// Define el nivel mínimo de mensajes que se imprimirán.
        static void SetLevel(LogLevel level);

        /// Emite un mensaje de diagnóstico detallado.
        static void Trace(const std::string &msg);

        /// Emite información general sobre el flujo de ejecución.
        static void Info(const std::string &msg);

        /// Notifica situaciones inusuales pero no críticas.
        static void Warn(const std::string &msg);

        /// Reporta errores que requieren atención inmediata.
        static void Error(const std::string &msg);

        /// Mensajes críticos previos a un cierre controlado o fallo fatal.
        static void Critical(const std::string &msg);

        static LogLevel s_MinLevel; ///< Nivel global configurado para los mensajes.
    };

// Macros para hacer los logs más cómodos (y fáciles de desactivar en Release)
#define EE_LOG_TRACE(msg) ::EntityEngine::Log::Trace(msg)
#define EE_LOG_INFO(msg) ::EntityEngine::Log::Info(msg)
#define EE_LOG_WARN(msg) ::EntityEngine::Log::Warn(msg)
#define EE_LOG_ERROR(msg) ::EntityEngine::Log::Error(msg)
#define EE_LOG_CRITICAL(msg) ::EntityEngine::Log::Critical(msg)

} // namespace EntityEngine
