#pragma once

#include "CoreMinimal.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveSaveGame.h"
#include "Engine/GameInstance.h"
#include "DeceiveGameInstance.generated.h"

/**
 * Holds what the player chose. Lives on the GameInstance so it outlives levels
 * and pawns, which matters once the three protagonists get swapped around.
 */
UCLASS()
class DECEIVE_API UDeceiveGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Stores an answer, replacing any earlier answer to the same ChoiceId. */
	UFUNCTION(BlueprintCallable, Category = "Choices")
	void RecordChoice(const FChoiceRecord& Record);

	UFUNCTION(BlueprintPure, Category = "Choices")
	const TArray<FChoiceRecord>& GetChoiceRecords() const { return ChoiceRecords; }

	/** False when that question has not been answered yet. */
	UFUNCTION(BlueprintPure, Category = "Choices")
	bool TryGetChoice(FName ChoiceId, bool& bOutChoseB) const;

	UFUNCTION(BlueprintCallable, Category = "Choices")
	void ClearChoices();

	// --- Item placement -----------------------------------------------------

	/** Records one more copy of an item standing in the world; returns its id. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 PlaceItemInWorld(FName ItemId, EDeceiveArea Area, const FVector& Location);

	/** Takes one placed copy back out of the world. False when the id is unknown. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemovePlacedItem(int32 PlacedId);

	/** Every copy the player has put down, anywhere. */
	const TArray<FDeceivePlacedItem>& GetPlacedItems() const { return PlacedItems; }

	/** Where a puzzle crate was left, so leaving the room does not reset it. */
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void RememberCratePosition(FName CrateId, const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	bool GetRememberedCratePosition(FName CrateId, FVector& OutLocation) const;

	/** Which way a statue was left looking, for the same reason. */
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void RememberStatueFacing(FName StatueId, int32 Facing);

	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	bool GetRememberedStatueFacing(FName StatueId, int32& OutFacing) const;

	// --- Saving -------------------------------------------------------------

	/** How many save slots the screen can ever list. */
	static constexpr int32 MaxSaveSlots = 20;

	/** Writes the run into the first free slot. False when every slot is taken. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveToNewSlot();

	/** Writes the run into that slot, replacing whatever was there. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveToSlot(int32 Slot);

	/** Puts a saved run back, including moving the player to where it was left. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadFromSlot(int32 Slot);

	/** Every slot that holds something, oldest slot number first. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void GetSaveSlots(TArray<FDeceiveSaveInfo>& OutSlots) const;

	static FString MakeSlotName(int32 Slot) { return FString::Printf(TEXT("Deceive_%02d"), Slot); }

	// --- The player ---------------------------------------------------------

	virtual void Init() override;

	UFUNCTION(BlueprintPure, Category = "Player")
	int32 GetHealth() const { return Health; }

	/** Clamped to the maximum in Player.json, and never below zero. */
	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetHealth(int32 NewHealth);

	/** Empty means the words in Player.json still apply. */
	UFUNCTION(BlueprintPure, Category = "Player")
	FText GetStatus() const { return Status; }

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetStatus(const FText& NewStatus) { Status = NewStatus; }

	// --- Inventory ---------------------------------------------------------

	/** Slots the bag has. The screen draws them all, filled or not. */
	static constexpr int32 InventoryCapacity = 12;

	/** Slots per row on the inventory screen. */
	static constexpr int32 InventoryColumns = 6;

	/**
	 * Adds an item, one slot per copy, so the same thing can be held several
	 * times over. False when the bag is full.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddInventoryItem(FName ItemId);

	/** Ids in the order they were picked up. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FName>& GetInventory() const { return Inventory; }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasInventoryItem(FName ItemId) const { return Inventory.Contains(ItemId); }

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsInventoryFull() const { return Inventory.Num() >= InventoryCapacity; }

	/** Takes an item out of the bag. False when it was not in there. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveInventoryItem(FName ItemId);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	// --- Flags -------------------------------------------------------------
	// Anything the story needs to remember: answers, items picked up, doors
	// opened, counters. A choice also writes its answer here (0 = first option).

	UFUNCTION(BlueprintCallable, Category = "Flags")
	void SetFlag(FName FlagName, int32 Value = 1);

	/** DefaultValue is returned when the flag was never set. */
	UFUNCTION(BlueprintPure, Category = "Flags")
	int32 GetFlag(FName FlagName, int32 DefaultValue = 0) const;

	UFUNCTION(BlueprintPure, Category = "Flags")
	bool HasFlag(FName FlagName) const;

	UFUNCTION(BlueprintCallable, Category = "Flags")
	void ClearFlag(FName FlagName);

	UFUNCTION(BlueprintPure, Category = "Flags")
	const TMap<FName, int32>& GetFlags() const { return Flags; }

	/**
	 * Call this from whatever ends the game. Until it is called the summary stays
	 * hidden. Also reachable from the console as `EndGame` for testing.
	 */
	UFUNCTION(Exec, BlueprintCallable, Category = "Flow")
	void EndGame();

	UFUNCTION(BlueprintPure, Category = "Flow")
	bool IsGameFinished() const { return bGameFinished; }

private:
	UPROPERTY()
	TArray<FChoiceRecord> ChoiceRecords;

	UPROPERTY()
	TMap<FName, int32> Flags;

	UPROPERTY()
	TArray<FName> Inventory;

	UPROPERTY()
	int32 Health = 0;

	UPROPERTY()
	FText Status;

	UPROPERTY()
	TArray<FDeceivePlacedItem> PlacedItems;

	/** Where each puzzle crate was last left, by its colour id. */
	UPROPERTY()
	TMap<FName, FVector> CratePositions;

	/** Which way each statue was last left looking. */
	UPROPERTY()
	TMap<FName, int32> StatueFacings;

	/** Handed out by PlaceItemInWorld; never reused, so ids stay unambiguous. */
	UPROPERTY()
	int32 NextPlacedId = 1;

	bool bGameFinished = false;
};
