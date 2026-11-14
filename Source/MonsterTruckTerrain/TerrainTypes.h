#pragma once

#include "CoreMinimal.h"
#include "TerrainTypes.generated.h"

UENUM(BlueprintType)
enum class ETerrainBrushMode : uint8
{
	Raise,
	Lower,
	Smooth,
	Flatten
};

/**
 * Parameters for a jump ramp. The ramp's local +X axis is the drive direction:
 * the low end is at X = 0 and the launch lip is at X = Length.
 * CurvePower = 1 gives a straight wedge; values above 1 give a concave kicker
 * that gets steeper toward the lip.
 */
USTRUCT(BlueprintType)
struct FRampParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp", meta = (ClampMin = "100"))
	float Length = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp", meta = (ClampMin = "100"))
	float Width = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp", meta = (ClampMin = "10"))
	float Height = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp", meta = (ClampMin = "0.5", ClampMax = "6.0"))
	float CurvePower = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp", meta = (ClampMin = "2", ClampMax = "128"))
	int32 LengthSegments = 24;
};

/** A placed ramp, as stored in save files and undo snapshots. */
USTRUCT(BlueprintType)
struct FRampRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp")
	FTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp")
	FRampParams Params;
};
