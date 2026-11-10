#include "DeceiveGameMode.h"

#include "CrossAlley.h"
#include "DeceiveArea.h"
#include "LockedRoomArea.h"
#include "StatueCorridor.h"
#include "WordPuzzleRoom.h"
#include "DeceiveHUD.h"
#include "DeceiveLog.h"
#include "GameFramework/PlayerStart.h"
#include "RoomArea.h"
#include "SpriteCharacter.h"

ADeceiveGameMode::ADeceiveGameMode()
{
	DefaultPawnClass = ASpriteCharacter::StaticClass();
	HUDClass = ADeceiveHUD::StaticClass();
	AlleyClass = ACrossAlley::StaticClass();
	BuildingClass = ARoomArea::StaticClass();
	NextStreetClass = ARoomArea::StaticClass();
	LockedRoomClass = ALockedRoomArea::StaticClass();
	PuzzleRoomClass = AWordPuzzleRoom::StaticClass();
	StatueCorridorClass = AStatueCorridor::StaticClass();
	StatueHallClass = ARoomArea::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

EDeceiveArea ADeceiveGameMode::GetCurrentAreaId() const
{
	return IsValid(CurrentArea) ? CurrentArea->AreaId : EDeceiveArea::Alley;
}

float ADeceiveGameMode::GetFadeAlpha() const
{
	// Smoothstep, so the black eases in and out instead of arriving at a constant rate.
	return FadeProgress * FadeProgress * (3.f - 2.f * FadeProgress);
}

void ADeceiveGameMode::TravelToWithFade(EDeceiveArea Area, FName EntryName)
{
	if (FadePhase != EFadePhase::None)
	{
		return;
	}

	if (FadeDuration <= 0.f)
	{
		TravelTo(Area, EntryName);
		return;
	}

	PendingArea = Area;
	PendingEntry = EntryName;
	FadePhase = EFadePhase::Out;
	FadeProgress = 0.f;
}

void ADeceiveGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FadePhase == EFadePhase::None)
	{
		return;
	}

	const float Step = DeltaSeconds / FadeDuration;

	if (FadePhase == EFadePhase::Out)
	{
		FadeProgress = FMath::Min(FadeProgress + Step, 1.f);
		if (FadeProgress >= 1.f)
		{
			// Nothing is visible now, so the swap can happen without being seen.
			TravelTo(PendingArea, PendingEntry);
			FadePhase = EFadePhase::In;
		}
		return;
	}

	FadeProgress = FMath::Max(FadeProgress - Step, 0.f);
	if (FadeProgress <= 0.f)
	{
		FadePhase = EFadePhase::None;
	}
}

void ADeceiveGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	CurrentArea = SpawnArea(EDeceiveArea::Alley);
	if (!CurrentArea)
	{
		return;
	}

	World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(),
		FTransform(CurrentArea->GetEntryLocation(NAME_None)));
}

void ADeceiveGameMode::LabelArea(ADeceiveArea* Area, EDeceiveArea AreaId)
{
#if WITH_EDITOR
	if (Area)
	{
		Area->SetActorLabel(FString::Printf(TEXT("Area_%s"), DeceiveAreaName(AreaId)));
	}
#endif
}

ADeceiveArea* ADeceiveGameMode::SpawnArea(EDeceiveArea Area)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (Area == EDeceiveArea::Alley)
	{
		ACrossAlley* Alley = AlleyClass ? World->SpawnActor<ACrossAlley>(AlleyClass, FTransform::Identity) : nullptr;
		LabelArea(Alley, Area);
		return Alley;
	}

	if (Area == EDeceiveArea::PuzzleRoom)
	{
		AWordPuzzleRoom* Puzzle = PuzzleRoomClass
			? World->SpawnActor<AWordPuzzleRoom>(PuzzleRoomClass, FTransform::Identity)
			: nullptr;
		LabelArea(Puzzle, Area);
		return Puzzle;
	}

	// The remaining rooms are all the same actor; they differ only in size and in
	// where their doors lead back to.
	TSubclassOf<ARoomArea> RoomClass = NextStreetClass;
	if (Area == EDeceiveArea::Building)
	{
		RoomClass = BuildingClass;
	}
	else if (Area == EDeceiveArea::LockedRoom)
	{
		RoomClass = LockedRoomClass;
	}
	else if (Area == EDeceiveArea::NextStreet)
	{
		RoomClass = StatueCorridorClass;
	}
	else if (Area == EDeceiveArea::StatueHall)
	{
		RoomClass = StatueHallClass;
	}

	if (!RoomClass)
	{
		return nullptr;
	}

	ARoomArea* Room = World->SpawnActorDeferred<ARoomArea>(RoomClass, FTransform::Identity);
	if (!Room)
	{
		return nullptr;
	}

	Room->AreaId = Area;
	Room->ReturnArea = EDeceiveArea::Alley;
	switch (Area)
	{
	case EDeceiveArea::Building:	Room->ReturnEntry = TEXT("FromBuilding"); break;
	case EDeceiveArea::LockedRoom:	Room->ReturnEntry = TEXT("FromEast"); break;
	case EDeceiveArea::StatueHall:	Room->ReturnEntry = TEXT("FromHall"); break;
	default:						Room->ReturnEntry = TEXT("FromWest"); break;
	}

	// Both side rooms sit beside a corridor, so their doors are on the wall that
	// faces it rather than on the camera-facing one.
	if (Area == EDeceiveArea::NextStreet)
	{
		Room->RoomWidth = NextStreetWidth;
		Room->RoomDepth = NextStreetDepth;
		Room->DoorSide = ERoomDoorSide::Right;
	}
	else if (Area == EDeceiveArea::LockedRoom)
	{
		Room->RoomWidth = LockedRoomWidth;
		Room->RoomDepth = LockedRoomDepth;
		Room->DoorSide = ERoomDoorSide::Left;
	}
	else if (Area == EDeceiveArea::StatueHall)
	{
		// The wide room sits above both of them, so its ways out are at the bottom.
		// The middle one goes down the top arm of the crossing.
		Room->RoomWidth = StatueHallWidth;
		Room->RoomDepth = StatueHallDepth;
		Room->DoorSide = ERoomDoorSide::Front;

		// And a small one at the left-hand end, back down into the corridor. It is
		// the statue door, so it is shut whenever they are not all looking right.
		FRoomDoorway ToCorridor;
		ToCorridor.Side = ERoomDoorSide::Front;
		ToCorridor.Offset = -(StatueHallWidth * 0.5f - Room->DoorWidth);
		ToCorridor.Area = EDeceiveArea::NextStreet;
		ToCorridor.Entry = TEXT("FromHall");
		ToCorridor.EntryName = TEXT("FromStreet");
		ToCorridor.RequiredFlag = TEXT("StatuePuzzle_Solved");
		// From this side the player cannot see the statues, so the line about which
		// way they are looking would be telling them something they cannot check.
		ToCorridor.LockedDialogueId = TEXT("StatueDoor_Hall");
		Room->ExtraDoors.Add(ToCorridor);
	}

	if (Area == EDeceiveArea::NextStreet)
	{
		// The bottom of the long street opens into the word room, which is how
		// the player comes back out of it.
		FRoomDoorway ToWordRoom;
		ToWordRoom.Side = ERoomDoorSide::Front;
		ToWordRoom.Area = EDeceiveArea::PuzzleRoom;
		ToWordRoom.Entry = TEXT("FromStreet");
		ToWordRoom.EntryName = TEXT("FromPuzzle");
		Room->ExtraDoors.Add(ToWordRoom);

		// The top of it is shut until the statues are all looking the same way.
		FRoomDoorway ToHall;
		ToHall.Side = ERoomDoorSide::Back;
		ToHall.Area = EDeceiveArea::StatueHall;
		ToHall.Entry = TEXT("FromStreet");
		ToHall.EntryName = TEXT("FromHall");
		ToHall.RequiredFlag = TEXT("StatuePuzzle_Solved");
		ToHall.LockedDialogueId = TEXT("StatueDoor");
		Room->ExtraDoors.Add(ToHall);
	}

	Room->FinishSpawning(FTransform::Identity);
	LabelArea(Room, Area);

	return Room;
}

void ADeceiveGameMode::TravelTo(EDeceiveArea Area, FName EntryName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (IsValid(CurrentArea))
	{
		CurrentArea->Destroy();
	}

	CurrentArea = SpawnArea(Area);
	if (!CurrentArea)
	{
		UE_LOG(LogDeceive, Warning, TEXT("No area class set for %s."), DeceiveAreaName(Area));
		return;
	}

	const FVector EntryLocation = CurrentArea->GetEntryLocation(EntryName);

	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (Pawn)
	{
		Pawn->SetActorLocation(EntryLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

	UE_LOG(LogDeceive, Log, TEXT("Travelled to %s, entry '%s', placed at %s."),
		DeceiveAreaName(Area), *EntryName.ToString(), *EntryLocation.ToString());
}
