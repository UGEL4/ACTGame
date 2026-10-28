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
	//
	// 不开 GetCameraBoom / GetFollowCamera：它们是 BlueprintReadOnly 的组件属性，
	// 蓝图里拖出来就能用，C++ 侧直接读成员。曾经有一对 FORCEINLINE getter，全项目 0 引用。

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

	// ================================================================ 运行时状态

	UPROPERTY(Transient)
	TObjectPtr<UACTCharacterMovementComponent> ACTMovement;

	/** Walk/Run 的当前偏好。冲刺是"按住"，释放后回到这个偏好。 */
	UPROPERTY(Transient)
	bool bDesiredWalk = false;
};
