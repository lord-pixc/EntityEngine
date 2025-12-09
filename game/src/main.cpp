#include "../../engine/include/Core/Application.h"
#include <iostream>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    // Punto de entrada del ejecutable de ejemplo. Inicializa la aplicación del
    // motor, ejecuta su bucle principal y luego sale limpiamente.
    std::cout << "[Game] Iniciando EntityEngine...\n";

    EntityEngine::Application app("EntityEngine Demo", 1280, 720);
    app.Run();

    std::cout << "[Game] Saliendo.";
}
