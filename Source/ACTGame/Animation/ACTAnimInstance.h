// ACT 动画实例
//
// ================================================================ 这个类为什么还活着
//
// 只有一个理由：`BlueprintThreadSafeUpdateAnimation` 的签名里只有 self。
// 要从 self 走到移动组件，得走 TryGetPawnOwner → Cast<ACharacter> → GetCharacterMovement()，
// 而 `GetCharacterMovement` **没有** BlueprintThreadSafe 标记，整条链在 WT 上编译不过。
//
// 所以这里只做一件事：把 Owner 和它的移动组件缓存成指针，给 ABP 一个 GT 侧的便捷入口。
//
// ================================================================ 故意不做什么
//
//   不计算任何 Locomotion 数据 —— Speed / AnimSpeed / StrideScale / OrientationAngle / Lean /
//   LandPredictionAlpha / BrakingDistance 全部在 ABP 里用节点算（Property Access + 数学节点），
//   改参数不重编译，迭代快。逐个接法见 Docs/ABP-v1-Wiring.md。
//
//   不持有任何跨线程状态 —— 曾经有一套「动画曲线写原子槽 → 移动组件在 GT 拉走」的旋转通道，
//   随着移动组件去掉动画驱动朝向（Docs/Turn-Pivot-CurveDriven-Design.md 附录 B）已经整条失效：
//   读端 `ConsumeAnimationDrivenYaw` 没了，写端 ABP 也从未调用过 `SetAnimationDrivenYawOffset`。
//   旋转的权威回到移动组件，动画层只通过下面的 getter 读数据，不再反向写。

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ACTAnimInstance.generated.h"

class ACharacter;
class UACTCharacterMovementComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogACTAnimInstance, Log, All);

/**
 * 角色动画实例基类。
 *
 * 这个类**不向动画蓝图提供任何 Locomotion 数据**。需要的数据请用 Property Access 直接读：
 *
 *   | 需要什么            | Property Access 路径                                  |
 *   |---------------------|------------------------------------------------------|
 *   | 速度 / 速度方向      | Character.Velocity（配 VSizeXY / Normalize 数学节点） |
 *   | 是否蹲下            | Character.bIsCrouched                                |
 *   | 是否在空中          | CharMoveComp.IsFalling()（或 MovementMode）           |
 *   | 步态 / 站姿 / 朝向模式 | CharMoveComp.Gait / .Stance / .RotationMode         |
 *   | 到落地时间 / 离地距离 | CharMoveComp.LocomotionState.TimeToLand 等           |
 *   | 停止距离            | PredictGroundMovementStopLocation 节点               |
 *
 * 完整接法与理由：Docs/ABP-v1-Wiring.md
 *
 * 注意：Property Access 的路径必须指向 BlueprintReadOnly 的 UPROPERTY，
 * 且整个链上的属性都要可见，否则编辑器里选不到。
 */
UCLASS(Blueprintable)
class ACTGAME_API UACTAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UACTAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ================================================================ 缓存访问
	//
	// 这两个是 BlueprintPure，**只能在 GT 上调**（Event Graph 没问题，WT 里不要调）。
	// 之所以值得存在：ABP 里要拿移动组件做 Cast / 取引用，每帧自己走一遍
	// GetOwningActor → Cast 既啰嗦又容易写错类型。

	/** 宿主角色。找不到时为 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Anim")
	ACharacter* GetActCharacter() const { return Character; }

	/** 自己的移动组件。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Anim")
	UACTCharacterMovementComponent* GetACTMovement() const { return ACTMovement; }

protected:
	virtual void NativeInitializeAnimation() override;

	// ================================================================ 缓存

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UACTCharacterMovementComponent> ACTMovement;
};
