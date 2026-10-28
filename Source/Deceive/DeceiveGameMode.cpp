#include "DeceiveGameMode.h"

#include "CrossAlley.h"
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

	// Both rooms are the same actor; they differ only in where their door leads.
	const TSubclassOf<ARoomArea> RoomClass = (Area == EDeceiveArea::Building) ? BuildingClass : NextStreetClass;
	if (!RoomClass)
	{
		return nullptr;
	}

	ARoomArea* Room = World->SpawnActorDeferred<ARoomArea>(RoomClass, FTransform::Identity);
	if (!Room)
	{
		return nullptr;
	}

	Room->ReturnArea = EDeceiveArea::Alley;
	Room->ReturnEntry = (Area == EDeceiveArea::Building) ? TEXT("FromBuilding") : TEXT("FromWest");

	if (Area == EDeceiveArea::NextStreet)
	{
		Room->RoomWidth = NextStreetWidth;
		Room->RoomDepth = NextStreetDepth;
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
