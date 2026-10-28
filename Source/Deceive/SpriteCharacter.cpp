#include "SpriteCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DeceiveGameInstance.h"
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
	GetCharacterMovement()->MaxWalkSpeed = 350.f;

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
		if (!Candidate || !Candidate->Implements<UInteractable>())
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
	// Movement shares the arrow keys with the choice cursor. Without this it would
	// consume them first and the cursor would never move.
	MoveAction->bConsumeInput = false;

	InteractAction = NewObject<UInputAction>(this, TEXT("InteractAction"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	NavigateUpAction = NewObject<UInputAction>(this, TEXT("NavigateUpAction"));
	NavigateUpAction->ValueType = EInputActionValueType::Boolean;

	NavigateDownAction = NewObject<UInputAction>(this, TEXT("NavigateDownAction"));
	NavigateDownAction->ValueType = EInputActionValueType::Boolean;

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

	MappingContext->MapKey(InteractAction, EKeys::F);
	MappingContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Bottom);

	// The arrow keys also drive movement, but movement is blocked while a question
	// is up, so the two never act at the same time.
	MappingContext->MapKey(NavigateUpAction, EKeys::Up);
	MappingContext->MapKey(NavigateUpAction, EKeys::Gamepad_DPad_Up);
	MappingContext->MapKey(NavigateDownAction, EKeys::Down);
	MappingContext->MapKey(NavigateDownAction, EKeys::Gamepad_DPad_Down);
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
		EnhancedInput->BindAction(NavigateUpAction, ETriggerEvent::Started, this, &ASpriteCharacter::OnNavigateUp);
		EnhancedInput->BindAction(NavigateDownAction, ETriggerEvent::Started, this, &ASpriteCharacter::OnNavigateDown);
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

	// A question with no id is a passing one, like a portal prompt; it is not
	// something the end-of-game summary should list.
	if (UDeceiveGameInstance* GameInstance = !ActiveDialogue.ChoiceId.IsNone()
		? GetGameInstance<UDeceiveGameInstance>() : nullptr)
	{
		FChoiceRecord Record;
		Record.ChoiceId = ActiveDialogue.ChoiceId;
		Record.Speaker = ActiveDialogue.Speaker;
		Record.Question = ActiveDialogue.Question;
		Record.ChosenLabel = Chosen.Label;
		Record.bChoseB = bChoiceB;
		GameInstance->RecordChoice(Record);
	}

	bAwaitingChoice = false;
	// Clearing the question is what stops it being asked again after the result.
	ActiveDialogue.Question = FText::GetEmpty();
	CurrentLines = Chosen.ResultLines;
	LineIndex = 0;

	if (CurrentLines.Num() == 0)
	{
		CloseDialogue();
	}
}

void ASpriteCharacter::CloseDialogue()
{
	ActiveDialogue = FDialogueData();
	CurrentLines.Reset();
	LineIndex = 0;
	SelectedChoiceIndex = 0;
	bDialogueActive = false;
	bAwaitingChoice = false;
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
	MoveSelection(-1);
}

void ASpriteCharacter::OnNavigateDown()
{
	MoveSelection(1);
}

void ASpriteCharacter::Move(const FInputActionValue& Value)
{
	// Walking away mid-line would leave the box open with nobody to talk to.
	if (IsDialogueActive() || IsGameFinished())
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

void ASpriteCharacter::TryInteract()
{
	if (IsGameFinished())
	{
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
