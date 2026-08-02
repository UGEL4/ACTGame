// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "Section_HitBox.generated.h"

/**
 * 
 */

// MovieSceneCapsuleTrackSection.h
USTRUCT(BlueprintType)
struct FCapsuleKeyframeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Keyframe")
    FFrameNumber Time; // 关键帧在时间轴上的位置

    UPROPERTY(EditAnywhere, Category = "Keyframe")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "Keyframe")
    float CapsuleRadius = 34.0f;

    UPROPERTY(EditAnywhere, Category = "Keyframe")
    float CapsuleHalfHeight = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Keyframe")
    FRotator Rotation = FRotator::ZeroRotator;
};

class AHitBoxActor;

UCLASS()
class ACTIONEDITOR_API USection_HitBox : public UMovieSceneSection
{
	GENERATED_BODY()
	
public:
    USection_HitBox(const FObjectInitializer& ObjectInitializer);
    virtual ~USection_HitBox();

public:
    //UPROPERTY(EditAnywhere, BlueprintReadWrite)
    //TArray<FActionCommand> CommandList;
    UPROPERTY(EditAnywhere, Category = "Capsule")
    TArray<FCapsuleKeyframeData> Keyframes;

    // 辅助函数：根据时间获取最近的关键帧
    int32 GetKeyframeIndexAtTime(FFrameNumber InFrame, float Tolerance = 0.1f) const;
    void AddKeyFrame(FFrameNumber InFrame);

    /** 为指定帧号设置关联的 Actor */
    void SetActorForFrame(FFrameNumber Frame, AHitBoxActor* Actor);
    /** 获取指定帧号的关联 Actor */
    AHitBoxActor* GetActorForFrame(FFrameNumber Frame) const;
    /** 移除指定帧号的 Actor（并销毁它） */
    void RemoveActorForFrame(FFrameNumber Frame);
    /** 清除所有关联的 Actor */
    void ClearAllActors();

    // 在 BeginDestroy 中清理
    virtual void BeginDestroy() override;

private:
    TMap<FFrameNumber, TWeakObjectPtr<AHitBoxActor>> KeyframeToActor;

public:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
};
