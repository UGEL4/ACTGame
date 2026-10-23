// ACT Locomotion 数据契约
//
// 设计约束（务必先读）：
//   1. 这是 C++ 与动画蓝图之间 **唯一** 的接口。动画层不允许 Cast 角色或移动组件去取数据。
//   2. 只放"物理侧才拿得到"的信号（例如落地时间要射线/重力推导），
//      **不做纯数学推导** —— 那些放动画蓝图用数学节点算，改参数不用重编译。
//   3. 字段总数控制在 14 个以内。每加一个字段，动画蓝图就多一条分支 —— 这是 ALS 复杂度失控的根源。
//   4. 不依赖动画曲线。所有表现参数由这里算出来，而不是从 AnimSequence 里读。

#pragma once

#include "CoreMinimal.h"
#include "ACTLocomotionTypes.generated.h"

/**
 * 站姿。只影响动画层的 Blend Space 选择与胶囊体尺寸。
 */
UENUM(BlueprintType)
enum class EACTStance : uint8
{
	Stand,
	Crouch
};

/**
 * 步态。决定最大速度档位，不直接决定动画（动画由实际速度驱动）。
 */
UENUM(BlueprintType)
enum class EACTGait : uint8
{
	Walk,
	Run,
	Sprint
};

/**
 * 运动状态机的大状态。故意做得很粗 —— 细分的过渡交给动画蓝图的混合完成。
 */
UENUM(BlueprintType)
enum class EACTLocomotionMode : uint8
{
	/** 站在可行走地面上 */
	Grounded,
	/** 起跳上冲阶段（VelocityZ > 0），播放 Jump 动画 */
	Jumping,
	/** 下落阶段（VelocityZ <= 0 且未接触地面） */
	Falling,
	/** 落地缓冲阶段，持续 LandDuration 后回到 Grounded */
	Landing
};

/**
 * 朝向策略。对应 ALS 的 RotationMode，但只有两个值 —— 更多值意味着更多分支。
 */
UENUM(BlueprintType)
enum class EACTRotationMode : uint8
{
	/** 朝移动方向（自由移动） */
	VelocityDirection,
	/** 朝相机/锁定目标方向（战斗、瞄准） */
	LookingDirection
};

/**
 * 移动组件每帧算好的状态。由组件持有（`UACTCharacterMovementComponent::LocomotionState`），
 * 动画蓝图通过 **Property Access** 直接读，不再由动画实例拷贝一份。
 *
 * 使用方式：
 *   Property Access 路径：`CharMoveComp.LocomotionState.Speed` / `.TimeToLand` / `.Mode` ...
 *   C++ 侧：`Movement->GetLocomotionState()`（返回引用，零拷贝）
 *
 * 注意：本结构体是"物理侧信号"，不是"动画层计算结果"。
 * 混合权重、Lean、步幅缩放这类纯数学，一律在 ABP 里用节点算，见 Docs/ABP-v1-Wiring.md。
 */
USTRUCT(BlueprintType)
struct FActLocomotionState
{
	GENERATED_BODY()

	// ---------------------------------------------------------------- 速度

	/** 水平速度大小（cm/s）。Blend Space 的主轴。 */
	UPROPERTY(BlueprintReadOnly, Category = "Speed")
	float Speed = 0.0f;

	/** 垂直速度（cm/s）。负值表示下落。 */
	UPROPERTY(BlueprintReadOnly, Category = "VerticalSpeed")
	float VerticalSpeed = 0.0f;

	/** 速度方向相对角色前向的夹角（度，-180~180）。用于 strafe 混合与转身判定。
	 *  0 = 正前方，+90 = 右侧。角色静止时为 0。 */
	UPROPERTY(BlueprintReadOnly, Category = "Speed")
	float VelocityYawOffset = 0.0f;

	/**
	 * 朝向修正角（度）= 角色当前朝向 与 「应该朝向」 的差。
	 * 这是 Orientation Warping 的输入。由 C++ 计算，不需要动画曲线。
	 * 正负号需与 Orientation Warping 节点的符号约定一致，Phase 4 联调时确认。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Speed")
	float YawOffset = 0.0f;

	// ---------------------------------------------------------------- 输入意图

	/** 归一化移动输入，**角色本地空间**（X = 右，Y = 前），模长已钳到 1。
	 *  用来做起步方向判定与"有输入但还没加速起来"的提前量，不用来做 Strafe 混合
	 *  （Strafe 用 VelocityYawOffset，那是实际速度方向，不会抖）。 */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	FVector2D MoveInput = FVector2D::ZeroVector;

	/** 加速强度（cm/s²，已平滑）。用于 Lean 与起步倾斜。 */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	float AccelerationMagnitude = 0.0f;

	/** 刹车强度（0~1，已平滑）。1 = 全速刹停中。驱动 Stop 动画的权重。 */
	UPROPERTY(BlueprintReadOnly, Category = "Input")
	float BrakeAmount = 0.0f;

	// ---------------------------------------------------------------- 状态

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EACTLocomotionMode Mode = EACTLocomotionMode::Grounded;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EACTStance Stance = EACTStance::Stand;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EACTGait Gait = EACTGait::Run;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EACTRotationMode RotationMode = EACTRotationMode::VelocityDirection;

	// ---------------------------------------------------------------- 地面 / 空中

	/**
	 * 与地面的距离（cm）。Grounded 时约等于 0，空中为正值，下落越快值越大。
	 * Foot Placement 与落地预备姿势用。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Ground")
	float DistanceToGround = 0.0f;

	/**
	 * 距离落地的预估时间（秒）。Falling 时有效，用于"落地预备"混合。
	 * 由 VerticalSpeed / GravityZ 推导，不做射线。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Ground")
	float TimeToLand = 0.0f;

	/** 是否处于落地缓冲阶段。动画层用它切 Land 动画的淡出。 */
	UPROPERTY(BlueprintReadOnly, Category = "Ground")
	bool bLanding = false;
};

/**
 * 单档步态的移动参数。放在 DeveloperSettings 或 DataAsset 里，不要硬编码。
 */
USTRUCT(BlueprintType)
struct FActGaitSettings
{
	GENERATED_BODY()

	/** 该档位在动画 Blend Space 里对应的参考速度（cm/s）。用于把速度映射到混合位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", Meta = (ClampMin = 0))
	float BlendSpaceSpeed = 0.0f;

	/** 最大移动速度（cm/s）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", Meta = (ClampMin = 0))
	float MaxSpeed = 0.0f;

	/** 达到最大速度所需时间（秒）。换算成加速度用，比直接填加速度更好调。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", Meta = (ClampMin = 0.01f))
	float TimeToMaxSpeed = 0.3f;

	/** 减速到 0 所需时间（秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gait", Meta = (ClampMin = 0.01f))
	float TimeToStop = 0.2f;
};

/**
 * 三档步态的参数集合。
 *
 * 不用 TMap<EACTGait, FActGaitSettings>：枚举做 TMap 键在 UHT/序列化上容易踩坑，
 * 固定三个成员 + Get() 更省事，也更容易在编辑器里读。
 */
USTRUCT(BlueprintType)
struct FActGaitSettingsSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk")
	FActGaitSettings Walk;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Run")
	FActGaitSettings Run;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sprint")
	FActGaitSettings Sprint;

	const FActGaitSettings& Get(EACTGait InGait) const
	{
		switch (InGait)
		{
		case EACTGait::Walk: return Walk;
		case EACTGait::Sprint: return Sprint;
		default: return Run;
		}
	}
};
