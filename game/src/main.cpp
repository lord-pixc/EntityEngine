#include "../../engine/include/Core/Application.h"
#include <iostream>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    std::cout << "[Game] Iniciando EntityEngine...\n";

    EntityEngine::Application app("EntityEngine Demo", 1280, 720);
    app.Run();

    std::cout << "[Game] Saliendo.\n";
    return 0;
}
