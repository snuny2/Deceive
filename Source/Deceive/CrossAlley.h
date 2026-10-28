#pragma once

#include "CoreMinimal.h"
#include "DeceiveArea.h"
#include "CrossAlley.generated.h"

class AInteractableBox;
class UTexture2D;

/** A cross-shaped alley built from box meshes, so no level asset is required. */
UCLASS()
class DECEIVE_API ACrossAlley : public ADeceiveArea
{
	GENERATED_BODY()

public:
	ACrossAlley();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual FVector GetEntryLocation(FName EntryName) const override;

	/** Walkable width of each corridor. */
	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "100"))
	float CorridorWidth = 400.f;

	/** Distance from the crossing centre to the end of each of the four arms. */
	UPROPERTY(EditAnywhere, Category = "Alley", meta = (ClampMin = "200"))
	float ArmLength = 1200.f;

	/** Doorway into the building, cut into the far side of the -Y arm. */
	UPROPERTY(EditAnywhere, Category = "Alley|Doors", meta = (ClampMin = "50"))
	float DoorWidth = 300.f;

	/** Where along the -Y arm that doorway sits. */
	UPROPERTY(EditAnywhere, Category = "Alley|Doors")
	float DoorCenterY = -700.f;

	/** Swap this to retexture the ground. */
	UPROPERTY(EditAnywhere, Category = "Alley|Texture")
	TObjectPtr<UTexture2D> FloorTexture;

	/** Swap this to retexture the walls. */
	UPROPERTY(EditAnywhere, Category = "Alley|Texture")
	TObjectPtr<UTexture2D> WallTexture;

	/** Test target dropped at the crossing. */
	UPROPERTY(EditAnywhere, Category = "Alley")
	TSubclassOf<AInteractableBox> TestBoxClass;

protected:
	virtual void SpawnContents() override;
};
