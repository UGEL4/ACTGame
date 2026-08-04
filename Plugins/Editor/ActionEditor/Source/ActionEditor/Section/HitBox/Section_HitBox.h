// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneSection.h"
#include "Section_HitBox.generated.h"

/**
 * 
 */

class AHitBoxActor;

// MovieSceneCapsuleTrackSection.h
USTRUCT(BlueprintType)
struct FCapsuleKeyframeData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Keyframe")
    FFrameNumber Time; // 关键帧在时间轴上的位置
};

USTRUCT(BlueprintType)
struct FFrameHitBoxActor
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<TWeakObjectPtr<AHitBoxActor>> Actors;
};

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
    int32 GetKeyframeIndexAtTime(FFrameNumber InFrame) const;
    void AddKeyFrame(FFrameNumber InFrame);
    bool RemoveKeyFrame(FFrameNumber InFrame);

    /** 为指定帧号设置关联的 Actor */
    void SetActorForFrame(FFrameNumber Frame, AHitBoxActor* Actor);
    /** 获取指定帧号的关联 Actor */
    void GetActorsForFrame(FFrameNumber Frame, TArray<TWeakObjectPtr<AHitBoxActor>>& OutActors) const;
    bool HasAnyHitBoxInFrame(FFrameNumber Frame) const;
    /** 移除指定帧号的 Actor（并销毁它） */
    void RemoveActorForFrame(FFrameNumber Frame, AHitBoxActor* Actor);
    bool RemoveActorsForFrame(FFrameNumber Frame);
    /** 清除所有关联的 Actor */
    void ClearAllActors();

    bool ClearInvalidHitBoxActor();

    // 在 BeginDestroy 中清理
    virtual void BeginDestroy() override;

private:
    UPROPERTY()
    TMap<FFrameNumber, FFrameHitBoxActor> KeyframeToActor;

public:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
	
};
