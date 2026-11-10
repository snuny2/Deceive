#include "PressurePlate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DeceiveSpriteMaterial.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "InteractableBox.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeMeshSize = 100.f;
}

APressurePlate::APressurePlate()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Trigger->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Trigger);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh->SetStaticMesh(CubeMeshFinder.Object);
	// Standing on it must not be the same as walking into it.
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APressurePlate::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// The trigger reaches a little higher than the plate itself, so a capsule
	// standing on top of it still counts as overlapping.
	Trigger->SetBoxExtent(FVector(PlateSize * 0.5f, PlateSize * 0.5f, PlateThickness));
	Mesh->SetRelativeScale3D(FVector(PlateSize, PlateSize, PlateThickness) / CubeMeshSize);

	if (UMaterialInstanceDynamic* Material =
		MakeTexturedMaterial(this, BaseMaterial, PlateTexture, DeceiveOpaqueSpriteMaterialPath))
	{
		Mesh->SetMaterial(0, Material);
	}
}

bool APressurePlate::IsPressed() const
{
	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping);

	for (const AActor* Other : Overlapping)
	{
		// The player's own weight, or anything they have set down on it. Doorways
		// and other triggers overlap everything, so they are not enough.
		if (IsValid(Other) && (Other->IsA<APawn>() || Other->IsA<AInteractableBox>()))
		{
			return true;
		}
	}

	return false;
}
