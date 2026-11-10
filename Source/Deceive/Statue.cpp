#include "Statue.h"

#include "Components/StaticMeshComponent.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveGameInstance.h"
#include "DeceiveSpriteMaterial.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "SpriteCharacter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeMeshSize = 100.f;

	/** Neither answer, for a question that was closed without one. */
	constexpr int32 UnansweredTurn = -1;
}

AStatue::AStatue()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh->SetStaticMesh(CubeMeshFinder.Object);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	InteractPrompt = FText::FromString(TEXT("돌려본다"));
	TurnDialogueId = TEXT("Statue_Turn");
	FacingDialogueIds = {TEXT("Statue_Up"), TEXT("Statue_Right"), TEXT("Statue_Down"), TEXT("Statue_Left")};
}

void AStatue::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Mesh->SetRelativeScale3D(FVector(StatueSize / CubeMeshSize, StatueSize / CubeMeshSize,
		StatueSize * 1.6f / CubeMeshSize));

	if (UMaterialInstanceDynamic* Material =
		MakeTexturedMaterial(this, BaseMaterial, StatueTexture, DeceiveOpaqueSpriteMaterialPath))
	{
		Mesh->SetMaterial(0, Material);
	}
}

void AStatue::SetFacing(EStatueFacing NewFacing)
{
	Facing = NewFacing;

	if (UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>())
	{
		Game->RememberStatueFacing(StatueId, static_cast<int32>(Facing));
	}
}

FName AStatue::TurnChoiceId() const
{
	return FName(*FString::Printf(TEXT("StatueTurn_%s"), *StatueId.ToString()));
}

void AStatue::Interact_Implementation(AActor* InteractingActor)
{
	ASpriteCharacter* PlayerCharacter = GetPlayerCharacter();
	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;

	FDialogueData Question;
	if (!PlayerCharacter || !Data || !Data->FindDialogue(TurnDialogueId, Question)
		|| Question.Question.IsEmpty())
	{
		// No question to ask: a quarter turn clockwise, as it used to be.
		SetFacing(static_cast<EStatueFacing>((static_cast<int32>(Facing) + 1) % 4));
		SayFacing();
		return;
	}

	// The answer lands in a flag of this statue's own, which is how the handler
	// below knows which way the player chose. Cleared first, so a question that
	// is somehow closed unanswered turns nothing.
	const FName ChoiceId = TurnChoiceId();
	Question.ChoiceId = ChoiceId;
	if (UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>())
	{
		Game->SetFlag(ChoiceId, UnansweredTurn);
	}

	FSimpleDelegate OnClosed;
	OnClosed.BindUObject(this, &AStatue::TurnAsAnswered);
	PlayerCharacter->StartDialogueThen(Question, OnClosed);
}

void AStatue::TurnAsAnswered()
{
	UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>();
	if (!Game)
	{
		return;
	}

	const int32 Answer = Game->GetFlag(TurnChoiceId(), UnansweredTurn);
	if (Answer != 0 && Answer != 1)
	{
		return;
	}

	// Up/Right/Down/Left runs clockwise, so right is one on and left is one back.
	const int32 Step = (Answer == 0) ? 3 : 1;
	SetFacing(static_cast<EStatueFacing>((static_cast<int32>(Facing) + Step) % 4));
	SayFacing();
}

ASpriteCharacter* AStatue::GetPlayerCharacter() const
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	return PlayerController ? Cast<ASpriteCharacter>(PlayerController->GetPawn()) : nullptr;
}

FVector AStatue::FacingDirection() const
{
	// Up the screen is +X, and screen right is +Y.
	switch (Facing)
	{
	case EStatueFacing::Right:	return FVector(0.f, 1.f, 0.f);
	case EStatueFacing::Down:	return FVector(-1.f, 0.f, 0.f);
	case EStatueFacing::Left:	return FVector(0.f, -1.f, 0.f);
	default:					return FVector(1.f, 0.f, 0.f);
	}
}

void AStatue::SayFacing() const
{
	ASpriteCharacter* PlayerCharacter = GetPlayerCharacter();

	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;

	const int32 Index = static_cast<int32>(Facing);
	if (!PlayerCharacter || !Data || !FacingDialogueIds.IsValidIndex(Index))
	{
		return;
	}

	FDialogueData Dialogue;
	if (Data->FindDialogue(FacingDialogueIds[Index], Dialogue) && Dialogue.Lines.Num() > 0)
	{
		PlayerCharacter->ShowNotice(Dialogue.Lines[0]);
	}
}

void AStatue::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// No textures yet: the colour says which statue it is, and the line out of
	// its front says which way it is looking.
	const FVector Center = GetActorLocation();
	DrawDebugBox(GetWorld(), Center, FVector(StatueSize * 0.5f, StatueSize * 0.5f, StatueSize * 0.8f),
		Colour, false, -1.f, SDPG_Foreground, 6.f);

	const FVector Nose = Center + FacingDirection() * StatueSize * 0.9f;
	DrawDebugLine(GetWorld(), Center, Nose, Colour, false, -1.f, SDPG_Foreground, 8.f);
	DrawDebugBox(GetWorld(), Nose, FVector(StatueSize * 0.15f), Colour,
		false, -1.f, SDPG_Foreground, 6.f);
}
