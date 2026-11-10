#include "DeceiveGameInstance.h"

#include "DeceiveDataSubsystem.h"
#include "DeceiveGameMode.h"
#include "DeceiveLog.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void UDeceiveGameInstance::Init()
{
	Super::Init();

	// The subsystems come up during Super::Init, so the table is readable by now.
	if (const UDeceiveDataSubsystem* Data = GetSubsystem<UDeceiveDataSubsystem>())
	{
		Health = Data->GetPlayerData().MaxHealth;
		Status = Data->GetPlayerData().Status;
	}
}

void UDeceiveGameInstance::SetHealth(int32 NewHealth)
{
	int32 MaxHealth = NewHealth;
	if (const UDeceiveDataSubsystem* Data = GetSubsystem<UDeceiveDataSubsystem>())
	{
		MaxHealth = Data->GetPlayerData().MaxHealth;
	}

	Health = FMath::Clamp(NewHealth, 0, MaxHealth);
}

void UDeceiveGameInstance::SetFlag(FName FlagName, int32 Value)
{
	if (FlagName.IsNone())
	{
		return;
	}

	Flags.Add(FlagName, Value);
	UE_LOG(LogDeceive, Log, TEXT("Flag set: %s = %d"), *FlagName.ToString(), Value);
}

int32 UDeceiveGameInstance::GetFlag(FName FlagName, int32 DefaultValue) const
{
	const int32* Found = Flags.Find(FlagName);
	return Found ? *Found : DefaultValue;
}

bool UDeceiveGameInstance::HasFlag(FName FlagName) const
{
	return Flags.Contains(FlagName);
}

void UDeceiveGameInstance::ClearFlag(FName FlagName)
{
	Flags.Remove(FlagName);
}

void UDeceiveGameInstance::RecordChoice(const FChoiceRecord& Record)
{
	// The answer itself lives in the flags; the record is only for the summary.
	SetFlag(Record.ChoiceId, Record.bChoseB ? 1 : 0);

	if (!Record.ChoiceId.IsNone())
	{
		const int32 Existing = ChoiceRecords.IndexOfByPredicate(
			[&Record](const FChoiceRecord& Candidate) { return Candidate.ChoiceId == Record.ChoiceId; });

		if (Existing != INDEX_NONE)
		{
			ChoiceRecords[Existing] = Record;
			return;
		}
	}

	ChoiceRecords.Add(Record);

	UE_LOG(LogDeceive, Log, TEXT("Choice recorded: %s = %s"),
		*Record.ChoiceId.ToString(), *Record.ChosenLabel.ToString());
}

bool UDeceiveGameInstance::TryGetChoice(FName ChoiceId, bool& bOutChoseB) const
{
	if (!HasFlag(ChoiceId))
	{
		return false;
	}

	bOutChoseB = GetFlag(ChoiceId) != 0;
	return true;
}

void UDeceiveGameInstance::ClearChoices()
{
	ChoiceRecords.Reset();
	Flags.Reset();
	bGameFinished = false;
}

void UDeceiveGameInstance::EndGame()
{
	bGameFinished = true;
	UE_LOG(LogDeceive, Log, TEXT("Game finished with %d recorded choice(s)."), ChoiceRecords.Num());
}

bool UDeceiveGameInstance::AddInventoryItem(FName ItemId)
{
	if (ItemId.IsNone())
	{
		return false;
	}

	if (Inventory.Num() >= InventoryCapacity)
	{
		UE_LOG(LogDeceive, Warning, TEXT("Inventory is full (%d slots); '%s' was not picked up."),
			InventoryCapacity, *ItemId.ToString());
		return false;
	}

	Inventory.Add(ItemId);
	UE_LOG(LogDeceive, Log, TEXT("Picked up '%s' (%d/%d)."),
		*ItemId.ToString(), Inventory.Num(), InventoryCapacity);
	return true;
}

int32 UDeceiveGameInstance::PlaceItemInWorld(FName ItemId, EDeceiveArea Area, const FVector& Location)
{
	FDeceivePlacedItem Placed;
	Placed.PlacedId = NextPlacedId++;
	Placed.ItemId = ItemId;
	Placed.Area = Area;
	Placed.Location = Location;
	PlacedItems.Add(Placed);

	UE_LOG(LogDeceive, Log, TEXT("'%s' put down in %s at %s (%d placed in all)."),
		*ItemId.ToString(), DeceiveAreaName(Area), *Location.ToString(), PlacedItems.Num());

	return Placed.PlacedId;
}

void UDeceiveGameInstance::RememberCratePosition(FName CrateId, const FVector& Location)
{
	CratePositions.Add(CrateId, Location);
}

bool UDeceiveGameInstance::GetRememberedCratePosition(FName CrateId, FVector& OutLocation) const
{
	if (const FVector* Found = CratePositions.Find(CrateId))
	{
		OutLocation = *Found;
		return true;
	}

	return false;
}

void UDeceiveGameInstance::RememberStatueFacing(FName StatueId, int32 Facing)
{
	StatueFacings.Add(StatueId, Facing);
}

bool UDeceiveGameInstance::GetRememberedStatueFacing(FName StatueId, int32& OutFacing) const
{
	if (const int32* Found = StatueFacings.Find(StatueId))
	{
		OutFacing = *Found;
		return true;
	}

	return false;
}

bool UDeceiveGameInstance::RemovePlacedItem(int32 PlacedId)
{
	const int32 Index = PlacedItems.IndexOfByPredicate(
		[PlacedId](const FDeceivePlacedItem& Placed) { return Placed.PlacedId == PlacedId; });

	if (Index == INDEX_NONE)
	{
		return false;
	}

	PlacedItems.RemoveAt(Index);
	return true;
}

bool UDeceiveGameInstance::RemoveInventoryItem(FName ItemId)
{
	// RemoveSingle, not Remove: the bag can hold several of the same thing, and
	// only the one being used should leave it.
	if (Inventory.RemoveSingle(ItemId) == 0)
	{
		return false;
	}

	UE_LOG(LogDeceive, Log, TEXT("Used up '%s' (%d/%d left)."),
		*ItemId.ToString(), Inventory.Num(), InventoryCapacity);
	return true;
}

void UDeceiveGameInstance::ClearInventory()
{
	Inventory.Reset();
}

bool UDeceiveGameInstance::SaveToSlot(int32 Slot)
{
	UDeceiveSaveGame* Save = Cast<UDeceiveSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UDeceiveSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	Save->SavedAt = FDateTime::Now();
	Save->Flags = Flags;
	Save->Inventory = Inventory;
	Save->ChoiceRecords = ChoiceRecords;
	Save->PlacedItems = PlacedItems;
	Save->Health = Health;
	Save->Status = Status;
	Save->NextPlacedId = NextPlacedId;
	Save->CratePositions = CratePositions;
	Save->StatueFacings = StatueFacings;

	// Where the run is standing right now, so loading puts the player back.
	if (const UWorld* World = GetWorld())
	{
		if (const ADeceiveGameMode* GameMode = World->GetAuthGameMode<ADeceiveGameMode>())
		{
			Save->Area = GameMode->GetCurrentAreaId();
		}

		const APlayerController* PlayerController = World->GetFirstPlayerController();
		if (const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr)
		{
			Save->PlayerLocation = Pawn->GetActorLocation();
		}
	}

	if (!UGameplayStatics::SaveGameToSlot(Save, MakeSlotName(Slot), 0))
	{
		UE_LOG(LogDeceive, Warning, TEXT("Could not write save slot %d."), Slot);
		return false;
	}

	UE_LOG(LogDeceive, Log, TEXT("Saved to slot %d: %s, %d item(s), %d flag(s)."),
		Slot, DeceiveAreaName(Save->Area), Save->Inventory.Num(), Save->Flags.Num());
	return true;
}

bool UDeceiveGameInstance::SaveToNewSlot()
{
	for (int32 Slot = 0; Slot < MaxSaveSlots; ++Slot)
	{
		if (!UGameplayStatics::DoesSaveGameExist(MakeSlotName(Slot), 0))
		{
			return SaveToSlot(Slot);
		}
	}

	UE_LOG(LogDeceive, Warning, TEXT("Every one of the %d save slots is taken."), MaxSaveSlots);
	return false;
}

bool UDeceiveGameInstance::LoadFromSlot(int32 Slot)
{
	const UDeceiveSaveGame* Save = Cast<UDeceiveSaveGame>(
		UGameplayStatics::LoadGameFromSlot(MakeSlotName(Slot), 0));
	if (!Save)
	{
		UE_LOG(LogDeceive, Warning, TEXT("Save slot %d could not be read."), Slot);
		return false;
	}

	Flags = Save->Flags;
	Inventory = Save->Inventory;
	ChoiceRecords = Save->ChoiceRecords;
	PlacedItems = Save->PlacedItems;
	Health = Save->Health;
	Status = Save->Status;
	NextPlacedId = FMath::Max(Save->NextPlacedId, 1);
	CratePositions = Save->CratePositions;
	StatueFacings = Save->StatueFacings;
	bGameFinished = false;

	UWorld* World = GetWorld();
	ADeceiveGameMode* GameMode = World ? World->GetAuthGameMode<ADeceiveGameMode>() : nullptr;
	if (GameMode)
	{
		// Rebuilds the area, then the exact spot replaces the entrance it picked.
		GameMode->TravelTo(Save->Area, NAME_None);

		const APlayerController* PlayerController = World->GetFirstPlayerController();
		if (APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr)
		{
			Pawn->SetActorLocation(Save->PlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}

	UE_LOG(LogDeceive, Log, TEXT("Loaded slot %d: %s, %d item(s), %d flag(s)."),
		Slot, DeceiveAreaName(Save->Area), Inventory.Num(), Flags.Num());
	return true;
}

void UDeceiveGameInstance::GetSaveSlots(TArray<FDeceiveSaveInfo>& OutSlots) const
{
	OutSlots.Reset();

	for (int32 Slot = 0; Slot < MaxSaveSlots; ++Slot)
	{
		const FString SlotName = MakeSlotName(Slot);
		if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		{
			continue;
		}

		const UDeceiveSaveGame* Save = Cast<UDeceiveSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (!Save)
		{
			continue;
		}

		FDeceiveSaveInfo Info;
		Info.Slot = Slot;
		Info.SavedAt = Save->SavedAt;
		Info.Area = Save->Area;
		Info.ItemCount = Save->Inventory.Num();
		OutSlots.Add(Info);
	}
}
