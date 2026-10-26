#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrossAlley.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;

/** A cross-shaped alley built from box meshes, so no level asset is required. */
UCLASS()
class DECEIVE_API ACrossAlley : public AActor
{
	GENERATED_BODY()

public:
	ACrossAlley();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Walkable width of each corridor. */
	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "100"))
	float CorridorWidth = 400.f;

	/** Distance from the crossing centre to the end of each of the four arms. */
	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "200"))
	float ArmLength = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "100"))
	float WallHeight = 600.f;

	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "10"))
	float WallThickness = 100.f;

	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "5"))
	float FloorThickness = 20.f;

	/** Swap this to retexture the ground. */
	UPROPERTY(EditAnywhere, Category = "Alley|Texture")
	TObjectPtr<UTexture2D> FloorTexture;

	/** Swap this to retexture the walls. */
	UPROPERTY(EditAnywhere, Category = "Alley|Texture")
	TObjectPtr<UTexture2D> WallTexture;

	/** Optional override; defaults to Paper2D's MaskedUnlitSpriteMaterial. */
	UPROPERTY(EditAnywhere, Category = "Alley|Texture")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/**
	 * Stops the walls that sit between the fixed camera and the crossing from being
	 * drawn in game. They keep their collision, so they still block movement.
	 */
	UPROPERTY(EditAnywhere, Category = "Alley")
	bool bHideCameraSideWalls = true;

private:
	/** Index 0-1 floors, 2-9 corridor walls, 10-13 arm end caps. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Boxes;

	void LayOutBox(int32 Index, const FVector& Center, const FVector& Size, UTexture2D* Texture, bool bCameraSide = false);
};
