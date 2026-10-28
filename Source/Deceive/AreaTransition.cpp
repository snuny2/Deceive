#include "AreaTransition.h"

#include "Components/BoxComponent.h"
#include "DeceiveGameMode.h"
#include "DeceiveLog.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"

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
	ADeceiveGameMode* GameMode = World ? World->GetAuthGameMode<ADeceiveGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}

	UE_LOG(LogDeceive, Log, TEXT("Portal used: heading to %s, entry '%s'"),
		DeceiveAreaName(TargetArea), *TargetEntry.ToString());

	// Travelling destroys this doorway, so do it once this call stack is gone.
	const EDeceiveArea Area = TargetArea;
	const FName Entry = TargetEntry;
	World->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(GameMode, [GameMode, Area, Entry]()
		{
			GameMode->TravelTo(Area, Entry);
		}));
}
