#pragma once

#include "CoreMinimal.h"
#include "HitBox.generated.h"

UENUM(BlueprintType)
enum class EHitShapeType : uint8
{
    Box,
    Sphere,
    Capsule
};

USTRUCT(BlueprintType)
struct FShapeData
{
    GENERATED_BODY()

    // 形状种类
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    EHitShapeType ShapeType = EHitShapeType::Box;

    // 相对于挂载组件的偏移
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    FVector RelativeLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    FRotator RelativeRotation = FRotator::ZeroRotator;

    // 统一参数：盒体(X半长,Y半宽,Z半高) | 球体(X半径) | 胶囊(X半径, Y半高)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    FVector Param = FVector(50.0f, 50.0f, 50.0f);

    // 辅助取值函数
    FVector GetBoxExtent() const { return Param; }
    float GetSphereRadius() const { return Param.X; }
    float GetCapsuleRadius() const { return Param.X; }
    float GetCapsuleHalfHeight() const { return Param.Y; }
};

USTRUCT(BlueprintType)
struct FAttackBoxData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    FShapeData Shape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    FName SameTag; // 对口组，有时候需要有相同tag的攻击框和受击框才能相互碰撞

    UPROPERTY(EditAnywhere)
    float Attack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
    int32 Priority = 0;
};

USTRUCT(BlueprintType)
struct FHitBoxData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape")
    FShapeData Shape;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
    FName SameTag; // 对口组，有时候需要有相同tag的攻击框和受击框才能相互碰撞

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
    float Defense;
};

