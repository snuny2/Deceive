#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "DeceiveItemTypes.generated.h"

/** One interactable object, as described by Content/Data/Items.json. */
USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName ItemId;

	/** Name shown in the outliner. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FString Label;

	/** Wording on the interact prompt. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FText InteractPrompt;

	/** Which entry of Dialogues.json this object says. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName DialogueId;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float Size = 150.f;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	EDeceiveArea Area = EDeceiveArea::Alley;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FVector Location = FVector::ZeroVector;
};
