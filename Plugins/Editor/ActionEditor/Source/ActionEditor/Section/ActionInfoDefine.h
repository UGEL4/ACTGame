#pragma once

#include "CoreMinimal.h"
//#include "HAL/Platform.h"
//#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "ActionInfoDefine.generated.h"

USTRUCT(BlueprintType)
struct FEditorCancelTag
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FixPriority{ 0 };
};

USTRUCT(BlueprintType)
struct FEditorCancelData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> Tags;

    //TArray<CancelTag> TempCancelTags;
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 FixPriority{ 0 };

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StartFrame{ 0 };
};

UENUM(BlueprintType)
enum class EEditor_KeyMap : uint8
{
	NoDir,
	NoInput,
	DirInput,
	ButtonWest,
	ButtonEast,
	ButtonNouth,
	ButtonSouth
};

USTRUCT(BlueprintType)
struct FEditorActionCommand
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<EEditor_KeyMap> keySequences;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ValidInFrameCount;
};
