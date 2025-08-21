#pragma once

#include <cstdint>
#include <functional>
struct ACTGameFrameManager
{
public:
    void Tick(float DeltaTime);
    std::int64_t GetCurrentFrame() const { return CurrentFrame; }
    void SetTickCallback(std::function<void(float, std::int64_t)> callback)
    {
        TickCallback = callback;
    }

    void SetGameFrameRate(std::int32_t FrameRate)
    {
        FixedDeltaTime = 1.f / FrameRate;
    }
private:
    std::function<void(float, std::int64_t)> TickCallback;
    std::int64_t CurrentFrame = 0;
    double NowTime = 0.0;
    double AccumulateTime = 0.0;
    float FixedDeltaTime = 0.33;
};
