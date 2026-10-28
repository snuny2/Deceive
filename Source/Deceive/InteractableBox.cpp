#include "InteractableBox.h"

#include "Components/StaticMeshComponent.h"
#include "DeceiveLog.h"
#include "DeceiveSpriteMaterial.h"
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

void AInteractableBox::ApplyItemData(const FItemData& Item, const FDialogueData& InDialogue)
{
	BoxSize = Item.Size;
	InteractPrompt = Item.InteractPrompt;
	Dialogue = InDialogue;
}

void AInteractableBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Mesh->SetRelativeScale3D(FVector(BoxSize / CubeMeshSize));

	if (UMaterialInstanceDynamic* Material =
		MakeTexturedMaterial(this, BaseMaterial, BoxTexture, DeceiveOpaqueSpriteMaterialPath))
	{
		Mesh->SetMaterial(0, Material);
	}
}

void AInteractableBox::Interact_Implementation(AActor* InteractingActor)
{
	UE_LOG(LogDeceive, Log, TEXT("Interact: %s interacted with %s"),
		*GetNameSafe(InteractingActor), *GetName());

	if (ASpriteCharacter* PlayerCharacter = Cast<ASpriteCharacter>(InteractingActor))
	{
		PlayerCharacter->StartDialogue(Dialogue);
	}
}
