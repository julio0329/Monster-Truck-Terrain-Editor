#include "MonsterTruckController.h"
#include "MonsterTruckGameMode.h"
#include "TerrainActor.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	const TCHAR* ModeName(ETerrainBrushMode Mode)
	{
		switch (Mode)
		{
		case ETerrainBrushMode::Raise:   return TEXT("Raise");
		case ETerrainBrushMode::Lower:   return TEXT("Lower");
		case ETerrainBrushMode::Smooth:  return TEXT("Smooth");
		case ETerrainBrushMode::Flatten: return TEXT("Flatten");
		}
		return TEXT("?");
	}

	const FString SaveSlot = TEXT("Arena1");
}

AMonsterTruckController::AMonsterTruckController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AMonsterTruckController::AddYawInput(float Val)
{
	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		Super::AddYawInput(Val);
	}
}

void AMonsterTruckController::AddPitchInput(float Val)
{
	if (IsInputKeyDown(EKeys::RightMouseButton))
	{
		Super::AddPitchInput(Val);
	}
}

void AMonsterTruckController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!Terrain)
	{
		if (const AMonsterTruckGameMode* GameMode = GetWorld()->GetAuthGameMode<AMonsterTruckGameMode>())
		{
			Terrain = GameMode->Terrain;
		}
	}
	if (!Terrain)
	{
		return;
	}

	// Put the camera above the arena once both the pawn and terrain exist.
	if (!bCameraPlaced && GetPawn())
	{
		GetPawn()->SetActorLocation(Terrain->GetCenterWorld() + FVector(-3500.f, 0.f, 3000.f));
		SetControlRotation(FRotator(-40.f, 0.f, 0.f));
		bCameraPlaced = true;
	}

	HandleKeys();
	HandleBrush(DeltaTime);
	ShowStatus();
}

void AMonsterTruckController::SetStatus(const FString& Message)
{
	StatusMessage = Message;
	StatusUntil = GetWorld()->GetTimeSeconds() + 3.0;
}

void AMonsterTruckController::HandleKeys()
{
	if (WasInputKeyJustPressed(EKeys::One))   { Mode = ETerrainBrushMode::Raise; }
	if (WasInputKeyJustPressed(EKeys::Two))   { Mode = ETerrainBrushMode::Lower; }
	if (WasInputKeyJustPressed(EKeys::Three)) { Mode = ETerrainBrushMode::Smooth; }
	if (WasInputKeyJustPressed(EKeys::Four))  { Mode = ETerrainBrushMode::Flatten; }

	if (WasInputKeyJustPressed(EKeys::LeftBracket))  { BrushRadius = FMath::Clamp(BrushRadius - 100.f, 100.f, 3000.f); }
	if (WasInputKeyJustPressed(EKeys::RightBracket)) { BrushRadius = FMath::Clamp(BrushRadius + 100.f, 100.f, 3000.f); }
	if (WasInputKeyJustPressed(EKeys::Hyphen))       { BrushStrength = FMath::Clamp(BrushStrength - 50.f, 50.f, 2000.f); }
	if (WasInputKeyJustPressed(EKeys::Equals))       { BrushStrength = FMath::Clamp(BrushStrength + 50.f, 50.f, 2000.f); }

	if (WasInputKeyJustPressed(EKeys::Z)) { RampParams.Height = FMath::Clamp(RampParams.Height - 50.f, 50.f, 1500.f); }
	if (WasInputKeyJustPressed(EKeys::X)) { RampParams.Height = FMath::Clamp(RampParams.Height + 50.f, 50.f, 1500.f); }
	if (WasInputKeyJustPressed(EKeys::C)) { RampParams.Length = FMath::Clamp(RampParams.Length - 100.f, 300.f, 4000.f); }
	if (WasInputKeyJustPressed(EKeys::V)) { RampParams.Length = FMath::Clamp(RampParams.Length + 100.f, 300.f, 4000.f); }
	if (WasInputKeyJustPressed(EKeys::B)) { RampParams.Width = FMath::Clamp(RampParams.Width - 100.f, 200.f, 2000.f); }
	if (WasInputKeyJustPressed(EKeys::N)) { RampParams.Width = FMath::Clamp(RampParams.Width + 100.f, 200.f, 2000.f); }
	if (WasInputKeyJustPressed(EKeys::K)) { RampParams.CurvePower = FMath::Clamp(RampParams.CurvePower - 0.25f, 0.5f, 4.f); }
	if (WasInputKeyJustPressed(EKeys::L)) { RampParams.CurvePower = FMath::Clamp(RampParams.CurvePower + 0.25f, 0.5f, 4.f); }

	if (WasInputKeyJustPressed(EKeys::R))
	{
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			Terrain->PushUndo();
			// Face the camera's heading so driving "forward" goes up the ramp.
			const FRotator Facing(0.f, GetControlRotation().Yaw, 0.f);
			Terrain->SpawnRamp(FTransform(Facing, Hit.Location), RampParams);
			SetStatus(TEXT("Ramp placed"));
		}
	}

	if (WasInputKeyJustPressed(EKeys::U))
	{
		SetStatus(Terrain->Undo() ? TEXT("Undo") : TEXT("Nothing to undo"));
	}
	if (WasInputKeyJustPressed(EKeys::F5))
	{
		SetStatus(Terrain->SaveArena(SaveSlot) ? TEXT("Arena saved") : TEXT("Save failed"));
	}
	if (WasInputKeyJustPressed(EKeys::F9))
	{
		SetStatus(Terrain->LoadArena(SaveSlot) ? TEXT("Arena loaded") : TEXT("Load failed (no save, or grid size changed)"));
	}
}

void AMonsterTruckController::HandleBrush(float DeltaTime)
{
	FHitResult Hit;
	const bool bHit = GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (bHit)
	{
		// Brush outline on the ground under the cursor.
		DrawDebugCircle(GetWorld(), Hit.Location + FVector(0.f, 0.f, 20.f), BrushRadius, 48,
			FColor::Yellow, false, -1.f, 0, 8.f, FVector(1.f, 0.f, 0.f), FVector(0.f, 1.f, 0.f), false);
	}

	const bool bPainting = IsInputKeyDown(EKeys::LeftMouseButton)
		&& !IsInputKeyDown(EKeys::RightMouseButton) && bHit;

	if (!bPainting)
	{
		bStrokeActive = false;
		return;
	}

	if (!bStrokeActive)
	{
		bStrokeActive = true;
		Terrain->PushUndo();
		// The Flatten brush levels toward the height where the stroke started.
		FlattenHeight = Hit.Location.Z - Terrain->GetActorLocation().Z;
	}

	ETerrainBrushMode Effective = Mode;
	if (Mode == ETerrainBrushMode::Raise && IsInputKeyDown(EKeys::LeftAlt))
	{
		Effective = ETerrainBrushMode::Lower;
	}

	Terrain->ApplyBrush(Hit.Location, Effective, BrushRadius, BrushStrength, GetWorld()->GetDeltaSeconds(), FlattenHeight);
	Terrain->RebuildDirtyChunks();
}

void AMonsterTruckController::ShowStatus()
{
	if (!GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(1001, 0.f, FColor::Yellow, FString::Printf(
		TEXT("Brush: %s | Radius %.0f | Strength %.0f   (1-4 mode, [ ] radius, - = strength, Alt+LMB lowers)"),
		ModeName(Mode), BrushRadius, BrushStrength));

	GEngine->AddOnScreenDebugMessage(1002, 0.f, FColor::Cyan, FString::Printf(
		TEXT("Ramp: length %.0f | width %.0f | height %.0f | curve %.2f   (R place, Z/X height, C/V length, B/N width, K/L curve)"),
		RampParams.Length, RampParams.Width, RampParams.Height, RampParams.CurvePower));

	GEngine->AddOnScreenDebugMessage(1003, 0.f, FColor::White,
		TEXT("U undo | F5 save | F9 load | hold RMB + WASD/Space to fly"));

	if (GetWorld()->GetTimeSeconds() < StatusUntil)
	{
		GEngine->AddOnScreenDebugMessage(1004, 0.f, FColor::Green, StatusMessage);
	}
}
