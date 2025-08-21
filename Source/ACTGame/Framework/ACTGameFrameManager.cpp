#include "ACTGameFrameManager.h"

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
