#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "GameFramework/Actor.h"
#include "DeceiveArea.generated.h"

class AAreaTransition;
class AInteractableBox;
class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;

/**
 * Base for the procedurally built areas. Each one owns its box meshes and
 * whatever it spawns inside itself, so swapping areas is a destroy and a spawn.
 */
UCLASS(Abstract)
class DECEIVE_API ADeceiveArea : public AActor
{
	GENERATED_BODY()

public:
	ADeceiveArea();

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	/** Where the player should stand when arriving through the named entrance. */
	virtual FVector GetEntryLocation(FName EntryName) const;


	/** Optional override; defaults to Paper2D's OpaqueUnlitSpriteMaterial. */
	UPROPERTY(EditAnywhere, Category = "Area|Texture")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/**
	 * Stops the walls that sit between the fixed camera and the player from being
	 * drawn in game. They keep their collision, so they still block movement.
	 */
	UPROPERTY(EditAnywhere, Category = "Area")
	bool bHideCameraSideWalls = true;

	UPROPERTY(EditAnywhere, Category = "Area", meta = (ClampMin = "10"))
	float WallHeight = 600.f;

	UPROPERTY(EditAnywhere, Category = "Area", meta = (ClampMin = "10"))
	float WallThickness = 100.f;

	UPROPERTY(EditAnywhere, Category = "Area", meta = (ClampMin = "5"))
	float FloorThickness = 20.f;

	/** Which entry of EDeceiveArea this is; decides which items belong here. */
	UPROPERTY(EditAnywhere, Category = "Area")
	EDeceiveArea AreaId = EDeceiveArea::Alley;

	/** Actor class the item table is spawned as. */
	UPROPERTY(EditAnywhere, Category = "Area")
	TSubclassOf<AInteractableBox> ItemClass;

protected:
	/** Call from the constructor; box meshes cannot be created any later. */
	void CreateBoxes(int32 Count);

	void LayOutBox(int32 Index, const FVector& Center, const FVector& Size, UTexture2D* Texture, bool bCameraSide = false);

	/**
	 * An invisible wall. Doorways are real holes in the geometry, so without one
	 * the player can simply walk through and off the edge of the area.
	 */
	void LayOutBlocker(int32 Index, const FVector& Center, const FVector& Size);

	/**
	 * Spawns whatever lives inside this area. Called once, on BeginPlay. The base
	 * version spawns this area's entries from Items.json; overrides should call it.
	 */
	virtual void SpawnContents();

	AAreaTransition* SpawnTransition(const FVector& Location, const FVector& Extent, EDeceiveArea TargetArea, FName TargetEntry);

	/** Tracks spawned actors so they die with the area. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedContents;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Boxes;
};
