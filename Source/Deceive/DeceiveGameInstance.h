#pragma once

#include "CoreMinimal.h"
#include "DeceiveDialogueTypes.h"
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

	bool bGameFinished = false;
};
