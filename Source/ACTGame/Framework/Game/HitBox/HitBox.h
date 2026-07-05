#pragma once

#include "CoreMinimal.h"

struct BoxBase
{
    int32 Priority;
    FName SameTag;
};

struct AttackHitBox : public BoxBase
{
    float Attack;
};

struct HitBox : public BoxBase
{
    float Defense;
};

