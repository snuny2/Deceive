#pragma once

#include "CoreMinimal.h"
#include "RoomArea.h"
#include "LockedRoomArea.generated.h"

class APressurePlate;

/**
 * The room behind the locked door, with its puzzle: a barred-off strip at the
 * far side, and two plates that have to be weighed down at the same time before
 * the gate in the bars opens. One body cannot do both, so something has to be
 * left standing on a plate.
 */
UCLASS()
class DECEIVE_API ALockedRoomArea : public ARoomArea
{
	GENERATED_BODY()

public:
	ALockedRoomArea();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Distance from the right-hand wall to the bars. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "100"))
	float BarInset = 300.f;

	/** Where the gate sits along the bars, and how wide it is. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	float GateCenterX = 350.f;

	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "50"))
	float GateWidth = 300.f;

	/** Where the two plates sit. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FVector2D FirstPlate = FVector2D(400.f, -300.f);

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FVector2D SecondPlate = FVector2D(-400.f, -300.f);

	/**
	 * Shown in the notice box when a plate takes weight. A notice rather than the
	 * dialogue box, so stepping on a plate does not stop the player walking.
	 */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName PlateDialogueId = TEXT("Plate_Click");

	/** Shown the same way when the weight comes off again. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName PlateReleasedDialogueId = TEXT("Plate_Release");

	/** Said when the last plate goes down and the gate gives way. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName GateDialogueId = TEXT("Gate_Open");

	/** Said when weight comes off a plate and the gate drops back. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName GateClosedDialogueId = TEXT("Gate_Close");

protected:
	virtual void SpawnContents() override;

private:
	/** The gate is only ever open while every plate is weighed down. */
	void SetGateOpen(bool bOpen, bool bSaySo);

	/** Shows one of this room's lines in the dialogue box. */
	void Say(FName DialogueId);

	/** Shows its first line in the notice box instead, which does not interrupt. */
	void Notice(FName DialogueId);

	/** The player, when there is one to talk to. */
	class ASpriteCharacter* GetPlayerCharacter() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APressurePlate>> Plates;

	bool bGateOpen = false;

	/** The first look at the plates settles the gate without announcing it. */
	bool bFirstCheck = true;
};
