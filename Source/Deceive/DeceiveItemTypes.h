#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "DeceiveItemTypes.generated.h"

/**
 * One copy of an item the player has put down somewhere. There can be any number
 * of these for the same item, which is what lets several stones be laid out.
 */
USTRUCT(BlueprintType)
struct FDeceivePlacedItem
{
	GENERATED_BODY()

	/** Identifies this copy, so picking it up again removes the right one. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	int32 PlacedId = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	EDeceiveArea Area = EDeceiveArea::Alley;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FVector Location = FVector::ZeroVector;
};

/** The protagonist, as described by Content/Data/Player.json. */
USTRUCT(BlueprintType)
struct FDeceivePlayerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	FText Name;

	/** Asset path of the portrait; empty falls back to the HUD's placeholder. */
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	FString PortraitPath;

	UPROPERTY(BlueprintReadOnly, Category = "Player")
	int32 MaxHealth = 20;

	/** How the character is doing, in words. */
	UPROPERTY(BlueprintReadOnly, Category = "Player")
	FText Status;
};

/** One thing the player can carry, as described by Content/Data/Inventory.json. */
USTRUCT(BlueprintType)
struct FInventoryItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName ItemId;

	/** Shown beside the picture and in the slot list. */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText Name;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FText Description;

	/**
	 * Asset path of the picture, such as "/Game/Art/RustyKey.RustyKey". Empty, or
	 * a path that does not load, falls back to the HUD's placeholder.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FString TexturePath;

	/**
	 * Can be put down from the inventory. Items.json needs an entry of the same
	 * id, which is what gets built in the world.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bPlaceable = false;
};

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

	/**
	 * Entries of Dialogues.json this object can say, in priority order. The first
	 * one whose condition passes is used, so put the conditional ones first.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TArray<FName> DialogueIds;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	float Size = 150.f;

	/**
	 * Only ever appears where the player puts it down. Items like this are the
	 * world form of something carried, so no area builds one on its own.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	bool bPlacedOnly = false;

	/** Stands in the world without blocking the way; the player walks over it. */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	bool bWalkThrough = false;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	EDeceiveArea Area = EDeceiveArea::Alley;

	UPROPERTY(BlueprintReadOnly, Category = "Item")
	FVector Location = FVector::ZeroVector;
};
