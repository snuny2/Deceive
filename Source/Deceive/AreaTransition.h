#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "AreaTransition.generated.h"

class UBoxComponent;

/**
 * A doorway. Standing in it does nothing by itself: the player has to interact.
 * The prompt already says what happens, so there is no question to answer.
 */
UCLASS()
class DECEIVE_API AAreaTransition : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AAreaTransition();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractPrompt_Implementation() const override { return InteractPrompt; }

	/** Wording on the prompt while this doorway is in range. */
	UPROPERTY(EditAnywhere, Category = "Transition")
	FText InteractPrompt = FText::FromString(TEXT("이동한다"));

	UPROPERTY(EditAnywhere, Category = "Transition")
	EDeceiveArea TargetArea = EDeceiveArea::Alley;

	/** Which entrance of the target area the player arrives at. */
	UPROPERTY(EditAnywhere, Category = "Transition")
	FName TargetEntry;

	UPROPERTY(EditAnywhere, Category = "Transition")
	FVector Extent = FVector(60.f, 150.f, 120.f);

	/**
	 * Entry of Inventory.json the player must be carrying. Leave it None and the
	 * doorway is always open.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Lock")
	FName RequiredItem;

	/**
	 * Flag that has to be set before this doorway works, such as a puzzle being
	 * solved. Checked before the item is, and it is never used up.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Lock")
	FName RequiredFlag;

	/**
	 * Said instead of travelling while the item is missing, resolved like any
	 * other dialogue so conditions still apply.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Lock")
	TArray<FName> LockedDialogueIds;

	/**
	 * Said the one time the lock is opened, before travelling. "{item}" in a line
	 * is replaced with the name of the item that opened it.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Lock")
	TArray<FName> UnlockDialogueIds;

	/** The key is used up by opening the lock. */
	UPROPERTY(EditAnywhere, Category = "Transition|Lock")
	bool bConsumeRequiredItem = true;

	/**
	 * Set once the lock is open, and checked before the item is. Without it a key
	 * that gets used up would leave the door shut for good.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Lock")
	FName UnlockedFlag;

	/**
	 * Draws the trigger volume so it can be seen and adjusted. The colour comes
	 * from the destination. Compiled out of Shipping builds either way.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Debug")
	bool bShowDebugVolume = true;

	/** Thickness of the drawn edges. */
	UPROPERTY(EditAnywhere, Category = "Transition|Debug", meta = (ClampMin = "0"))
	float DebugLineThickness = 6.f;

private:
	/** Plays the locked line, when there is one to play. */
	void SayLocked(AActor* InteractingActor);

	/**
	 * Plays the line for opening the lock, and travels once it has been read.
	 * False when there is nothing to say, so the caller can travel straight away.
	 */
	bool SayUnlockedThenTravel(AActor* InteractingActor);

	/** Hands the travel to the game mode, fade and all. */
	void Travel();

	UPROPERTY(VisibleAnywhere, Category = "Transition")
	TObjectPtr<UBoxComponent> Trigger;
};
