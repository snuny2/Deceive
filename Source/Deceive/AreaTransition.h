#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "DeceiveDialogueTypes.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "AreaTransition.generated.h"

class UBoxComponent;

/**
 * A doorway. Standing in it does nothing by itself: the player has to interact,
 * answer the question and pick the option that travels.
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
	UPROPERTY(EditAnywhere, Category = "Transition|Dialogue")
	FText InteractPrompt = FText::FromString(TEXT("이동한다"));

	UPROPERTY(EditAnywhere, Category = "Transition")
	EDeceiveArea TargetArea = EDeceiveArea::Alley;

	/** Which entrance of the target area the player arrives at. */
	UPROPERTY(EditAnywhere, Category = "Transition")
	FName TargetEntry;

	UPROPERTY(EditAnywhere, Category = "Transition")
	FVector Extent = FVector(60.f, 150.f, 120.f);

	/**
	 * What the player is asked. The first option travels, the second stays put.
	 * Left empty, it is filled in from the destination.
	 */
	UPROPERTY(EditAnywhere, Category = "Transition|Dialogue")
	FDialogueData Dialogue;

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
	/** Fills Dialogue with wording that suits the destination. */
	void ApplyDefaultDialogue();

	UPROPERTY(VisibleAnywhere, Category = "Transition")
	TObjectPtr<UBoxComponent> Trigger;
};
