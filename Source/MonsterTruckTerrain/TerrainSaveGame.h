#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TerrainTypes.h"
#include "TerrainSaveGame.generated.h"

/** Saved arena: the heightfield plus every placed ramp. */
UCLASS()
class UTerrainSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bump when the layout changes so old saves can be migrated or rejected. */
	UPROPERTY()
	int32 Version = 1;

	UPROPERTY()
	int32 VertsX = 0;

	UPROPERTY()
	int32 VertsY = 0;

	UPROPERTY()
	float CellSize = 100.f;

	UPROPERTY()
	TArray<float> Heights;

	UPROPERTY()
	TArray<FRampRecord> Ramps;
};
