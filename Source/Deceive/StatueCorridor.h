#pragma once

#include "CoreMinimal.h"
#include "RoomArea.h"
#include "Statue.h"
#include "StatueCorridor.generated.h"

class AStatue;

/**
 * The long corridor, with three statues standing along it. They all have to be
 * looking the same way for the door at the top to open.
 */
UCLASS()
class DECEIVE_API AStatueCorridor : public ARoomArea
{
	GENERATED_BODY()

public:
	AStatueCorridor();

	virtual void Tick(float DeltaSeconds) override;

	/** The way every statue has to be looking. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	EStatueFacing RequiredFacing = EStatueFacing::Right;

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName SolvedFlag = TEXT("StatuePuzzle_Solved");

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName SolvedDialogueId = TEXT("StatuePuzzle_Solved");

	/** What is said when a statue is turned away and the door shuts again. */
	UPROPERTY(EditAnywhere, Category = "Statues")
	FName ClosedDialogueId = TEXT("StatuePuzzle_Closed");

	/** How far out from the left wall the statues stand. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	float StatueInset = 140.f;

protected:
	virtual void SpawnContents() override;

private:
	void CheckStatues();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStatue>> Statues;

	bool bSolved = false;

	/** The check right after walking in only catches up; it says nothing. */
	bool bFirstCheck = true;
};
