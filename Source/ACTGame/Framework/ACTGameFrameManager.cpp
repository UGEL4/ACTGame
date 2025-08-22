#include "ACTGameFrameManager.h"

void ACTGameFrameManager::Reset()
{
    TickCallback   = nullptr;
    CurrentFrame   = 0;
    NowTime        = 0.0;
    AccumulateTime = 0.0;
    FixedDeltaTime = 0.33;
}

void ACTGameFrameManager::Tick(float DeltaTime)
{
    AccumulateTime += DeltaTime;
    NowTime += DeltaTime;
    while (AccumulateTime >= FixedDeltaTime)
    {
        AccumulateTime -= FixedDeltaTime;
        CurrentFrame += 1;
        TickCallback(DeltaTime, CurrentFrame);
    }
}
