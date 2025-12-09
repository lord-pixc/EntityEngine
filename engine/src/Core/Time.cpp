#include "../../include/Core/Time.h"

#include <chrono>

namespace EntityEngine
{

    // Puntos de referencia para medir el tiempo transcurrido entre frames.
    static std::chrono::high_resolution_clock::time_point s_StartTime;
    static std::chrono::high_resolution_clock::time_point s_LastFrameTime;
    static float s_DeltaTime = 0.0f;

    void Time::Init()
    {
        s_LastFrameTime = std::chrono::high_resolution_clock::now();
        s_StartTime = s_LastFrameTime;
        s_DeltaTime = 0.0f;
    }

    void Time::Update()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> frameTime = currentTime - s_LastFrameTime;
        s_DeltaTime = frameTime.count();
        s_LastFrameTime = currentTime;
    }

    float Time::GetDeltaTime()
    {
        return s_DeltaTime;
    }

    float Time::GetTime()
    {
        std::chrono::duration<float> elapsed = s_LastFrameTime - s_StartTime;
        return elapsed.count();
    }

    float Time::GetFPS()
    {
        if (s_DeltaTime <= 0.00001f)
            return 0.0f;

        return 1.0f / s_DeltaTime;
    }

} // namespace EntityEngine
