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

	InteractPrompt = FText::FromString(TEXT("조사한다"));

	Dialogue.Speaker = FText::FromString(TEXT("나무 상자"));
	Dialogue.Lines.Add(FText::FromString(TEXT("낡은 나무 상자다. 못이 하나 빠져 있고, 뚜껑이 살짝 들려 있다.")));
	Dialogue.Lines.Add(FText::FromString(TEXT("안에서 희미하게 쇳소리가 난다.")));

	Dialogue.ChoiceId = TEXT("WoodenBox_Lid");
	Dialogue.Question = FText::FromString(TEXT("뚜껑을 열어볼까?"));

	Dialogue.ChoiceA.Label = FText::FromString(TEXT("열어본다"));
	Dialogue.ChoiceA.ResultLines.Add(FText::FromString(TEXT("뚜껑을 들어올리자 먼지가 쏟아진다.")));
	Dialogue.ChoiceA.ResultLines.Add(FText::FromString(TEXT("안에는 녹슨 열쇠가 하나 들어 있다. 주머니에 넣어 두었다.")));

	Dialogue.ChoiceB.Label = FText::FromString(TEXT("그냥 둔다"));
	Dialogue.ChoiceB.ResultLines.Add(FText::FromString(TEXT("괜히 건드릴 필요는 없을 것 같다.")));
	Dialogue.ChoiceB.ResultLines.Add(FText::FromString(TEXT("상자를 등지고 돌아섰다.")));
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
