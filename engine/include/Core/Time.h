#pragma once

namespace EntityEngine
{
    /**
     * @brief Utilidades estáticas para medir el tiempo de ejecución.
     *
     * La clase no se instancia. Proporciona información sobre el delta time
     * por frame, el tiempo transcurrido desde el inicio y una estimación
     * del FPS actual. Debe inicializarse una vez al arrancar la aplicación
     * y actualizarse cada frame antes de usarse.
     */
    class Time
    {
    public:
        /// Inicializa el reloj interno registrando el instante inicial.
        static void Init();

        /// Calcula el delta time desde la última llamada.
        static void Update();

        /// @return Tiempo en segundos entre el frame actual y el anterior.
        static float GetDeltaTime();

        /// @return Tiempo total en segundos desde la inicialización.
        static float GetTime();

        /// @return Fotogramas por segundo estimados según el delta actual.
        static float GetFPS();

    private:
        Time() = default;
    };
}
