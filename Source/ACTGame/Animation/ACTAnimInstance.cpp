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

void UACTAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// 只在初始化时解析一次。之后 Pawn 不会换（换 Pawn 会重建 AnimInstance），
	// 所以没有做 PawnOwner 变化的失效检测 —— 真要做的话该在 ABP 的
	// BlueprintInitializeAnimation 里，而不是在这里每帧比指针。
	Character = Cast<ACharacter>(GetOwningActor());
	ACTMovement = Character ? Cast<UACTCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;

	if (!Character)
	{
		UE_LOG(LogACTAnimInstance, Warning,
			TEXT("%s: 动画实例的 Owner 不是 ACharacter。ABP 里的 Property Access 会取不到数据。"),
			*GetNameSafe(this));
	}
}

// ================================================================ 偏航曲线通道（跨线程）

void UACTAnimInstance::PushYawCurveOffset(float OffsetDegrees)
{
	// ⚠️ 这个函数会在**动画工作线程**上执行（BlueprintThreadSafeUpdateAnimation / 状态函数）。
	// 只写下面三行原子：不读 Character / ACTMovement、不写 UPROPERTY、不打日志。
	// 任何一个都会变成数据竞争，症状是偶发数值跳变，几乎查不出来。
	//
	// 顺序有讲究：先写载荷，再 release 递增序号。GT 侧 acquire 读到新序号时，
	// 载荷一定已经可见 —— 反过来（先加序号）会读到上一帧的偏移。
	YawCurveOffset.store(OffsetDegrees, std::memory_order_relaxed);
	YawCurveActive.store(true, std::memory_order_relaxed);
	YawCurveSeq.fetch_add(1, std::memory_order_release);
}

void UACTAnimInstance::ClearYawCurveOffset()
{
	// 同上，WT 侧。真正的收尾（退回引擎驱动）在 CMC 的 ApplyAnimationDrivenYaw 里做。
	YawCurveActive.store(false, std::memory_order_release);
}

bool UACTAnimInstance::ReadYawCurve(float& OutOffsetDegrees, uint32& OutSeq) const
{
	// GT 侧。先读序号再读载荷：序号变了说明载荷也写好了（见 Push 里的顺序说明）。
	OutSeq = YawCurveSeq.load(std::memory_order_acquire);
	OutOffsetDegrees = YawCurveOffset.load(std::memory_order_relaxed);
	return YawCurveActive.load(std::memory_order_relaxed);
}

void UACTAnimInstance::ResetYawCurve()
{
	// 不动序号：调用方（CMC）会紧接着用 GetYawCurveSeq() 对齐，这样下一个新样本一定能被识别。
	YawCurveOffset.store(0.0f, std::memory_order_relaxed);
	YawCurveActive.store(false, std::memory_order_relaxed);
}
