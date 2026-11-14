#include "MonsterTruckGameMode.h"
#include "MonsterTruckController.h"
#include "TerrainActor.h"
#include "GameFramework/SpectatorPawn.h"
#include "Engine/World.h"

AMonsterTruckGameMode::AMonsterTruckGameMode()
{
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	PlayerControllerClass = AMonsterTruckController::StaticClass();
}

void AMonsterTruckGameMode::StartPlay()
{
	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Terrain = World->SpawnActor<ATerrainActor>(ATerrainActor::StaticClass(), FTransform::Identity, Params);
	}

	Super::StartPlay();
}
