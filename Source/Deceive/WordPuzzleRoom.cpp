#include "WordPuzzleRoom.h"

#include "AreaTransition.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveGameInstance.h"
#include "DeceiveLog.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "PushableBox.h"
#include "SpriteCharacter.h"

namespace
{
	/** Floor, three far-wall segments, three plain walls, two doorway blockers. */
	constexpr int32 NumStructuralBoxes = 9;

	/** How much of a square of CrateSize is inside a square of SlotSize, 0 to 1. */
	float OverlapFraction(const FVector& CrateCenter, float CrateSize,
		const FVector& SlotCenter, float SlotSize)
	{
		const float HalfSpan = (CrateSize + SlotSize) * 0.5f;
		const float OverlapX = HalfSpan - FMath::Abs(CrateCenter.X - SlotCenter.X);
		const float OverlapY = HalfSpan - FMath::Abs(CrateCenter.Y - SlotCenter.Y);
		if (OverlapX <= 0.f || OverlapY <= 0.f || CrateSize <= 0.f)
		{
			return 0.f;
		}

		const float Inside = FMath::Min(OverlapX, CrateSize) * FMath::Min(OverlapY, CrateSize);
		return Inside / (CrateSize * CrateSize);
	}

	constexpr int32 FloorIndex = 0;
	constexpr int32 FirstBackIndex = 1;
	constexpr int32 FrontIndex = 4;
	constexpr int32 LeftIndex = 5;
	constexpr int32 RightIndex = 6;
	constexpr int32 FirstBlockerIndex = 7;
	constexpr int32 FirstDeskIndex = 9;

	/**
	 * The word, a column of characters per letter. '#' is a desk, '.' is the space
	 * inside or beside a letter, and a digit is a hole a crate of that colour has
	 * to fill. Letters are three cells wide with one cell between them.
	 */
	const TCHAR* WordRows[] =
	{
		TEXT("### ### ### #.#"),
		TEXT("#.# #.# #.. ###"),
		TEXT("#.# ### ### ###"),
		TEXT("#.# #.. #.. #.#"),
		TEXT("#1# 2.. #3# #.#"),
	};

	constexpr int32 WordRowCount = UE_ARRAY_COUNT(WordRows);
	constexpr int32 WordColumnCount = 15;

	/** Counting the desks, so the box count is never a guess. */
	int32 CountCharacter(TCHAR Wanted)
	{
		int32 Count = 0;
		for (int32 Row = 0; Row < WordRowCount; ++Row)
		{
			for (int32 Column = 0; Column < WordColumnCount; ++Column)
			{
				Count += (WordRows[Row][Column] == Wanted) ? 1 : 0;
			}
		}

		return Count;
	}

	FColor SlotColour(TCHAR Digit)
	{
		switch (Digit)
		{
		case TEXT('1'):	return FColor::Red;
		case TEXT('2'):	return FColor::Green;
		default:		return FColor::Cyan;
		}
	}

	FName SlotColourId(TCHAR Digit)
	{
		switch (Digit)
		{
		case TEXT('1'):	return TEXT("Red");
		case TEXT('2'):	return TEXT("Green");
		default:		return TEXT("Cyan");
		}
	}
}

AWordPuzzleRoom::AWordPuzzleRoom()
{
	// Nine boxes hold the room up; the rest are the desks the word is made of.
	CreateBoxes(NumStructuralBoxes + CountCharacter(TEXT('#')));

	PrimaryActorTick.bCanEverTick = true;
	AreaId = EDeceiveArea::PuzzleRoom;
}

FVector AWordPuzzleRoom::CellLocation(int32 Row, int32 Column) const
{
	// The word is centred across the room, with open floor above and below it so
	// crates can be walked around the outside.
	const float FirstY = -(WordColumnCount - 1) * DeskCell * 0.5f;
	const float FirstX = (WordRowCount - 1) * DeskCell * 0.5f;

	return FVector(FirstX - Row * DeskCell, FirstY + Column * DeskCell, DeskHeight * 0.5f);
}

void AWordPuzzleRoom::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float HalfDepth = RoomDepth * 0.5f;
	const float HalfWidth = RoomWidth * 0.5f;
	const float WallZ = WallHeight * 0.5f;
	const float OuterHalfWidth = HalfWidth + WallThickness;

	LayOutBox(FloorIndex, FVector(0.f, 0.f, -FloorThickness * 0.5f),
		FVector(RoomDepth, RoomWidth, FloorThickness), FloorTexture);

	// The far wall carries both doorways, so it goes down in three lengths.
	const float HalfDoor = DoorWidth * 0.5f;
	const float BackX = HalfDepth + WallThickness * 0.5f;
	const float LeftEdge = -OuterHalfWidth;
	const float RightEdge = OuterHalfWidth;

	const float FirstDoorY = FMath::Min(ExitDoorY, EntryDoorY);
	const float SecondDoorY = FMath::Max(ExitDoorY, EntryDoorY);

	const float Spans[3][2] =
	{
		{LeftEdge, FirstDoorY - HalfDoor},
		{FirstDoorY + HalfDoor, SecondDoorY - HalfDoor},
		{SecondDoorY + HalfDoor, RightEdge},
	};

	for (int32 Segment = 0; Segment < 3; ++Segment)
	{
		const float Length = FMath::Max(Spans[Segment][1] - Spans[Segment][0], 1.f);
		LayOutBox(FirstBackIndex + Segment,
			FVector(BackX, (Spans[Segment][0] + Spans[Segment][1]) * 0.5f, WallZ),
			FVector(WallThickness, Length, WallHeight), WallTexture);
	}

	// Front wall faces the camera, so it is there for collision only.
	LayOutBox(FrontIndex, FVector(-BackX, 0.f, WallZ),
		FVector(WallThickness, OuterHalfWidth * 2.f, WallHeight), WallTexture, true);

	LayOutBox(LeftIndex, FVector(0.f, -HalfWidth - WallThickness * 0.5f, WallZ),
		FVector(RoomDepth, WallThickness, WallHeight), WallTexture);
	LayOutBox(RightIndex, FVector(0.f, HalfWidth + WallThickness * 0.5f, WallZ),
		FVector(RoomDepth, WallThickness, WallHeight), WallTexture);

	// Doorways are real holes, so both need filling in.
	LayOutBlocker(FirstBlockerIndex,
		FVector(BackX, ExitDoorY, WallZ), FVector(WallThickness, DoorWidth, WallHeight));
	LayOutBlocker(FirstBlockerIndex + 1,
		FVector(BackX, EntryDoorY, WallZ), FVector(WallThickness, DoorWidth, WallHeight));

	LayOutWord(FirstDeskIndex);
}

int32 AWordPuzzleRoom::LayOutWord(int32 FirstBox)
{
	int32 Index = FirstBox;
	Slots.Reset();

	for (int32 Row = 0; Row < WordRowCount; ++Row)
	{
		for (int32 Column = 0; Column < WordColumnCount; ++Column)
		{
			const TCHAR Cell = WordRows[Row][Column];
			if (Cell == TEXT('.') || Cell == TEXT(' '))
			{
				continue;
			}

			const FVector Location = CellLocation(Row, Column);

			if (Cell != TEXT('#'))
			{
				// A hole: no desk here until a crate of the right colour fills it.
				FWordPuzzleSlot Slot;
				Slot.Location = Location;
				Slot.Colour = SlotColour(Cell);
				Slot.ColourId = SlotColourId(Cell);
				Slots.Add(Slot);
				continue;
			}

			// Each desk is its own box, so the word is built up rather than being
			// one solid shape.
			LayOutBox(Index++, Location, FVector(DeskCell, DeskCell, DeskHeight), DeskTexture);
		}
	}

	return Index;
}

FVector AWordPuzzleRoom::GetEntryLocation(FName EntryName) const
{
	// Just inside whichever doorway was used, clear of its trigger.
	const float Inside = RoomDepth * 0.5f - 250.f;
	const float DoorY = (EntryName == ExitDoorName) ? ExitDoorY : EntryDoorY;

	return FVector(Inside, DoorY, 120.f);
}

void AWordPuzzleRoom::SpawnContents()
{
	Super::SpawnContents();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float BackX = RoomDepth * 0.5f + WallThickness * 0.5f;
	const FVector PortalExtent(60.f, DoorWidth * 0.5f, 120.f);
	const float Inset = WallThickness * 0.5f + 40.f;

	// The way back to where the player came from.
	SpawnTransition(FVector(BackX - Inset, EntryDoorY, 100.f), PortalExtent, EntryArea, EntryTargetEntry);

	// The way on, which stays shut until the word is finished.
	if (AAreaTransition* Exit = SpawnTransition(
		FVector(BackX - Inset, ExitDoorY, 100.f), PortalExtent, ExitArea, ExitTargetEntry))
	{
		Exit->RequiredFlag = SolvedFlag;
		Exit->LockedDialogueIds.Add(TEXT("WordPuzzle_Locked"));
	}

	// The crates wait below the word, in an order that does not match the holes.
	const FName CrateColours[] = {TEXT("Cyan"), TEXT("Red"), TEXT("Green")};
	const FColor CrateMarks[] = {FColor::Cyan, FColor::Red, FColor::Green};

	const float CrateX = -RoomDepth * 0.5f + 220.f;
	const float CrateSize = DeskCell * 0.85f;
	for (int32 Crate = 0; Crate < 3; ++Crate)
	{
		// Resting on the floor rather than sunk into it, unless the player has
		// already shoved it somewhere, in which case it stays where it was left.
		FVector CrateLocation(CrateX, (Crate - 1) * DeskCell * 4.f, CrateSize * 0.5f);
		if (UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>())
		{
			Game->GetRememberedCratePosition(CrateColours[Crate], CrateLocation);
		}

		const FTransform SpawnTransform(CrateLocation);
		APushableBox* Box = World->SpawnActorDeferred<APushableBox>(
			APushableBox::StaticClass(), SpawnTransform);
		if (!Box)
		{
			continue;
		}

		Box->ColourId = CrateColours[Crate];
		Box->Colour = CrateMarks[Crate];
		Box->BoxSize = CrateSize;
		Box->FinishSpawning(SpawnTransform);

		Crates.Add(Box);
		SpawnedContents.Add(Box);
	}
}

void AWordPuzzleRoom::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The holes are drawn in their colour, which is what says where each crate
	// belongs while there are no textures to say it.
	for (const FWordPuzzleSlot& Slot : Slots)
	{
		DrawDebugBox(GetWorld(), Slot.Location, FVector(DeskCell * 0.5f), Slot.Colour,
			false, -1.f, SDPG_Foreground, Slot.Seated ? 10.f : 4.f);
	}

	if (!bSolved)
	{
		CheckSlots();
	}
}

void AWordPuzzleRoom::CheckSlots()
{
	bool bAllSeated = true;
	bool bAnyNewlySeated = false;

	// Crates are put back where they were left, so the first check of a visit
	// re-seats them. That is not news: it has all been said once already.
	const bool bQuiet = bFirstCheck;
	bFirstCheck = false;

	for (FWordPuzzleSlot& Slot : Slots)
	{
		if (IsValid(Slot.Seated))
		{
			continue;
		}

		for (APushableBox* Crate : Crates)
		{
			// Colour is the whole of the matching: a crate will not settle into a
			// hole that is not its own, however exactly it is lined up.
			if (!IsValid(Crate) || Crate->IsSeated() || Crate->ColourId != Slot.ColourId)
			{
				continue;
			}

			if (OverlapFraction(Crate->GetActorLocation(), Crate->BoxSize,
				Slot.Location, DeskCell) >= SeatOverlapFraction)
			{
				Crate->Seat(Slot.Location);
				Slot.Seated = Crate;
				bAnyNewlySeated = true;
				break;
			}
		}

		bAllSeated &= (Slot.Seated != nullptr);
	}

	if (!bAllSeated)
	{
		if (bAnyNewlySeated && !bQuiet)
		{
			Say(SeatedDialogueId, true);
		}

		return;
	}

	bSolved = true;

	if (UDeceiveGameInstance* GameInstance = Cast<UDeceiveGameInstance>(GetGameInstance()))
	{
		if (!SolvedFlag.IsNone())
		{
			GameInstance->SetFlag(SolvedFlag, 1);
		}
	}

	UE_LOG(LogDeceive, Log, TEXT("Word puzzle: the word is finished, the far door is open."));

	if (!bQuiet)
	{
		Say(SolvedDialogueId, false);
	}
}

void AWordPuzzleRoom::Say(FName DialogueId, bool bAsNotice)
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	ASpriteCharacter* PlayerCharacter = PlayerController
		? Cast<ASpriteCharacter>(PlayerController->GetPawn())
		: nullptr;

	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	if (!PlayerCharacter || !Data || DialogueId.IsNone())
	{
		return;
	}

	FDialogueData Dialogue;
	if (!Data->FindDialogue(DialogueId, Dialogue) || Dialogue.Lines.Num() == 0)
	{
		return;
	}

	// A crate settling is a small thing and should not stop the player walking;
	// finishing the word is worth a box.
	if (bAsNotice)
	{
		PlayerCharacter->ShowNotice(Dialogue.Lines[0]);
	}
	else
	{
		PlayerCharacter->StartDialogue(Dialogue);
	}
}
