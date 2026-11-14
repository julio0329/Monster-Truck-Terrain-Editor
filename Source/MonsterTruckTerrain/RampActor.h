#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainTypes.h"
#include "RampActor.generated.h"

class UProceduralMeshComponent;

/** A solid jump ramp generated from FRampParams. Drive direction is local +X. */
UCLASS()
class ARampActor : public AActor
{
	GENERATED_BODY()

public:
	ARampActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ramp")
	FRampParams Params;

	/** Regenerates the mesh and collision from Params. */
	UFUNCTION(BlueprintCallable, Category = "Ramp")
	void Rebuild();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(VisibleAnywhere, Category = "Ramp")
	TObjectPtr<UProceduralMeshComponent> Mesh;
};
