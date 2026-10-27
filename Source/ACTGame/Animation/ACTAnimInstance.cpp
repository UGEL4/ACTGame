#include "Animation/ACTAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Locomotion/ACTCharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ACTAnimInstance)

DEFINE_LOG_CATEGORY(LogACTAnimInstance);

UACTAnimInstance::UACTAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// ================================================================ 生命周期

void UACTAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Character = Cast<ACharacter>(GetOwningActor());
	ACTMovement = Character ? Cast<UACTCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;

	if (!Character)
	{
		UE_LOG(LogACTAnimInstance, Warning,
			TEXT("%s: 动画实例的 Owner 不是 ACharacter。蒙太奇仍可用，但 ABP 里的 Property Access 会取不到数据。"),
			*GetNameSafe(this));
	}
}

void UACTAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 这里唯一的职责。Locomotion 数据由 ABP 通过 Property Access 直接读，
	// 不需要每帧拷快照 —— 那是"动画层零推导"时代的做法，现在没有 WT 逻辑就不需要了。
	FlushPendingMontages();
}

// ================================================================ 蒙太奇

float UACTAnimInstance::PlayMontageSafe(UAnimMontage* Montage, float PlayRate, float StartPosition)
{
	if (!IsValid(Montage))
	{
		return 0.0f;
	}

	if (!IsInGameThread())
	{
		QueueMontage(Montage, PlayRate, StartPosition);
		return 0.0f;
	}

	return Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, StartPosition);
}

void UACTAnimInstance::QueueMontage(UAnimMontage* Montage, float PlayRate, float StartPosition)
{
	if (!IsValid(Montage))
	{
		return;
	}

	FActPendingMontage Request;
	Request.Montage = Montage;
	Request.PlayRate = PlayRate;
	Request.StartPosition = StartPosition;

	PendingMontages.Enqueue(MoveTemp(Request));
}

void UACTAnimInstance::StopMontagesSafe(float BlendOutTime)
{
	if (!IsInGameThread())
	{
		PendingStopRequests.Enqueue(FMath::Max(0.0f, BlendOutTime));
		return;
	}

	Montage_Stop(FMath::Max(0.0f, BlendOutTime));
}

void UACTAnimInstance::SetAnimationDrivenYawOffset(float YawOffsetDegrees)
{
	// ⚠️ 这个函数会在**动画工作线程**上执行（BlueprintThreadSafeUpdateAnimation / 状态函数）。
	// 只写下面三行原子：不读 Character / ACTMovement、不写 UPROPERTY、不打日志。
	// 任何一个都会变成数据竞争，症状是偶发数值跳变，几乎查不出来。
	AnimYawOffsetSlot.store(YawOffsetDegrees, std::memory_order_relaxed);
	AnimYawActiveSlot.store(true, std::memory_order_relaxed);
	AnimYawSeq.fetch_add(1, std::memory_order_relaxed);
}

void UACTAnimInstance::StopAnimationDrivenYaw()
{
	// 同上，WT 侧。真正的收尾在移动组件的 ConsumeAnimationDrivenYaw() 里做（GT）。
	AnimYawActiveSlot.store(false, std::memory_order_relaxed);
}

void UACTAnimInstance::FlushPendingMontages()
{
	// 先处理停止请求，顺序是"先停后播"：同一帧里"取消上一招 + 接下一招"才不会把新招也停掉。
	float StopBlendOut = 0.0f;
	bool bHasStop = false;

	while (PendingStopRequests.Dequeue(StopBlendOut))
	{
		bHasStop = true;
	}

	if (bHasStop)
	{
		Montage_Stop(StopBlendOut);
	}

	FActPendingMontage Request;
	while (PendingMontages.Dequeue(Request))
	{
		UAnimMontage* Montage = Request.Montage.Get();

		// 排队期间资源可能已被 GC / 卸载，必须重新确认。
		if (!IsValid(Montage))
		{
			continue;
		}

		Montage_Play(Montage, Request.PlayRate, EMontagePlayReturnType::MontageLength, Request.StartPosition);
	}
}
