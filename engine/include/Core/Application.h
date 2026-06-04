#pragma once

#include <memory>
#include <string>

namespace EntityEngine
{
    class Game;
    class IRenderer2D;
    class Window;

    /**
     * @brief Clase de alto nivel que gestiona el ciclo de vida de la aplicación.
     *
     * Se encarga de inicializar SDL, crear la ventana principal y ejecutar
     * el bucle principal. Otros subsistemas (input, tiempo, render, etc.)
     * deberían arrancarse o actualizarse desde aquí para garantizar un punto
     * de entrada coherente para los juegos construidos sobre el engine.
     */
    class Application
    {
    public:
        /**
         * @brief Construye la aplicación y prepara los sistemas base.
         *
         * @param title  Título que aparecerá en la barra de la ventana.
         * @param width  Anchura inicial de la ventana en píxeles.
         * @param height Altura inicial de la ventana en píxeles.
         */
        Application(const std::string &title = "Entity Engine", int width = 1280, int height = 720);

        /**
         * @brief Libera recursos y cierra SDL si se inicializó correctamente.
         */
        ~Application();

        /**
         * @brief Ejecuta el bucle principal hasta que se solicite salir.
         *
         * Gestiona eventos, actualiza el reloj (Time) y delega en
         * OnUpdate() para la lógica del juego o motor. Solo se ejecutará
         * si la construcción de la aplicación fue exitosa.
         */
        void Run();
        void Run(Game &game);

    private:
        bool m_IsRunning;          ///< Señala si el bucle principal sigue activo.
        bool m_SdlInitialized;     ///< Indica si SDL se inicializó para limpiar en el destructor.
        std::unique_ptr<Window> m_Window; ///< Ventana principal de la aplicación.
        std::unique_ptr<IRenderer2D> m_Renderer2D; ///< Sistema de renderizado 2D (inicialmente nulo).
    };
}
