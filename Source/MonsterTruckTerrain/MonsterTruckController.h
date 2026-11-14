#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TerrainTypes.h"
#include "MonsterTruckController.generated.h"

class ATerrainActor;

/**
 * Terrain editor controls. Uses direct key polling so no input assets are needed.
 *
 *   LMB            apply brush (Alt+LMB lowers while the Raise brush is active)
 *   1 2 3 4        brush: Raise, Lower, Smooth, Flatten
 *   [ ]            brush radius -/+
 *   - =            brush strength -/+
 *   R              place a ramp at the cursor, facing the camera direction
 *   Z X            ramp height -/+      C V  ramp length -/+
 *   B N            ramp width -/+       K L  ramp curve -/+
 *   U              undo
 *   F5 / F9        save / load arena
 *   Hold RMB       mouse-look, with W A S D Space to fly
 */
UCLASS()
class AMonsterTruckController : public APlayerController
{
	GENERATED_BODY()

public:
	AMonsterTruckController();

	virtual void PlayerTick(float DeltaTime) override;

	// Mouse-look only while the right mouse button is held, so the cursor stays usable.
	virtual void AddYawInput(float Val) override;
	virtual void AddPitchInput(float Val) override;

private:
	void HandleKeys();
	void HandleBrush(float DeltaTime);
	void ShowStatus();
	void SetStatus(const FString& Message);

	UPROPERTY()
	TObjectPtr<ATerrainActor> Terrain;

	ETerrainBrushMode Mode = ETerrainBrushMode::Raise;
	float BrushRadius = 600.f;
	float BrushStrength = 400.f;
	float FlattenHeight = 0.f;
	FRampParams RampParams;

	bool bCameraPlaced = false;
	bool bStrokeActive = false;

	FString StatusMessage;
	double StatusUntil = 0.0;
};
