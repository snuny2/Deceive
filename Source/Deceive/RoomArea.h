#pragma once

#include "CoreMinimal.h"
#include "DeceiveArea.h"
#include "RoomArea.generated.h"

class UTexture2D;

/**
 * A plain rectangular room with one doorway on its camera-facing side. Used for
 * the building interior and, for now, as the placeholder beyond the map edge.
 */
UCLASS()
class DECEIVE_API ARoomArea : public ADeceiveArea
{
	GENERATED_BODY()

public:
	ARoomArea();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual FVector GetEntryLocation(FName EntryName) const override;

	/** Across the screen. */
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "200"))
	float RoomWidth = 900.f;

	/** Into the screen. */
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "200"))
	float RoomDepth = 700.f;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "50"))
	float DoorWidth = 300.f;

	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> FloorTexture;

	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> WallTexture;

	/** Where the doorway leads back to. */
	UPROPERTY(EditAnywhere, Category = "Room")
	EDeceiveArea ReturnArea = EDeceiveArea::Alley;

	UPROPERTY(EditAnywhere, Category = "Room")
	FName ReturnEntry;

protected:
	virtual void SpawnContents() override;
};
