#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MonsterTruckGameMode.generated.h"

class ATerrainActor;

/** Spawns the editable terrain and gives the player a fly camera plus the editor controller. */
UCLASS()
class AMonsterTruckGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMonsterTruckGameMode();

	virtual void StartPlay() override;

	UPROPERTY()
	TObjectPtr<ATerrainActor> Terrain;
};
