#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveItemTypes.h"
#include "GameFramework/SaveGame.h"
#include "DeceiveSaveGame.generated.h"

/** One saved run: everything the story has remembered, and where the player stands. */
UCLASS()
class DECEIVE_API UDeceiveSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FDateTime SavedAt;

	UPROPERTY()
	TMap<FName, int32> Flags;

	UPROPERTY()
	TArray<FName> Inventory;

	UPROPERTY()
	TArray<FChoiceRecord> ChoiceRecords;

	UPROPERTY()
	TArray<FDeceivePlacedItem> PlacedItems;

	UPROPERTY()
	int32 NextPlacedId = 1;

	UPROPERTY()
	TMap<FName, FVector> CratePositions;

	UPROPERTY()
	TMap<FName, int32> StatueFacings;

	UPROPERTY()
	int32 Health = 0;

	UPROPERTY()
	FText Status;

	UPROPERTY()
	EDeceiveArea Area = EDeceiveArea::Alley;

	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;
};

/** What the save list shows about a slot, without loading the whole thing. */
USTRUCT(BlueprintType)
struct FDeceiveSaveInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 Slot = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime SavedAt;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	EDeceiveArea Area = EDeceiveArea::Alley;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 ItemCount = 0;
};
