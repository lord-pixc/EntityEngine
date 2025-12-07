#pragma once

namespace EntityEngine
{
    class Time
    {
    public:
        static void Init();
        static void Update();
        static float GetDeltaTime();
        static float GetTime();
        static float GetFPS();

    private:
        Time() = default;
    };
}