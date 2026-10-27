// ACT 动画实例
//
// ================================================================ 实现层归属原则
//
//   动画蓝图节点能做的   → 一律在 ABP 里做（Property Access / Warping / Distance Matching / 数学节点）
//   节点做不到的         → 才放这里（目前只剩：跨线程安全的蒙太奇播放）
//
// 理由：ABP 里改参数不重编译，迭代快；C++ 只在真的出现性能问题（Property Access 拷贝量过大、
// 线程安全更新超预算）时才接管计算。
//
// 因此这个类**故意做得很薄**，不覆写 NativeThreadSafeUpdateAnimation —— 没有 WT 逻辑。
// 曾经在这里算过的 Speed / AnimSpeed / StrideScale / OrientationAngle / Lean / LandPredictionAlpha /
// BrakingDistance 全部下放到 ABP，逐个节点的接法见 Docs/ABP-v1-Wiring.md。
//
// ================================================================ 剩下的唯一职责
//
// Montage_Play 会改 MontageInstances 等 UObject 状态，在动画工作线程（WT）上调用是未定义行为。
// ALS 为此给 FAnimInstanceProxy::PostUpdate 打了补丁；这里用一个 MPSC 队列实现同样语义：
//   GT 调用 → 立即播；WT 调用 → 入队，下一次 NativeUpdateAnimation 里播。
// 这是节点做不到的事，也是继承 UAnimInstance 的唯一理由。

#pragma once

#include <atomic>

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ACTAnimInstance.generated.h"

class ACharacter;
class UAnimMontage;
class UACTCharacterMovementComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogACTAnimInstance, Log, All);

/**
 * 待播放的蒙太奇请求。用于跨线程投递：WT 里（例如动画通知）不能直接调 Montage_Play，
 * 必须排队到 GT。持有 WeakObjectPtr 以避免请求跨帧存活时把 Montage 钉在内存里。
 */
struct FActPendingMontage
{
	TWeakObjectPtr<UAnimMontage> Montage;
	float PlayRate = 1.0f;
	float StartPosition = 0.0f;
};

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

	// ================================================================ 便捷访问

	/** 宿主角色。找不到时为 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Anim")
	ACharacter* GetActCharacter() const { return Character; }

	/** 自己的移动组件。BlueprintPure 只能在 GT 上调（Event Graph 里没问题，WT 里不要调）。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Anim")
	UACTCharacterMovementComponent* GetACTMovement() const { return ACTMovement; }

	// ================================================================ 蒙太奇（线程安全）

	/**
	 * 播放蒙太奇。可在任意线程调用：
	 *   - GT 上调用：立即播放，返回蒙太奇时长；
	 *   - WT 上调用：入队，返回 0，实际播放在下一次 NativeUpdateAnimation 里发生。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Montage")
	float PlayMontageSafe(UAnimMontage* Montage, float PlayRate = 1.0f, float StartPosition = 0.0f);

	/** 强制入队（即使当前在 GT 上）。用于"本帧不要立刻生效"的场合，例如同一帧连发两个动作。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Montage")
	void QueueMontage(UAnimMontage* Montage, float PlayRate = 1.0f, float StartPosition = 0.0f);

	/** 停止当前所有蒙太奇（带淡出）。同上，WT 安全。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Montage")
	void StopMontagesSafe(float BlendOutTime = 0.2f);

	// ================================================================ 动画驱动朝向（线程安全出口）
	//
	// 为什么放在 AnimInstance 而不是移动组件：`BlueprintThreadSafeUpdateAnimation` 里能拿到的
	// 只有 self。要拿到移动组件就得走 TryGetPawnOwner → Cast → GetCharacterMovement，
	// 而 `GetCharacterMovement` **没有** BlueprintThreadSafe 标记，整条链在 WT 上编译不过 ——
	// 这就是"没有线程安全的 self 可以赋值"的来源。
	//
	// 放在这里之后：WT 写的是自己的原子槽，不碰任何外部对象；移动组件在 GT 上把它拉走。
	// 好处是全程没有跨对象的 UFUNCTION 调用（ProcessEvent 在 WT 上跑才是真正的风险）。

	/**
	 * 把动画曲线里的 root 偏航角喂给移动组件。可在 WT 上直接调。
	 * 传的是**相对进入 pivot 状态时的偏移**（度，带符号），曲线首帧应为 0。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion", meta = (BlueprintThreadSafe))
	void SetAnimationDrivenYawOffset(float YawOffsetDegrees);

	/** 结束动画驱动。可在 WT 上直接调。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion", meta = (BlueprintThreadSafe))
	void StopAnimationDrivenYaw();

	// 下面三个只给移动组件在 GT 上拉取，不要在蓝图里用（没有 BlueprintPure，不进节点面板）。

	bool IsAnimYawDriveActive() const { return AnimYawActiveSlot.load(std::memory_order_relaxed); }
	float GetAnimYawDriveOffset() const { return AnimYawOffsetSlot.load(std::memory_order_relaxed); }
	uint32 GetAnimYawDriveSeq() const { return AnimYawSeq.load(std::memory_order_relaxed); }

protected:
	// ================================================================ 生命周期

	virtual void NativeInitializeAnimation() override;

	/** GT。唯一的职责是 flush 蒙太奇队列 —— 这里不拷贝任何 Locomotion 数据。 */
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// ================================================================ 内部

	void FlushPendingMontages();

	// ================================================================ 缓存

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UACTCharacterMovementComponent> ACTMovement;

	// ================================================================ 队列

	/** 蒙太奇请求队列。Mpsc = 多生产者（任意工作线程）单消费者（GT）。 */
	TQueue<FActPendingMontage, EQueueMode::Mpsc> PendingMontages;

	TQueue<float, EQueueMode::Mpsc> PendingStopRequests;

	// ================================================================ 动画驱动朝向的原子槽
	//
	// 用 std::atomic 而不是 UPROPERTY：这两个槽会被动画工作线程写。
	// UPROPERTY 的写入会碰到 UObject 内部状态（GC 列表、属性链），在 WT 上做就是数据竞争。

	std::atomic<float> AnimYawOffsetSlot{0.0f};
	std::atomic<bool> AnimYawActiveSlot{false};
	std::atomic<uint32> AnimYawSeq{0};
};
