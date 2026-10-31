// ACT 动画实例
//
// ================================================================ 两个职责，都是"节点做不到"的
//
//   1. 缓存访问：`BlueprintThreadSafeUpdateAnimation` 的签名里只有 self，要从 self 走到移动组件
//      得 TryGetPawnOwner → Cast<ACharacter> → GetCharacterMovement()，而 GetCharacterMovement
//      **没有** BlueprintThreadSafe 标记，整条链在 WT 上编译不过。所以这里缓存两个指针。
//
//   2. 偏航曲线通道：曲线模式的旋转由 ABP 驱动，ABP 算完的偏移要交给移动组件。
//      但 ABP 的线程安全更新跑在动画工作线程上，碰 `UPROPERTY` 会碰 GC/属性链 —— 那是数据竞争。
//      所以用两个 atomic 槽 + 一个序号做跨线程投递，GT 侧的移动组件按序号取走最新样本。
//      这条通道是**有意存在**的，不是历史残留：断掉任何一端，曲线模式就失效。
//
// ================================================================ 故意不做什么
//
//   不计算任何 Locomotion 数据 —— Speed / AnimSpeed / StrideScale / OrientationAngle / Lean /
//   LandPredictionAlpha / BrakingDistance 全部在 ABP 里用节点算（Property Access + 数学节点），
//   改参数不重编译，迭代快。逐个接法见 Docs/ABP-v1-Wiring.md。
//
//   不解释语义 —— "什么时候开始转、转多少度、转多快"全在 ABP 的曲线与 Notify 里。
//   这个类只搬数字：进来一个 float，出去一个 float，中间不判断。

#pragma once

#include <atomic>

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

	/** 宿主角色。找不到时为 nullptr。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Anim")
	ACharacter* GetActCharacter() const { return Character; }

	/** 自己的移动组件。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Anim")
	UACTCharacterMovementComponent* GetACTMovement() const { return ACTMovement; }

	// ================================================================ 偏航曲线通道（跨线程）
	//
	// 用法（ABP 侧，`Turn` 状态里）：
	//   On State Update  → PushYawCurveOffset(Pivot_Yaw 曲线值 * k)
	//   On State Exit    → ClearYawCurveOffset()
	//
	// 通道本身不做超时判断 —— 那在 CMC（`AnimationYawTimeout`）。这样"多久算过期"是
	// 物理侧的参数，动画侧只负责推。

	/**
	 * 推一个偏航偏移（度，带符号，正 = 向左）。可在 WT 上直接调。
	 *
	 * 传的是**相对进入曲线模式那一刻朝向**的偏移，不是绝对角 ——
	 * 所以曲线首帧必须是 0，否则第一帧会跳。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion", meta = (BlueprintThreadSafe))
	void PushYawCurveOffset(float OffsetDegrees);

	/** 结束推送。可在 WT 上直接调。之后 CMC 会退回引擎驱动（不必等超时）。 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion", meta = (BlueprintThreadSafe))
	void ClearYawCurveOffset();

	// 下面三个只给移动组件在 GT 上拉取，不要在蓝图里用（不进节点面板）。

	/** 原子地读出当前样本。返回 false 表示通道已关闭。 */
	bool ReadYawCurve(float& OutOffsetDegrees, uint32& OutSeq) const;

	/** 清空样本（进入曲线模式时调，避免吃到上一轮的残留）。不改序号。 */
	void ResetYawCurve();

	uint32 GetYawCurveSeq() const { return YawCurveSeq.load(std::memory_order_relaxed); }

protected:
	virtual void NativeInitializeAnimation() override;

	// ================================================================ 缓存

	UPROPERTY(Transient)
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient)
	TObjectPtr<UACTCharacterMovementComponent> ACTMovement;

	// ================================================================ 偏航通道的原子槽
	//
	// 用 std::atomic 而不是 UPROPERTY：这两个槽会被动画工作线程写。
	// UPROPERTY 的写入会碰到 UObject 内部状态（GC 列表、属性链），在 WT 上做就是数据竞争，
	// 症状是偶发数值跳变，几乎查不出来。

	std::atomic<float> YawCurveOffset{0.0f};

	/** 序号：每推一帧 +1。GT 侧靠它判断"这帧有没有新数据"，避免重复应用同一帧的值。 */
	std::atomic<uint32> YawCurveSeq{0};

	std::atomic<bool> YawCurveActive{false};
};
