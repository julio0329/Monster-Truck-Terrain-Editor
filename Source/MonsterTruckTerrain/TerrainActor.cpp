#include "TerrainActor.h"
#include "RampActor.h"
#include "TerrainSaveGame.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"

ATerrainActor::ATerrainActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

void ATerrainActor::BeginPlay()
{
	Super::BeginPlay();

	Heights.Init(0.f, GetVertsX() * GetVertsY());
	CreateChunks();
	MarkAllDirty();
	RebuildDirtyChunks();
}

FVector ATerrainActor::GetCenterWorld() const
{
	const FVector LocalCenter(ChunksX * ChunkCells * CellSize * 0.5f,
		ChunksY * ChunkCells * CellSize * 0.5f, 0.f);
	return GetActorTransform().TransformPosition(LocalCenter);
}

void ATerrainActor::CreateChunks()
{
	Chunks.Empty();
	Chunks.Reserve(ChunksX * ChunksY);

	for (int32 CY = 0; CY < ChunksY; ++CY)
	{
		for (int32 CX = 0; CX < ChunksX; ++CX)
		{
			UProceduralMeshComponent* Chunk = NewObject<UProceduralMeshComponent>(
				this, FName(*FString::Printf(TEXT("Chunk_%d_%d"), CX, CY)));
			Chunk->bUseAsyncCooking = true;
			Chunk->bUseComplexAsSimpleCollision = true;
			Chunk->SetCollisionProfileName(TEXT("BlockAll"));
			Chunk->RegisterComponent();
			Chunk->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			Chunks.Add(Chunk);
		}
	}

	DirtyChunks.Init(true, ChunksX * ChunksY);
}

void ATerrainActor::MarkAllDirty()
{
	DirtyChunks.Init(true, ChunksX * ChunksY);
}

float ATerrainActor::HeightAt(int32 X, int32 Y) const
{
	X = FMath::Clamp(X, 0, GetVertsX() - 1);
	Y = FMath::Clamp(Y, 0, GetVertsY() - 1);
	return Heights[Y * GetVertsX() + X];
}

void ATerrainActor::BuildChunk(int32 CX, int32 CY)
{
	const int32 CC = ChunkCells;
	const int32 N = CC + 1; // vertices per chunk side

	TArray<FVector> Verts;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<int32> Tris;
	Verts.Reserve(N * N);
	Normals.Reserve(N * N);
	UVs.Reserve(N * N);
	Tris.Reserve(CC * CC * 6);

	for (int32 Y = 0; Y < N; ++Y)
	{
		for (int32 X = 0; X < N; ++X)
		{
			const int32 GX = CX * CC + X;
			const int32 GY = CY * CC + Y;

			// Normals come from the global heightfield so chunk seams stay smooth.
			const float DX = (HeightAt(GX + 1, GY) - HeightAt(GX - 1, GY)) / (2.f * CellSize);
			const float DY = (HeightAt(GX, GY + 1) - HeightAt(GX, GY - 1)) / (2.f * CellSize);

			Verts.Add(FVector(GX * CellSize, GY * CellSize, HeightAt(GX, GY)));
			Normals.Add(FVector(-DX, -DY, 1.f).GetSafeNormal());
			UVs.Add(FVector2D(GX * 0.25f, GY * 0.25f));
		}
	}

	// Winding is clockwise seen from above, which is the front face in Unreal.
	for (int32 Y = 0; Y < CC; ++Y)
	{
		for (int32 X = 0; X < CC; ++X)
		{
			const int32 A = Y * N + X;
			const int32 B = Y * N + (X + 1);
			const int32 C = (Y + 1) * N + X;
			const int32 D = (Y + 1) * N + (X + 1);
			Tris.Add(A); Tris.Add(B); Tris.Add(D);
			Tris.Add(A); Tris.Add(D); Tris.Add(C);
		}
	}

	UProceduralMeshComponent* Chunk = Chunks[CY * ChunksX + CX];
	Chunk->CreateMeshSection(0, Verts, Tris, Normals, UVs,
		TArray<FColor>(), TArray<FProcMeshTangent>(), true);
}

void ATerrainActor::RebuildDirtyChunks()
{
	for (int32 CY = 0; CY < ChunksY; ++CY)
	{
		for (int32 CX = 0; CX < ChunksX; ++CX)
		{
			const int32 Index = CY * ChunksX + CX;
			if (DirtyChunks.IsValidIndex(Index) && DirtyChunks[Index])
			{
				BuildChunk(CX, CY);
				DirtyChunks[Index] = false;
			}
		}
	}
}

void ATerrainActor::ApplyBrush(const FVector& WorldLocation, ETerrainBrushMode Mode, float Radius,
	float Strength, float DeltaTime, float FlattenHeight)
{
	if (Heights.Num() == 0 || Radius <= 0.f)
	{
		return;
	}

	const int32 VX = GetVertsX();
	const int32 VY = GetVertsY();
	const FVector Local = GetActorTransform().InverseTransformPosition(WorldLocation);

	const float RadiusCells = Radius / CellSize;
	const float CenterX = Local.X / CellSize;
	const float CenterY = Local.Y / CellSize;

	const int32 MinX = FMath::Max(0, FMath::FloorToInt(CenterX - RadiusCells));
	const int32 MaxX = FMath::Min(VX - 1, FMath::CeilToInt(CenterX + RadiusCells));
	const int32 MinY = FMath::Max(0, FMath::FloorToInt(CenterY - RadiusCells));
	const int32 MaxY = FMath::Min(VY - 1, FMath::CeilToInt(CenterY + RadiusCells));
	if (MinX > MaxX || MinY > MaxY)
	{
		return;
	}

	// Smoothing reads the pre-stroke-tick values so the result doesn't depend on loop order.
	const TArray<float> Source = (Mode == ETerrainBrushMode::Smooth) ? Heights : TArray<float>();

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float Dist = FVector2D::Distance(
				FVector2D(X * CellSize, Y * CellSize), FVector2D(Local.X, Local.Y));
			if (Dist >= Radius)
			{
				continue;
			}

			const float T = 1.f - Dist / Radius;
			const float Weight = T * T * (3.f - 2.f * T); // smoothstep falloff
			float& H = Heights[Y * VX + X];

			switch (Mode)
			{
			case ETerrainBrushMode::Raise:
				H += Strength * Weight * DeltaTime;
				break;
			case ETerrainBrushMode::Lower:
				H -= Strength * Weight * DeltaTime;
				break;
			case ETerrainBrushMode::Smooth:
			{
				float Sum = 0.f;
				for (int32 OY = -1; OY <= 1; ++OY)
				{
					for (int32 OX = -1; OX <= 1; ++OX)
					{
						const int32 SX = FMath::Clamp(X + OX, 0, VX - 1);
						const int32 SY = FMath::Clamp(Y + OY, 0, VY - 1);
						Sum += Source[SY * VX + SX];
					}
				}
				const float Alpha = FMath::Clamp(Weight * Strength * 0.01f * DeltaTime, 0.f, 1.f);
				H = FMath::Lerp(H, Sum / 9.f, Alpha);
				break;
			}
			case ETerrainBrushMode::Flatten:
			{
				const float Alpha = FMath::Clamp(Weight * Strength * 0.01f * DeltaTime, 0.f, 1.f);
				H = FMath::Lerp(H, FlattenHeight, Alpha);
				break;
			}
			}

			H = FMath::Clamp(H, MinHeight, MaxHeight);
		}
	}

	// Mark every chunk that contains an edited vertex or whose normals read one
	// (one vertex of margin). A vertex on a chunk edge belongs to both chunks.
	const int32 A_X = FMath::Max(MinX - 1, 0);
	const int32 B_X = FMath::Min(MaxX + 1, VX - 1);
	const int32 A_Y = FMath::Max(MinY - 1, 0);
	const int32 B_Y = FMath::Min(MaxY + 1, VY - 1);

	const int32 ChunkMinX = FMath::Max(0, FMath::DivideAndRoundUp(A_X, ChunkCells) - 1);
	const int32 ChunkMaxX = FMath::Min(ChunksX - 1, B_X / ChunkCells);
	const int32 ChunkMinY = FMath::Max(0, FMath::DivideAndRoundUp(A_Y, ChunkCells) - 1);
	const int32 ChunkMaxY = FMath::Min(ChunksY - 1, B_Y / ChunkCells);

	for (int32 CY = ChunkMinY; CY <= ChunkMaxY; ++CY)
	{
		for (int32 CX = ChunkMinX; CX <= ChunkMaxX; ++CX)
		{
			DirtyChunks[CY * ChunksX + CX] = true;
		}
	}
}

ARampActor* ATerrainActor::SpawnRamp(const FTransform& Transform, const FRampParams& Params)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Deferred spawn so Params is set before OnConstruction builds the mesh.
	ARampActor* Ramp = World->SpawnActorDeferred<ARampActor>(
		ARampActor::StaticClass(), Transform, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Ramp)
	{
		return nullptr;
	}

	Ramp->Params = Params;
	Ramp->FinishSpawning(Transform);
	Ramps.Add(Ramp);
	return Ramp;
}

void ATerrainActor::ClearRamps()
{
	for (ARampActor* Ramp : Ramps)
	{
		if (IsValid(Ramp))
		{
			Ramp->Destroy();
		}
	}
	Ramps.Empty();
}

FTerrainSnapshot ATerrainActor::CaptureSnapshot() const
{
	FTerrainSnapshot Snapshot;
	Snapshot.Heights = Heights;
	for (const ARampActor* Ramp : Ramps)
	{
		if (IsValid(Ramp))
		{
			FRampRecord Record;
			Record.Transform = Ramp->GetActorTransform();
			Record.Params = Ramp->Params;
			Snapshot.Ramps.Add(Record);
		}
	}
	return Snapshot;
}

void ATerrainActor::ApplySnapshot(const FTerrainSnapshot& Snapshot)
{
	Heights = Snapshot.Heights;
	ClearRamps();
	for (const FRampRecord& Record : Snapshot.Ramps)
	{
		SpawnRamp(Record.Transform, Record.Params);
	}
	MarkAllDirty();
	RebuildDirtyChunks();
}

void ATerrainActor::PushUndo()
{
	UndoStack.Add(CaptureSnapshot());
	if (UndoStack.Num() > MaxUndoSteps)
	{
		UndoStack.RemoveAt(0);
	}
}

bool ATerrainActor::Undo()
{
	if (UndoStack.Num() == 0)
	{
		return false;
	}
	const FTerrainSnapshot Snapshot = UndoStack.Pop();
	ApplySnapshot(Snapshot);
	return true;
}

bool ATerrainActor::SaveArena(const FString& SlotName)
{
	UTerrainSaveGame* Save = Cast<UTerrainSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTerrainSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	const FTerrainSnapshot Snapshot = CaptureSnapshot();
	Save->VertsX = GetVertsX();
	Save->VertsY = GetVertsY();
	Save->CellSize = CellSize;
	Save->Heights = Snapshot.Heights;
	Save->Ramps = Snapshot.Ramps;

	return UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}

bool ATerrainActor::LoadArena(const FString& SlotName)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return false;
	}

	const UTerrainSaveGame* Save = Cast<UTerrainSaveGame>(
		UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!Save)
	{
		return false;
	}

	// A save made with a different grid size can't be applied to this terrain.
	if (Save->VertsX != GetVertsX() || Save->VertsY != GetVertsY()
		|| Save->Heights.Num() != GetVertsX() * GetVertsY())
	{
		return false;
	}

	FTerrainSnapshot Snapshot;
	Snapshot.Heights = Save->Heights;
	Snapshot.Ramps = Save->Ramps;
	ApplySnapshot(Snapshot);
	UndoStack.Empty();
	return true;
}
