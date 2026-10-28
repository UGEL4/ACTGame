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
