// ACT 角色类 —— 输入到意图的唯一出口
//
// 职责边界（严格）：
//   1. 只负责「输入 → 移动意图」和「相机」。
//   2. 不写任何动画逻辑，不直接改 Velocity（一律走 AddMovementInput，由移动组件算出速度）。
//   3. 所有状态变更都转发给 UACTCharacterMovementComponent，角色本身不存一份副本
//      （避免出现"角色说在跑、组件说在走"这种双份状态不一致）。
//
// 输入结构沿用 ALS-Refactored 的 Example 角色：Enhanced Input + Input_OnXXX 系列虚函数，
// 好处是之后接入 ActionLogicComponent / GAS 时，只需在这些函数里改派发目标，绑定表不用动。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Locomotion/ACTLocomotionTypes.h"
#include "ACTCharacterBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UAnimMontage;
class UACTCharacterMovementComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogACTCharacterBase, Log, All);

UCLASS()
class ACTGAME_API AACTCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AACTCharacterBase(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;

	/** 控制器变化时换 IMC。断线重连、Possess 切换都走这里。 */
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// ================================================================ 相机

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	// ================================================================ 移动状态转发

	/**
	 * 拿到自己的移动组件。构造期已用 SetDefaultSubobjectClass 替换为 UACTCharacterMovementComponent，
	 * 所以正常情况下不会是 nullptr。
	 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	UACTCharacterMovementComponent* GetACTMovement() const;

	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetGait(EACTGait NewGait);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTGait GetGait() const;

	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetStance(EACTStance NewStance);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTStance GetStance() const;

	UFUNCTION(BlueprintCallable, Category = "ACT|Locomotion")
	void SetRotationMode(EACTRotationMode NewRotationMode);

	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	EACTRotationMode GetRotationMode() const;

	/** 蓝图用的按值拷贝。C++ 侧请直接 GetACTMovement()->GetLocomotionState() 拿引用，避免每帧拷贝。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Locomotion")
	FActLocomotionState GetLocomotionState() const;

protected:
	// ================================================================ 输入处理

	virtual void Input_OnLookMouse(const FInputActionValue& ActionValue);

	virtual void Input_OnLook(const FInputActionValue& ActionValue);

	virtual void Input_OnMove(const FInputActionValue& ActionValue);

	/** 按住冲刺，松开回到 Walk/Run 的当前偏好。 */
	virtual void Input_OnSprint(const FInputActionValue& ActionValue);

	/** 切换 Walk / Run 偏好（不影响正在冲刺时的实际步态）。 */
	virtual void Input_OnWalk();

	virtual void Input_OnJump(const FInputActionValue& ActionValue);

	virtual void Input_OnCrouch();

	virtual void Input_OnRoll();

	/**
	 * 移动组件请求急转身。TurnAngle 为剩余转角（度，正 = 向左）。
	 *
	 * 默认实现在 C++（播 PivotMontageLeft / Right）。做成 BlueprintNativeEvent 是为了让蓝图能：
	 *   1. 从指定时间开始播（跳过 clip 的 lead-in 段，让转身立刻开始）
	 *   2. 在旋转完成处提前结束（后半段"转身后继续跑"不一定需要完整播）
	 *
	 * 重写时请自己保留"已有蒙太奇在播就不插队"的判断，默认实现里有 ——
	 * 两个带 root motion 的蒙太奇叠加会把胶囊转飞。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "ACT|Action")
	void HandlePivotRequested(float TurnAngle);

	virtual void HandlePivotRequested_Implementation(float TurnAngle);

	/**
	 * 从指定时间（秒）开始播放蒙太奇。
	 *
	 * 蓝图节点 `Play Anim Montage` 只能指定 Section、不能指定时间，这个补上。
	 * 内部 `bStopAllMontages` 传 **false** —— 引擎默认是 true，会顺手把翻滚等蒙太奇全停掉。
	 *
	 * @return 从 StartTime 到结尾的剩余时长（秒）；失败返回 0。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Action")
	float PlayMontageFromTime(UAnimMontage* Montage, float StartTime, float PlayRate = 1.0f);

	/**
	 * 播 pivot 并从指定 Section 开始。**蓝图重写 HandlePivotRequested 时直接调这一个节点就够**。
	 *
	 * 一次做完四件事：按 TurnAngle 选左右、Montage_Play（bStopAllMontages = false）、
	 * 跳到 Section、记录 ActivePivotMontage（这样 StopPivotMontage 才能准确停掉它）。
	 *
	 * @param StartSection NAME_None 表示从头播。
	 * @param PlayRate <= 0 表示用 PivotPlayRate。
	 * @return 蒙太奇总时长（秒，已按 PlayRate 折算）；失败返回 0。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Action")
	float PlayPivotFromSection(float TurnAngle, FName StartSection, float PlayRate = -1.0f);

	/**
	 * 提前结束 pivot 蒙太奇（带淡出）。只停 pivot，不会误停翻滚/攻击。
	 *
	 * 注意：截断会同时截断 root motion。**要在旋转已经完成之后停**，
	 * 否则剩下的角度会由 PhysicsRotation 以 1080deg/s 补完，看起来是一段硬转。
	 */
	UFUNCTION(BlueprintCallable, Category = "ACT|Action")
	void StopPivotMontage(float BlendOutTime = 0.2f);

	/** pivot 蒙太奇当前播放位置（秒）；没在播返回 0。想"转到就停"可以用它，也可以改用 AnimNotify。 */
	UFUNCTION(BlueprintPure, Category = "ACT|Action")
	float GetPivotMontagePosition() const;

	// ================================================================ 组件

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// ================================================================ 输入资产

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RollAction;

	// ================================================================ 输入参数

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", Meta = (ClampMin = 0.0f, ForceUnits = "x"))
	float LookUpMouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", Meta = (ClampMin = 0.0f, ForceUnits = "x"))
	float LookRightMouseSensitivity = 1.0f;

	/** 摇杆/键鼠速率模式的俯仰速度（deg/s）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float LookUpRate = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", Meta = (ClampMin = 0.0f, ForceUnits = "deg/s"))
	float LookRightRate = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", Meta = (ClampMin = -89.0f, ClampMax = 0.0f, ForceUnits = "deg"))
	float MinLookPitch = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Look", Meta = (ClampMin = 0.0f, ClampMax = 89.0f, ForceUnits = "deg"))
	float MaxLookPitch = 60.0f;

	// ================================================================ 动作（Phase 6 会交给 ActionLogicComponent）

	/** 临时的翻滚蒙太奇直连。Phase 6 接入动作系统后改为由 ActionInfoAsset 驱动。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", Meta = (DisplayThumbnail = false))
	TObjectPtr<UAnimMontage> RollMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Action", Meta = (ClampMin = 0.1f))
	float RollPlayRate = 1.3f;

	/**
	 * 急转身（180° 反打）蒙太奇。移动组件判定"玩家真的反打了"时通过 OnPivotRequested 触发。
	 *
	 * 为什么必须是蒙太奇：这类 clip 自带 180° 的 root 旋转，只有蒙太奇的 root motion 会
	 * 真正驱动胶囊旋转（AnimInstance 默认 RootMotionMode 是 RootMotionFromMontagesOnly，
	 * 状态机里的 root motion 是被忽略的）。用状态机播的话，身体在 clip 里转了 180°，
	 * 胶囊没转，结果就是"人和朝向对不上"。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", Meta = (DisplayThumbnail = false))
	TObjectPtr<UAnimMontage> PivotMontageLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Action", Meta = (DisplayThumbnail = false))
	TObjectPtr<UAnimMontage> PivotMontageRight;

	/** pivot 蒙太奇的播放速率。想让它塞进 PivotCooldown 里就调这里。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input|Action", Meta = (ClampMin = 0.1f))
	float PivotPlayRate = 1.0f;

	/** 当前正在播的 pivot 蒙太奇。`Montage_Stop` 第二个参数传 nullptr 会停掉**所有**蒙太奇，
	 *  所以必须记住停哪一个。 */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActivePivotMontage;

	// ================================================================ 运行时状态

	UPROPERTY(Transient)
	TObjectPtr<UACTCharacterMovementComponent> ACTMovement;

	/** Walk/Run 的当前偏好。冲刺是"按住"，释放后回到这个偏好。 */
	UPROPERTY(Transient)
	bool bDesiredWalk = false;
};
