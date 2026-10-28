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

	bool bGameFinished = false;
};
