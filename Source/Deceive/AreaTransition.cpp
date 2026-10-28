#include "AreaTransition.h"

#include "Components/BoxComponent.h"
#include "DeceiveGameMode.h"
#include "DeceiveLog.h"
#include "DrawDebugHelpers.h"
#include "SpriteCharacter.h"
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

	if (Dialogue.Question.IsEmpty())
	{
		ApplyDefaultDialogue();
	}
}

void AAreaTransition::ApplyDefaultDialogue()
{
	switch (TargetArea)
	{
	case EDeceiveArea::Building:
		Dialogue.Speaker = FText::FromString(TEXT("건물 입구"));
		Dialogue.Question = FText::FromString(TEXT("건물에 들어갈까?"));
		Dialogue.ChoiceA.Label = FText::FromString(TEXT("들어간다"));
		Dialogue.ChoiceB.Label = FText::FromString(TEXT("안 들어간다"));
		break;

	case EDeceiveArea::Alley:
		Dialogue.Speaker = FText::FromString(TEXT("출구"));
		Dialogue.Question = FText::FromString(TEXT("밖으로 나갈까?"));
		Dialogue.ChoiceA.Label = FText::FromString(TEXT("나간다"));
		Dialogue.ChoiceB.Label = FText::FromString(TEXT("남아있는다"));
		break;

	default:
		Dialogue.Speaker = FText::FromString(TEXT("골목 끝"));
		Dialogue.Question = FText::FromString(TEXT("다음 장소로 이동할까?"));
		Dialogue.ChoiceA.Label = FText::FromString(TEXT("이동한다"));
		Dialogue.ChoiceB.Label = FText::FromString(TEXT("남아있는다"));
		break;
	}
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
	ASpriteCharacter* PlayerCharacter = Cast<ASpriteCharacter>(InteractingActor);
	UWorld* World = GetWorld();
	ADeceiveGameMode* GameMode = World ? World->GetAuthGameMode<ADeceiveGameMode>() : nullptr;

	if (!PlayerCharacter || !GameMode)
	{
		return;
	}

	const EDeceiveArea Area = TargetArea;
	const FName Entry = TargetEntry;
	TWeakObjectPtr<ADeceiveGameMode> WeakGameMode(GameMode);

	PlayerCharacter->StartDialogueWithHandler(Dialogue,
		[WeakGameMode, Area, Entry](bool bChoseB)
		{
			// The second option is always the one that stays put.
			if (bChoseB || !WeakGameMode.IsValid())
			{
				return;
			}

			UE_LOG(LogDeceive, Log, TEXT("Portal accepted: heading to %s, entry '%s'"),
				DeceiveAreaName(Area), *Entry.ToString());

			// Travelling destroys this doorway, so do it once this call stack is gone.
			ADeceiveGameMode* GameMode = WeakGameMode.Get();
			GameMode->GetWorld()->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateWeakLambda(GameMode, [GameMode, Area, Entry]()
				{
					GameMode->TravelTo(Area, Entry);
				}));
		});
}
