#include "LockedRoomArea.h"

#include "DeceiveDataSubsystem.h"
#include "DeceiveLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "PressurePlate.h"
#include "SpriteCharacter.h"

namespace
{
	/** Two lengths of bars with the gate between them, then the gate itself. */
	constexpr int32 NumPuzzleBoxes = 3;

	// The room's own boxes come first; these are counted from the end of them.
	constexpr int32 LowerBarIndex = ARoomArea::NumRoomBoxes;
	constexpr int32 UpperBarIndex = ARoomArea::NumRoomBoxes + 1;
	constexpr int32 GateIndex = ARoomArea::NumRoomBoxes + 2;
}

ALockedRoomArea::ALockedRoomArea()
{
	// The room's own seven boxes are made by ARoomArea; these come after them.
	CreateBoxes(NumPuzzleBoxes);

	PrimaryActorTick.bCanEverTick = true;
	AreaId = EDeceiveArea::LockedRoom;
}

void ALockedRoomArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float BarY = RoomWidth * 0.5f - BarInset;
	const float HalfDepth = RoomDepth * 0.5f;
	const float WallZ = WallHeight * 0.5f;

	const float GateNear = GateCenterX - GateWidth * 0.5f;
	const float GateFar = GateCenterX + GateWidth * 0.5f;

	// The bars run the depth of the room, broken only where the gate is.
	const float LowerLength = FMath::Max(GateNear + HalfDepth, 1.f);
	LayOutBox(LowerBarIndex,
		FVector((GateNear - HalfDepth) * 0.5f, BarY, WallZ),
		FVector(LowerLength, WallThickness, WallHeight), WallTexture);

	const float UpperLength = FMath::Max(HalfDepth - GateFar, 1.f);
	LayOutBox(UpperBarIndex,
		FVector((GateFar + HalfDepth) * 0.5f, BarY, WallZ),
		FVector(UpperLength, WallThickness, WallHeight), WallTexture);

	LayOutBox(GateIndex,
		FVector(GateCenterX, BarY, WallZ),
		FVector(GateWidth, WallThickness, WallHeight), WallTexture);
}

void ALockedRoomArea::SpawnContents()
{
	Super::SpawnContents();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FVector2D& Spot : {FirstPlate, SecondPlate})
	{
		const FTransform SpawnTransform(FVector(Spot.X, Spot.Y, 10.f));
		if (APressurePlate* Plate = World->SpawnActor<APressurePlate>(APressurePlate::StaticClass(), SpawnTransform))
		{
			Plates.Add(Plate);
			SpawnedContents.Add(Plate);
		}
	}

}

void ALockedRoomArea::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Plates.Num() == 0)
	{
		return;
	}

	bool bAllPressed = true;
	bool bAnyNewlyPressed = false;
	bool bAnyNewlyReleased = false;

	for (APressurePlate* Plate : Plates)
	{
		if (!IsValid(Plate))
		{
			continue;
		}

		const bool bPressed = Plate->IsPressed();
		bAnyNewlyPressed |= (bPressed && !Plate->bWasPressed);
		bAnyNewlyReleased |= (!bPressed && Plate->bWasPressed);
		Plate->bWasPressed = bPressed;
		bAllPressed &= bPressed;
	}

	// Walking back into the room with the plates still weighed down should find
	// the gate as it was left, without announcing it again.
	if (bFirstCheck)
	{
		bFirstCheck = false;
		SetGateOpen(bAllPressed, false);
		return;
	}

	if (bAllPressed != bGateOpen)
	{
		// The gate's own line covers the plate that caused it as well, so the two
		// do not pile up on screen in the same frame.
		SetGateOpen(bAllPressed, true);
		return;
	}

	if (bAnyNewlyPressed)
	{
		Notice(PlateDialogueId);
	}
	else if (bAnyNewlyReleased)
	{
		Notice(PlateReleasedDialogueId);
	}
}

void ALockedRoomArea::SetGateOpen(bool bOpen, bool bSaySo)
{
	bGateOpen = bOpen;

	if (UStaticMeshComponent* Gate = Boxes.IsValidIndex(GateIndex) ? Boxes[GateIndex].Get() : nullptr)
	{
		Gate->SetHiddenInGame(bOpen);
		Gate->SetCollisionEnabled(bOpen
			? ECollisionEnabled::NoCollision
			: ECollisionEnabled::QueryAndPhysics);
	}

	UE_LOG(LogDeceive, Log, TEXT("Locked room: the gate is now %s."),
		bOpen ? TEXT("open") : TEXT("shut"));

	if (bSaySo)
	{
		Say(bOpen ? GateDialogueId : GateClosedDialogueId);
	}
}

ASpriteCharacter* ALockedRoomArea::GetPlayerCharacter() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	return PlayerController ? Cast<ASpriteCharacter>(PlayerController->GetPawn()) : nullptr;
}

void ALockedRoomArea::Say(FName DialogueId)
{
	ASpriteCharacter* PlayerCharacter = GetPlayerCharacter();

	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	if (!PlayerCharacter || !Data || DialogueId.IsNone())
	{
		return;
	}

	FDialogueData Dialogue;
	if (Data->FindDialogue(DialogueId, Dialogue))
	{
		PlayerCharacter->StartDialogue(Dialogue);
	}
}

void ALockedRoomArea::Notice(FName DialogueId)
{
	ASpriteCharacter* PlayerCharacter = GetPlayerCharacter();

	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	if (!PlayerCharacter || !Data || DialogueId.IsNone())
	{
		return;
	}

	// The notice box holds one line, so that is all a plate gets to say. The
	// wording still lives in Dialogues.json like everything else.
	FDialogueData Dialogue;
	if (Data->FindDialogue(DialogueId, Dialogue) && Dialogue.Lines.Num() > 0)
	{
		PlayerCharacter->ShowNotice(Dialogue.Lines[0]);
	}
}
