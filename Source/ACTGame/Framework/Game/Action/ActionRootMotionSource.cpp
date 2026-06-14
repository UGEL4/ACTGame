#include "ActionRootMotionSource.h"
#include "GameFramework/Character.h"

void FActionRootMotionSource::PrepareRootMotion(float SimulationTime, float MovementTickTime, const ACharacter& Character, const UCharacterMovementComponent& MoveComponent)
{
    if (!SequenceData || SequenceData->RootMotionDeltas.Num() < 2)
    {
        return;
    }

    // 根据播放时间推进帧
    float SampleRate  = SequenceData->SampleFrameRate;
    int32 TotalFrames = SequenceData->RootMotionDeltas.Num();
    // 防止越界
    CurrentFrameIndex = FMath::Clamp(FMath::FloorToInt(CurrentTime * SampleRate), 0, TotalFrames - 1);

    // 获取本帧的局部空间增量
    FTransform LocalDelta = SequenceData->RootMotionDeltas[CurrentFrameIndex];

    // 转换为世界空间，基于角色当前朝向
    FTransform WorldDelta = LocalDelta * Character.GetActorTransform();
    // 只取位移和旋转，忽略缩放
    RootMotionParams.Set(WorldDelta);

    // 推进时间
    CurrentTime += MovementTickTime; // 你的固定步长
}
