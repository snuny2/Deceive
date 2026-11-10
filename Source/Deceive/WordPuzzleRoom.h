#pragma once

#include "CoreMinimal.h"
#include "DeceiveArea.h"
#include "WordPuzzleRoom.generated.h"

class APushableBox;

/** One hole in the word, and the crate that belongs in it. */
USTRUCT()
struct FWordPuzzleSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FName ColourId;

	UPROPERTY()
	FColor Colour = FColor::White;

	UPROPERTY()
	TObjectPtr<APushableBox> Seated;
};

/**
 * A room with a word spelled out in desks across the floor, a few letters left
 * unfinished. The player shoves crates into the holes, each crate into the hole
 * of its own colour, and the far door opens once the word is whole.
 */
UCLASS()
class DECEIVE_API AWordPuzzleRoom : public ADeceiveArea
{
	GENERATED_BODY()

public:
	AWordPuzzleRoom();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetEntryLocation(FName EntryName) const override;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "400"))
	float RoomDepth = 1600.f;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "400"))
	float RoomWidth = 2800.f;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "50"))
	float DoorWidth = 300.f;

	/** Both doorways are in the far wall; these are their positions along it. */
	UPROPERTY(EditAnywhere, Category = "Room")
	float ExitDoorY = -1000.f;

	UPROPERTY(EditAnywhere, Category = "Room")
	float EntryDoorY = -400.f;

	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> FloorTexture;

	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> WallTexture;

	/** The desks the word is built out of. */
	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> DeskTexture;

	/** One desk, and so one cell of the letter grid. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "40"))
	float DeskCell = 130.f;

	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "10"))
	float DeskHeight = 90.f;

	/** How much of a crate has to be inside a hole before it drops in. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float SeatOverlapFraction = 0.8f;

	/** Set once the word is finished; the far door waits on it. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName SolvedFlag = TEXT("WordPuzzle_Solved");

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName SeatedDialogueId = TEXT("Crate_Seated");

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName SolvedDialogueId = TEXT("WordPuzzle_Solved");

	/** Where the two doorways lead, and under what name they arrive there. */
	UPROPERTY(EditAnywhere, Category = "Room")
	EDeceiveArea EntryArea = EDeceiveArea::Alley;

	UPROPERTY(EditAnywhere, Category = "Room")
	FName EntryTargetEntry = TEXT("FromPuzzle");

	UPROPERTY(EditAnywhere, Category = "Room")
	EDeceiveArea ExitArea = EDeceiveArea::NextStreet;

	UPROPERTY(EditAnywhere, Category = "Room")
	FName ExitTargetEntry = TEXT("FromPuzzle");

	/** The names arrivals use to come in through each doorway. */
	UPROPERTY(EditAnywhere, Category = "Room")
	FName EntryDoorName = TEXT("FromAlley");

	UPROPERTY(EditAnywhere, Category = "Room")
	FName ExitDoorName = TEXT("FromStreet");

protected:
	virtual void SpawnContents() override;

private:
	/** Middle of a cell of the letter grid, in the room's own space. */
	FVector CellLocation(int32 Row, int32 Column) const;

	/** Lays the desks out and notes where the holes are. Returns the next box. */
	int32 LayOutWord(int32 FirstBox);

	/** Drops crates into any slot they are sitting on, and opens up when all are. */
	void CheckSlots();

	void Say(FName DialogueId, bool bAsNotice);

	UPROPERTY(Transient)
	TArray<FWordPuzzleSlot> Slots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APushableBox>> Crates;

	bool bSolved = false;

	/** The check right after walking in only catches up; it says nothing. */
	bool bFirstCheck = true;
};
