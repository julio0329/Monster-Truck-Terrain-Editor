#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainTypes.h"
#include "TerrainActor.generated.h"

class UProceduralMeshComponent;
class ARampActor;

/** A full copy of the editable state, used for undo. */
struct FTerrainSnapshot
{
	TArray<float> Heights;
	TArray<FRampRecord> Ramps;
};

/**
 * Editable heightfield terrain, split into chunks. Each chunk is its own
 * procedural mesh so a brush stroke only rebuilds the chunks it touches.
 * The terrain's corner is at the actor origin and it extends along +X and +Y.
 */
UCLASS()
class ATerrainActor : public AActor
{
	GENERATED_BODY()

public:
	ATerrainActor();

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "32"))
	int32 ChunksX = 4;

	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "32"))
	int32 ChunksY = 4;

	/** Cells along each side of one chunk. */
	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "4", ClampMax = "128"))
	int32 ChunkCells = 32;

	/** World size of one cell, in Unreal units (1 uu = 1 cm). */
	UPROPERTY(EditAnywhere, Category = "Terrain", meta = (ClampMin = "10"))
	float CellSize = 100.f;

	UPROPERTY(EditAnywhere, Category = "Terrain")
	float MinHeight = -500.f;

	UPROPERTY(EditAnywhere, Category = "Terrain")
	float MaxHeight = 3000.f;

	virtual void BeginPlay() override;

	int32 GetVertsX() const { return ChunksX * ChunkCells + 1; }
	int32 GetVertsY() const { return ChunksY * ChunkCells + 1; }

	/** World-space center of the terrain at ground level. */
	FVector GetCenterWorld() const;

	/**
	 * Applies a brush. Raise and Lower treat Strength as units per second.
	 * Smooth and Flatten treat Strength as a blend rate (Strength * 0.01 per second).
	 */
	void ApplyBrush(const FVector& WorldLocation, ETerrainBrushMode Mode, float Radius,
		float Strength, float DeltaTime, float FlattenHeight);

	/** Rebuilds the mesh and collision of every chunk touched since the last rebuild. */
	void RebuildDirtyChunks();

	ARampActor* SpawnRamp(const FTransform& Transform, const FRampParams& Params);

	/** Call before a stroke or placement so it can be undone. */
	void PushUndo();
	bool Undo();

	bool SaveArena(const FString& SlotName);
	bool LoadArena(const FString& SlotName);

private:
	void CreateChunks();
	void BuildChunk(int32 CX, int32 CY);
	void MarkAllDirty();
	float HeightAt(int32 X, int32 Y) const;

	FTerrainSnapshot CaptureSnapshot() const;
	void ApplySnapshot(const FTerrainSnapshot& Snapshot);
	void ClearRamps();

	UPROPERTY()
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TArray<TObjectPtr<UProceduralMeshComponent>> Chunks;

	UPROPERTY()
	TArray<TObjectPtr<ARampActor>> Ramps;

	TArray<float> Heights;
	TArray<bool> DirtyChunks;
	TArray<FTerrainSnapshot> UndoStack;

	static constexpr int32 MaxUndoSteps = 30;
};
