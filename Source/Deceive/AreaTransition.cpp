#include "AreaTransition.h"

#include "Components/BoxComponent.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveGameInstance.h"
#include "DeceiveGameMode.h"
#include "DeceiveLog.h"
#include "DrawDebugHelpers.h"
#include "SpriteCharacter.h"

AAreaTransition::AAreaTransition()
{
	PrimaryActorTick.bCanEverTick = true;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	// Overlaps are what lets the player's interaction sphere find this doorway.
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);
}

void AAreaTransition::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Trigger->SetBoxExtent(Extent);
}

void AAreaTransition::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bShowDebugVolume)
	{
		return;
	}

	// Drawn in the foreground so the volume stays visible through the walls it
	// usually sits inside.
	DrawDebugBox(GetWorld(), GetActorLocation(), Extent, DeceiveAreaColor(TargetArea),
		false, -1.f, SDPG_Foreground, DebugLineThickness);
}

void AAreaTransition::Interact_Implementation(AActor* InteractingActor)
{
	UWorld* World = GetWorld();
	if (!World || !World->GetAuthGameMode<ADeceiveGameMode>())
	{
		return;
	}

	UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();

	// A flag gate is the puzzle's doing, not the player's: no item is spent and
	// nothing is unlocked for good, it simply will not budge until it is set.
	// The value matters, not just that the flag was written once: a puzzle that
	// comes undone sets its flag back to 0 and the door has to shut again.
	if (!RequiredFlag.IsNone() && (!GameInstance || GameInstance->GetFlag(RequiredFlag) == 0))
	{
		SayLocked(InteractingActor);
		return;
	}

	const bool bAlreadyUnlocked = !UnlockedFlag.IsNone() && GameInstance && GameInstance->HasFlag(UnlockedFlag);

	if (!RequiredItem.IsNone() && !bAlreadyUnlocked)
	{
		if (!GameInstance || !GameInstance->HasInventoryItem(RequiredItem))
		{
			SayLocked(InteractingActor);
			return;
		}

		// The lock stays open afterwards, which matters when the key is used up.
		if (!UnlockedFlag.IsNone())
		{
			GameInstance->SetFlag(UnlockedFlag, 1);
		}

		if (bConsumeRequiredItem)
		{
			GameInstance->RemoveInventoryItem(RequiredItem);
		}

		if (SayUnlockedThenTravel(InteractingActor))
		{
			return;
		}
	}

	Travel();
}

void AAreaTransition::Travel()
{
	UWorld* World = GetWorld();
	ADeceiveGameMode* GameMode = World ? World->GetAuthGameMode<ADeceiveGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}

	UE_LOG(LogDeceive, Log, TEXT("Portal used: heading to %s, entry '%s'"),
		DeceiveAreaName(TargetArea), *TargetEntry.ToString());

	// The travel itself happens once the fade has covered the screen, so this
	// doorway is still alive when the call returns.
	GameMode->TravelToWithFade(TargetArea, TargetEntry);
}

bool AAreaTransition::SayUnlockedThenTravel(AActor* InteractingActor)
{
	ASpriteCharacter* PlayerCharacter = Cast<ASpriteCharacter>(InteractingActor);
	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	if (!PlayerCharacter || !Data)
	{
		return false;
	}

	FDialogueData Dialogue;
	FName DialogueId;
	if (!Data->ResolveDialogue(UnlockDialogueIds, Dialogue, DialogueId))
	{
		return false;
	}

	// Lines name the key through "{item}", so renaming it in Inventory.json is
	// enough and the wording never goes stale.
	FInventoryItemData Item;
	const FString ItemName = Data->FindInventoryItem(RequiredItem, Item)
		? Item.Name.ToString()
		: RequiredItem.ToString();

	for (FText& Line : Dialogue.Lines)
	{
		Line = FText::FromString(Line.ToString().Replace(TEXT("{item}"), *ItemName));
	}

	PlayerCharacter->StartDialogueThen(Dialogue,
		FSimpleDelegate::CreateWeakLambda(this, [this]() { Travel(); }));
	return true;
}

void AAreaTransition::SayLocked(AActor* InteractingActor)
{
	UE_LOG(LogDeceive, Log, TEXT("Door to %s is locked: '%s' is not in the bag."),
		DeceiveAreaName(TargetArea), *RequiredItem.ToString());

	ASpriteCharacter* PlayerCharacter = Cast<ASpriteCharacter>(InteractingActor);
	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	if (!PlayerCharacter || !Data)
	{
		return;
	}

	FDialogueData Dialogue;
	FName DialogueId;
	if (Data->ResolveDialogue(LockedDialogueIds, Dialogue, DialogueId))
	{
		PlayerCharacter->StartDialogue(Dialogue);
	}
}
