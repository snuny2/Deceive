#include "InteractableBox.h"

#include "Components/StaticMeshComponent.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveGameInstance.h"
#include "DeceiveLog.h"
#include "DeceiveSpriteMaterial.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "SpriteCharacter.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeMeshSize = 100.f;
}

AInteractableBox::AInteractableBox()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh->SetStaticMesh(CubeMeshFinder.Object);
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Everything this object says comes from Content/Data; see ApplyItemData.
}

void AInteractableBox::ApplyItemData(const FItemData& Item)
{
	ItemData = Item;
	BoxSize = Item.Size;
}

void AInteractableBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Mesh->SetRelativeScale3D(FVector(BoxSize / CubeMeshSize));

	// Something small left on the floor should not become a wall. Overlap still
	// reaches the player's interaction sphere, so it can be talked to either way.
	Mesh->SetCollisionProfileName(ItemData.bWalkThrough
		? TEXT("OverlapAllDynamic")
		: TEXT("BlockAll"));

	if (UMaterialInstanceDynamic* Material =
		MakeTexturedMaterial(this, BaseMaterial, BoxTexture, DeceiveOpaqueSpriteMaterialPath))
	{
		Mesh->SetMaterial(0, Material);
	}
}

void AInteractableBox::Interact_Implementation(AActor* InteractingActor)
{
	ASpriteCharacter* PlayerCharacter = Cast<ASpriteCharacter>(InteractingActor);
	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;

	if (!PlayerCharacter || !Data)
	{
		return;
	}

	// Resolved on every interaction, so flags set last time can change what gets
	// said this time.
	FDialogueData Dialogue;
	FName DialogueId;
	if (!Data->ResolveDialogue(ItemData.DialogueIds, Dialogue, DialogueId))
	{
		UE_LOG(LogDeceive, Warning, TEXT("Interact: %s has nothing to say right now."), *GetName());
		return;
	}

	UE_LOG(LogDeceive, Log, TEXT("Interact: %s says '%s'"), *GetName(), *DialogueId.ToString());

	// Asking "pick it up again?" with nowhere to put it would only lead to a
	// result line that contradicts itself, so the box says so instead.
	const UDeceiveGameInstance* DeceiveGameInstance = Cast<UDeceiveGameInstance>(GameInstance);
	if (PlacedId != 0 && DeceiveGameInstance && DeceiveGameInstance->IsInventoryFull())
	{
		PlayerCharacter->SayInventoryFull();
		return;
	}

	// Only a copy the player put down can be taken back, and only when its
	// dialogue actually asks something.
	if (PlacedId == 0 || Dialogue.ChoiceId.IsNone())
	{
		PlayerCharacter->StartDialogue(Dialogue);
		return;
	}

	// The answer is read back off the record the choice writes, so the dialogue
	// file needs no new kind of instruction.
	const FName ChoiceId = Dialogue.ChoiceId;
	PlayerCharacter->StartDialogueThen(Dialogue,
		FSimpleDelegate::CreateWeakLambda(this, [this, PlayerCharacter, ChoiceId]()
		{
			TakeIfAccepted(PlayerCharacter, ChoiceId);
		}));
}

void AInteractableBox::TakeIfAccepted(ASpriteCharacter* PlayerCharacter, FName ChoiceId)
{
	UDeceiveGameInstance* GameInstance = Cast<UDeceiveGameInstance>(GetGameInstance());
	if (!IsValid(PlayerCharacter) || !GameInstance)
	{
		return;
	}

	bool bChoseB = false;
	if (!GameInstance->TryGetChoice(ChoiceId, bChoseB) || bChoseB)
	{
		return;
	}

	if (!GameInstance->AddInventoryItem(ItemData.ItemId))
	{
		PlayerCharacter->SayInventoryFull();
		return;
	}

	GameInstance->RemovePlacedItem(PlacedId);
	Destroy();
}
