#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PressurePlate.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;

/**
 * A flat plate that counts as pressed while the player, or anything the player
 * has put down, is standing on it. It holds no puzzle logic of its own: the area
 * that spawned it watches the plates and decides what their state means.
 */
UCLASS()
class DECEIVE_API APressurePlate : public AActor
{
	GENERATED_BODY()

public:
	APressurePlate();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** True while something is weighing it down. Worked out on the spot. */
	UFUNCTION(BlueprintPure, Category = "Puzzle")
	bool IsPressed() const;

	/** How wide the plate is, across both axes. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "20"))
	float PlateSize = 200.f;

	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "4"))
	float PlateThickness = 20.f;

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	TObjectPtr<UTexture2D> PlateTexture;

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** What the watching area saw last time, so it can spot the change. */
	bool bWasPressed = false;

private:
	/** Overlap only: the plate is something to stand on, not to bump into. */
	UPROPERTY(VisibleAnywhere, Category = "Puzzle")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category = "Puzzle")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
