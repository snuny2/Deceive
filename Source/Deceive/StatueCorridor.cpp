#include "StatueCorridor.h"

#include "DeceiveDataSubsystem.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveGameInstance.h"
#include "DeceiveLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "SpriteCharacter.h"
#include "Statue.h"

AStatueCorridor::AStatueCorridor()
{
	PrimaryActorTick.bCanEverTick = true;
	AreaId = EDeceiveArea::NextStreet;
}

void AStatueCorridor::SpawnContents()
{
	Super::SpawnContents();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>();

	// Three of them, spread along the corridor against its left-hand wall.
	const FName Ids[] = {TEXT("Statue_Green"), TEXT("Statue_Red"), TEXT("Statue_Blue")};
	const FColor Colours[] = {FColor::Green, FColor::Red, FColor::Cyan};
	const float Spacing = RoomDepth / 4.f;
	const float StatueY = -RoomWidth * 0.5f + StatueInset;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FTransform SpawnTransform(
			FVector((1 - Index) * Spacing, StatueY, 110.f));

		AStatue* Statue = World->SpawnActorDeferred<AStatue>(AStatue::StaticClass(), SpawnTransform);
		if (!Statue)
		{
			continue;
		}

		Statue->StatueId = Ids[Index];
		Statue->Colour = Colours[Index];
		Statue->FinishSpawning(SpawnTransform);

		// Back the way the player left it, rather than facing up again.
		int32 Remembered = 0;
		Statue->SetFacing(Game && Game->GetRememberedStatueFacing(Ids[Index], Remembered)
			? static_cast<EStatueFacing>(FMath::Clamp(Remembered, 0, 3))
			: EStatueFacing::Up);

		Statues.Add(Statue);
		// Without this the statue outlives the corridor and turns up in the next area.
		SpawnedContents.Add(Statue);
	}
}

void AStatueCorridor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	CheckStatues();
}

void AStatueCorridor::CheckStatues()
{
	// Walking back in re-reads facings that were already set; that is not news.
	const bool bQuiet = bFirstCheck;
	bFirstCheck = false;

	if (Statues.Num() == 0)
	{
		return;
	}

	bool bAllFacing = true;
	for (const AStatue* Statue : Statues)
	{
		if (!IsValid(Statue) || Statue->GetFacing() != RequiredFacing)
		{
			bAllFacing = false;
			break;
		}
	}

	if (bAllFacing == bSolved)
	{
		return;
	}

	// The door follows the statues: turn one away and it shuts again.
	bSolved = bAllFacing;

	if (UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>())
	{
		if (!SolvedFlag.IsNone())
		{
			Game->SetFlag(SolvedFlag, bSolved ? 1 : 0);
		}
	}

	UE_LOG(LogDeceive, Log, TEXT("Statues: the door is now %s."),
		bSolved ? TEXT("open") : TEXT("shut"));

	// Shutting is news too: the player has just turned one of them away.
	const FName DialogueId = bSolved ? SolvedDialogueId : ClosedDialogueId;
	if (bQuiet || DialogueId.IsNone())
	{
		return;
	}

	const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	ASpriteCharacter* PlayerCharacter = PlayerController
		? Cast<ASpriteCharacter>(PlayerController->GetPawn())
		: nullptr;

	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;

	FDialogueData Dialogue;
	if (PlayerCharacter && Data && Data->FindDialogue(DialogueId, Dialogue))
	{
		PlayerCharacter->StartDialogue(Dialogue);
	}
}
