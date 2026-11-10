#include "SpriteCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveGameInstance.h"
#include "DeceiveArea.h"
#include "DeceiveGameMode.h"
#include "DeceiveLog.h"
#include "DeceiveSpriteMaterial.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interactable.h"
#include "InteractableBox.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float PlaneMeshSize = 100.f;

	/**
	 * Engine's plane mesh lies in XY with its normal on +Z. This rotation stands it
	 * up so the normal faces -X, the texture's U axis runs along +Y and its V axis
	 * runs down. Yaw is the outermost term, so adding to it keeps that orientation
	 * while turning the quad towards the camera.
	 */
	const FRotator QuadBaseRotation(0.f, 90.f, -90.f);
	constexpr float QuadBaseNormalYaw = 180.f;
}

ASpriteCharacter::ASpriteCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	SpriteQuad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpriteQuad"));
	SpriteQuad->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	SpriteQuad->SetStaticMesh(PlaneMeshFinder.Object);
	SpriteQuad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteQuad->SetRelativeRotation(QuadBaseRotation);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(GetCapsuleComponent());
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 2700.f;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetWorldRotation(FRotator(-25.f, 0.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 40.f;
}

void ASpriteCharacter::BeginPlay()
{
	Super::BeginPlay();
	RefreshSprite();
	InteractionSphere->SetSphereRadius(InteractionRadius);
}

void ASpriteCharacter::RefreshSprite()
{
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SpriteQuad->SetRelativeLocation(FVector(0.f, 0.f, SpriteHeight * 0.5f - HalfHeight));
	SpriteQuad->SetRelativeScale3D(FVector(SpriteWidth / PlaneMeshSize, SpriteHeight / PlaneMeshSize, 1.f));

	if (UMaterialInstanceDynamic* Material = MakeTexturedMaterial(this, BaseMaterial, SpriteTexture))
	{
		SpriteQuad->SetMaterial(0, Material);
	}
}

void ASpriteCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Yaw-only billboarding: turn the quad's normal towards the active camera.
	const FVector ViewLocation = Camera->GetComponentLocation();
	const FVector ToView = ViewLocation - SpriteQuad->GetComponentLocation();
	if (ToView.SizeSquared2D() > KINDA_SMALL_NUMBER)
	{
		const float TargetNormalYaw = FMath::RadiansToDegrees(FMath::Atan2(ToView.Y, ToView.X));
		SpriteQuad->SetWorldRotation(FRotator(
			QuadBaseRotation.Pitch,
			QuadBaseRotation.Yaw + TargetNormalYaw - QuadBaseNormalYaw,
			QuadBaseRotation.Roll));
	}

	UpdateCurrentInteractable();
}

void ASpriteCharacter::UpdateCurrentInteractable()
{
	TArray<AActor*> Overlapping;
	InteractionSphere->GetOverlappingActors(Overlapping);

	AActor* Nearest = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* Candidate : Overlapping)
	{
		// Overlap lists can still carry an actor that was destroyed this frame.
		if (!IsValid(Candidate) || !Candidate->Implements<UInteractable>())
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Candidate->GetActorLocation(), GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Candidate;
		}
	}

	CurrentInteractable = Nearest;
}

void ASpriteCharacter::EnsureInputObjects()
{
	if (MappingContext)
	{
		return;
	}

	MoveAction = NewObject<UInputAction>(this, TEXT("MoveAction"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	// Movement shares the arrow keys with the menu cursor. Without this it would
	// consume them first and the cursor would never move.
	MoveAction->bConsumeInput = false;

	InteractAction = NewObject<UInputAction>(this, TEXT("InteractAction"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	InventoryAction = NewObject<UInputAction>(this, TEXT("InventoryAction"));
	InventoryAction->ValueType = EInputActionValueType::Boolean;

	CancelAction = NewObject<UInputAction>(this, TEXT("CancelAction"));
	CancelAction->ValueType = EInputActionValueType::Boolean;

	RunAction = NewObject<UInputAction>(this, TEXT("RunAction"));
	RunAction->ValueType = EInputActionValueType::Boolean;

	NavigateUpAction = NewObject<UInputAction>(this, TEXT("NavigateUpAction"));
	NavigateUpAction->ValueType = EInputActionValueType::Boolean;

	NavigateDownAction = NewObject<UInputAction>(this, TEXT("NavigateDownAction"));
	NavigateDownAction->ValueType = EInputActionValueType::Boolean;

	NavigateLeftAction = NewObject<UInputAction>(this, TEXT("NavigateLeftAction"));
	NavigateLeftAction->ValueType = EInputActionValueType::Boolean;

	NavigateRightAction = NewObject<UInputAction>(this, TEXT("NavigateRightAction"));
	NavigateRightAction->ValueType = EInputActionValueType::Boolean;

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("MappingContext"));

	// A key press arrives on X. Swizzle moves it to Y for the forward axis, and
	// negate flips it for the back and left keys.
	auto MapMove = [this](const FKey& Key, bool bSwizzle, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(MoveAction, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
	};

	MapMove(EKeys::W, true, false);
	MapMove(EKeys::S, true, true);
	MapMove(EKeys::D, false, false);
	MapMove(EKeys::A, false, true);
	MapMove(EKeys::Up, true, false);
	MapMove(EKeys::Down, true, true);
	MapMove(EKeys::Right, false, false);
	MapMove(EKeys::Left, false, true);

	// The stick already reports both axes, so it needs no modifiers.
	MappingContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	MappingContext->MapKey(InteractAction, EKeys::Z);
	MappingContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Bottom);

	MappingContext->MapKey(CancelAction, EKeys::X);
	MappingContext->MapKey(CancelAction, EKeys::Gamepad_FaceButton_Right);

	MappingContext->MapKey(InventoryAction, EKeys::C);
	MappingContext->MapKey(InventoryAction, EKeys::Gamepad_Special_Right);

	MappingContext->MapKey(RunAction, EKeys::LeftShift);
	MappingContext->MapKey(RunAction, EKeys::RightShift);
	MappingContext->MapKey(RunAction, EKeys::Gamepad_LeftShoulder);

	// The arrow keys also drive movement, but movement is blocked while a question
	// or the menu is up, so the two never act at the same time.
	MappingContext->MapKey(NavigateUpAction, EKeys::Up);
	MappingContext->MapKey(NavigateUpAction, EKeys::Gamepad_DPad_Up);
	MappingContext->MapKey(NavigateDownAction, EKeys::Down);
	MappingContext->MapKey(NavigateDownAction, EKeys::Gamepad_DPad_Down);
	MappingContext->MapKey(NavigateLeftAction, EKeys::Left);
	MappingContext->MapKey(NavigateLeftAction, EKeys::Gamepad_DPad_Left);
	MappingContext->MapKey(NavigateRightAction, EKeys::Right);
	MappingContext->MapKey(NavigateRightAction, EKeys::Gamepad_DPad_Right);
}

void ASpriteCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	EnsureInputObjects();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(MappingContext, 0);
	}
}

void ASpriteCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputObjects();

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpriteCharacter::Move);
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ASpriteCharacter::TryInteract);
		EnhancedInput->BindAction(InventoryAction, ETriggerEvent::Started, this, &ASpriteCharacter::ToggleMenu);
		EnhancedInput->BindAction(CancelAction, ETriggerEvent::Started, this, &ASpriteCharacter::Cancel);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Started, this, &ASpriteCharacter::StartRunning);
		EnhancedInput->BindAction(RunAction, ETriggerEvent::Completed, this, &ASpriteCharacter::StopRunning);
		EnhancedInput->BindAction(NavigateUpAction, ETriggerEvent::Started, this, &ASpriteCharacter::OnNavigateUp);
		EnhancedInput->BindAction(NavigateDownAction, ETriggerEvent::Started, this, &ASpriteCharacter::OnNavigateDown);
		EnhancedInput->BindAction(NavigateLeftAction, ETriggerEvent::Started, this, &ASpriteCharacter::OnNavigateLeft);
		EnhancedInput->BindAction(NavigateRightAction, ETriggerEvent::Started, this, &ASpriteCharacter::OnNavigateRight);
	}
}

void ASpriteCharacter::StartDialogue(const FDialogueData& InDialogue)
{
	if (InDialogue.Lines.Num() == 0 && InDialogue.Question.IsEmpty())
	{
		return;
	}

	ActiveDialogue = InDialogue;
	CurrentLines = InDialogue.Lines;
	LineIndex = 0;
	bDialogueActive = true;
	bAwaitingChoice = false;

	SelectedChoiceIndex = 0;

	// A dialogue that is nothing but a question opens straight on it.
	if (CurrentLines.Num() == 0)
	{
		bAwaitingChoice = true;
	}
}

void ASpriteCharacter::SayInventoryFull()
{
	FDialogueData Full;
	Full.Lines.Add(InventoryFullLine);
	StartDialogue(Full);
}

void ASpriteCharacter::StartDialogueThen(const FDialogueData& InDialogue, FSimpleDelegate OnClosed)
{
	StartDialogue(InDialogue);

	// Only held on to if the box actually opened; otherwise it would fire at the
	// end of whatever is said next.
	if (bDialogueActive)
	{
		DialogueClosedHandler = OnClosed;
	}
}

FText ASpriteCharacter::GetCurrentDialogueLine() const
{
	if (bAwaitingChoice)
	{
		return ActiveDialogue.Question;
	}

	return CurrentLines.IsValidIndex(LineIndex) ? CurrentLines[LineIndex] : FText::GetEmpty();
}

void ASpriteCharacter::AdvanceDialogue()
{
	++LineIndex;
	if (CurrentLines.IsValidIndex(LineIndex))
	{
		return;
	}

	// The run is over. Ask the question if one is still outstanding.
	if (!ActiveDialogue.Question.IsEmpty())
	{
		bAwaitingChoice = true;
		SelectedChoiceIndex = 0;
		return;
	}

	CloseDialogue();
}

void ASpriteCharacter::SelectChoice(bool bChoiceB)
{
	if (!bAwaitingChoice)
	{
		return;
	}

	const FDialogueChoice& Chosen = bChoiceB ? ActiveDialogue.ChoiceB : ActiveDialogue.ChoiceA;
	bool bFullBag = false;

	if (UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>())
	{
		for (const TPair<FName, int32>& Flag : Chosen.SetFlags)
		{
			GameInstance->SetFlag(Flag.Key, Flag.Value);
		}

		for (const FName& ItemId : Chosen.AddItems)
		{
			bFullBag |= !GameInstance->AddInventoryItem(ItemId);
		}

		// A question with no id is a passing one; it is not something the
		// end-of-game summary should list.
		if (!ActiveDialogue.ChoiceId.IsNone())
		{
			FChoiceRecord Record;
			Record.ChoiceId = ActiveDialogue.ChoiceId;
			Record.Speaker = ActiveDialogue.Speaker;
			Record.Question = ActiveDialogue.Question;
			Record.ChosenLabel = Chosen.Label;
			Record.bChoseB = bChoiceB;
			GameInstance->RecordChoice(Record);
		}
	}

	bAwaitingChoice = false;
	// Clearing the question is what stops it being asked again after the result.
	ActiveDialogue.Question = FText::GetEmpty();
	// The written result says the thing was picked up, which would be a lie when
	// there was no room for it.
	CurrentLines = bFullBag ? TArray<FText>{InventoryFullLine} : Chosen.ResultLines;
	LineIndex = 0;

	if (CurrentLines.Num() == 0)
	{
		CloseDialogue();
	}
}

void ASpriteCharacter::CloseDialogue()
{
	// Taken first: what it runs may well start another dialogue.
	FSimpleDelegate Handler = DialogueClosedHandler;
	DialogueClosedHandler.Unbind();

	ActiveDialogue = FDialogueData();
	CurrentLines.Reset();
	LineIndex = 0;
	SelectedChoiceIndex = 0;
	bDialogueActive = false;
	bAwaitingChoice = false;

	Handler.ExecuteIfBound();
}

void ASpriteCharacter::ToggleMenu()
{
	// Mid-line the same key would fight the dialogue box for the screen.
	if (IsDialogueActive() || IsGameFinished())
	{
		return;
	}

	bMenuOpen = !bMenuOpen;
	SelectedItemIndex = 0;
	SelectedSaveIndex = 0;
	// Opens on the tab strip, which is where a tab gets chosen, and always on the
	// first tab rather than wherever it was left last time.
	bFocusOnTabs = true;
	ActiveTab = EDeceiveMenuTab::Info;

	if (bMenuOpen)
	{
		RefreshSaveSlots();
	}
}

void ASpriteCharacter::RefreshSaveSlots()
{
	if (UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>())
	{
		GameInstance->GetSaveSlots(SaveSlots);
	}
}

void ASpriteCharacter::MoveMenuCursor(int32 Delta)
{
	if (bFocusOnTabs)
	{
		const int32 Tab = FMath::Clamp(static_cast<int32>(ActiveTab) + Delta, 0, DeceiveMenuTabCount - 1);
		ActiveTab = static_cast<EDeceiveMenuTab>(Tab);
		SelectedItemIndex = 0;
		SelectedSaveIndex = 0;
		return;
	}

	switch (ActiveTab)
	{
	case EDeceiveMenuTab::Inventory:
		// Up and down step a whole row of the grid.
		MoveInventoryCursor(Delta * UDeceiveGameInstance::InventoryColumns);
		break;

	case EDeceiveMenuTab::Save:
		// Row 0 is "save now"; the saved runs follow it.
		SelectedSaveIndex = FMath::Clamp(SelectedSaveIndex + Delta, 0, SaveSlots.Num());
		break;

	default:
		break;
	}
}

void ASpriteCharacter::MoveMenuCursorSideways(int32 Delta)
{
	// The arrow keys stay inside whichever zone the cursor is in. Crossing
	// between the tabs and the tab's contents is the interact and cancel keys'
	// job, so a cursor never leaves a zone by accident.
	if (!bFocusOnTabs && ActiveTab == EDeceiveMenuTab::Inventory)
	{
		MoveInventoryCursor(Delta);
	}
}

void ASpriteCharacter::ActivateMenuRow()
{
	// On the tabs, the interact key is what steps into the tab's contents.
	if (bFocusOnTabs)
	{
		bFocusOnTabs = false;
		return;
	}

	if (ActiveTab == EDeceiveMenuTab::Inventory)
	{
		if (PlaceSelectedItem())
		{
			// The world has changed, so there is something to look at.
			bMenuOpen = false;
			ShowNotice(FText::FromString(TEXT("내려놓았습니다.")));
		}

		return;
	}

	if (ActiveTab != EDeceiveMenuTab::Save)
	{
		return;
	}

	UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	if (!GameInstance)
	{
		return;
	}

	if (SelectedSaveIndex == 0)
	{
		const bool bSaved = GameInstance->SaveToNewSlot();
		ShowNotice(FText::FromString(bSaved
			? TEXT("저장되었습니다.")
			: TEXT("빈 저장 칸이 없습니다.")));
		RefreshSaveSlots();
		return;
	}

	const int32 ListIndex = SelectedSaveIndex - 1;
	if (!SaveSlots.IsValidIndex(ListIndex))
	{
		return;
	}

	if (GameInstance->LoadFromSlot(SaveSlots[ListIndex].Slot))
	{
		// Nothing on screen belongs to the run that was just replaced.
		bMenuOpen = false;
		ShowNotice(FText::FromString(TEXT("불러왔습니다.")));
	}
	else
	{
		ShowNotice(FText::FromString(TEXT("불러오지 못했습니다.")));
	}
}

void ASpriteCharacter::ShowNotice(const FText& Text)
{
	NoticeText = Text;

	const UWorld* World = GetWorld();
	NoticeExpiry = (World ? World->GetTimeSeconds() : 0.f) + NoticeDuration;
}

FText ASpriteCharacter::GetNoticeText() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetTimeSeconds() > NoticeExpiry)
	{
		return FText::GetEmpty();
	}

	return NoticeText;
}

void ASpriteCharacter::MoveInventoryCursor(int32 Delta)
{
	// The cursor runs over every slot, empty ones included, so the player can see
	// how much room is left.
	SelectedItemIndex = FMath::Clamp(SelectedItemIndex + Delta, 0,
		UDeceiveGameInstance::InventoryCapacity - 1);
}

void ASpriteCharacter::MoveSelection(int32 Delta)
{
	if (!bAwaitingChoice)
	{
		return;
	}

	// Only ever two options, so the selection just clamps at both ends.
	SelectedChoiceIndex = FMath::Clamp(SelectedChoiceIndex + Delta, 0, 1);
}

void ASpriteCharacter::OnNavigateUp()
{
	if (bMenuOpen)
	{
		MoveMenuCursor(-1);
		return;
	}

	MoveSelection(-1);
}

void ASpriteCharacter::OnNavigateDown()
{
	if (bMenuOpen)
	{
		MoveMenuCursor(1);
		return;
	}

	MoveSelection(1);
}

void ASpriteCharacter::OnNavigateLeft()
{
	if (bMenuOpen)
	{
		MoveMenuCursorSideways(-1);
	}
}

void ASpriteCharacter::OnNavigateRight()
{
	if (bMenuOpen)
	{
		MoveMenuCursorSideways(1);
	}
}

void ASpriteCharacter::Cancel()
{
	// A question can be walked away from. Plain lines cannot: they are there to
	// be read, and the key that gets through them is the interact one.
	if (bAwaitingChoice)
	{
		CancelDialogue();
		return;
	}

	if (!bMenuOpen)
	{
		return;
	}

	// One zone at a time: out of the tab's contents first, then out of the menu.
	if (bFocusOnTabs)
	{
		ToggleMenu();
		return;
	}

	bFocusOnTabs = true;
}

void ASpriteCharacter::CancelDialogue()
{
	// Whatever was waiting on this dialogue was waiting on an answer, and there
	// is none, so it must not run.
	DialogueClosedHandler.Unbind();

	// The box stays open on one last line, so backing out reads as a decision
	// rather than as the screen simply going away.
	bAwaitingChoice = false;
	ActiveDialogue.Question = FText::GetEmpty();
	CurrentLines = TArray<FText>{CancelledLine};
	LineIndex = 0;
}

void ASpriteCharacter::StartRunning()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * RunSpeedMultiplier;
}

void ASpriteCharacter::StopRunning()
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ASpriteCharacter::Move(const FInputActionValue& Value)
{
	// Walking away mid-line would leave the box open with nobody to talk to.
	if (IsDialogueActive() || IsMenuOpen() || IsGameFinished())
	{
		return;
	}

	// The camera yaw is fixed at 0, so screen-up is world +X.
	const FVector2D Axis = Value.Get<FVector2D>();
	AddMovementInput(FVector::ForwardVector, Axis.Y);
	AddMovementInput(FVector::RightVector, Axis.X);
}

bool ASpriteCharacter::IsGameFinished() const
{
	const UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	return GameInstance && GameInstance->IsGameFinished();
}

bool ASpriteCharacter::PlaceSelectedItem()
{
	UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	if (!GameInstance)
	{
		return false;
	}

	const TArray<FName>& Carried = GameInstance->GetInventory();
	if (!Carried.IsValidIndex(SelectedItemIndex))
	{
		return false;
	}

	const FName ItemId = Carried[SelectedItemIndex];

	const UDeceiveDataSubsystem* Data = GameInstance->GetSubsystem<UDeceiveDataSubsystem>();
	FInventoryItemData Entry;
	FItemData Item;
	if (!Data || !Data->FindInventoryItem(ItemId, Entry) || !Entry.bPlaceable || !Data->FindItem(ItemId, Item))
	{
		return false;
	}

	const UWorld* World = GetWorld();
	ADeceiveGameMode* GameMode = World ? World->GetAuthGameMode<ADeceiveGameMode>() : nullptr;
	ADeceiveArea* Area = GameMode ? GameMode->GetCurrentArea() : nullptr;
	if (!Area)
	{
		return false;
	}

	// Right where the player stands. Items meant to be laid out are walk-through,
	// so standing on one is fine; its height comes from the table.
	const FVector PlayerLocation = GetActorLocation();
	const FVector DropLocation(PlayerLocation.X, PlayerLocation.Y, Item.Location.Z);

	const int32 PlacedId = GameInstance->PlaceItemInWorld(ItemId, Area->AreaId, DropLocation);
	if (!Area->SpawnItem(Item, DropLocation, PlacedId))
	{
		GameInstance->RemovePlacedItem(PlacedId);
		return false;
	}

	GameInstance->RemoveInventoryItem(ItemId);
	return true;
}

void ASpriteCharacter::TryInteract()
{
	if (IsGameFinished())
	{
		return;
	}

	// The menu has its own key; this one only works the save rows.
	if (bMenuOpen)
	{
		ActivateMenuRow();
		return;
	}

	// While the box is open the same key confirms a choice or steps to the next line.
	if (IsDialogueActive())
	{
		if (bAwaitingChoice)
		{
			SelectChoice(SelectedChoiceIndex == 1);
		}
		else
		{
			AdvanceDialogue();
		}
		return;
	}

	if (CurrentInteractable)
	{
		IInteractable::Execute_Interact(CurrentInteractable, this);
	}
}
