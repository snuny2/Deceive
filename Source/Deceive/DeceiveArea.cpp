#include "DeceiveArea.h"

#include "AreaTransition.h"
#include "Components/StaticMeshComponent.h"
#include "DeceiveLog.h"
#include "DeceiveSpriteMaterial.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeMeshSize = 100.f;
}

ADeceiveArea::ADeceiveArea()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ADeceiveArea::CreateBoxes(int32 Count)
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));

	Boxes.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Box = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Box%d"), Index));
		Box->SetupAttachment(GetRootComponent());
		Box->SetStaticMesh(CubeMeshFinder.Object);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Boxes.Add(Box);
	}
}

void ADeceiveArea::LayOutBox(int32 Index, const FVector& Center, const FVector& Size, UTexture2D* Texture, bool bCameraSide)
{
	UStaticMeshComponent* Box = Boxes.IsValidIndex(Index) ? Boxes[Index].Get() : nullptr;
	if (!Box)
	{
		return;
	}

	Box->SetRelativeLocation(Center);
	Box->SetRelativeScale3D(Size / CubeMeshSize);
	Box->SetHiddenInGame(bCameraSide && bHideCameraSideWalls);

	// Walls and floors are solid, so they use the opaque sprite material.
	if (UMaterialInstanceDynamic* Material =
		MakeTexturedMaterial(this, BaseMaterial, Texture, DeceiveOpaqueSpriteMaterialPath))
	{
		Box->SetMaterial(0, Material);
	}
}

void ADeceiveArea::LayOutBlocker(int32 Index, const FVector& Center, const FVector& Size)
{
	UStaticMeshComponent* Box = Boxes.IsValidIndex(Index) ? Boxes[Index].Get() : nullptr;
	if (!Box)
	{
		return;
	}

	Box->SetRelativeLocation(Center);
	Box->SetRelativeScale3D(Size / CubeMeshSize);
	// Never drawn, but it keeps its collision.
	Box->SetHiddenInGame(true);
}

void ADeceiveArea::BeginPlay()
{
	Super::BeginPlay();
	SpawnContents();
}

void ADeceiveArea::Destroyed()
{
	for (AActor* Content : SpawnedContents)
	{
		if (IsValid(Content))
		{
			Content->Destroy();
		}
	}
	SpawnedContents.Reset();

	Super::Destroyed();
}

FVector ADeceiveArea::GetEntryLocation(FName EntryName) const
{
	return FVector(0.f, 0.f, 120.f);
}

AAreaTransition* ADeceiveArea::SpawnTransition(const FVector& Location, const FVector& Extent, EDeceiveArea TargetArea, FName TargetEntry)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(Location);
	AAreaTransition* Transition = World->SpawnActorDeferred<AAreaTransition>(
		AAreaTransition::StaticClass(), SpawnTransform);

	if (!Transition)
	{
		return nullptr;
	}

	Transition->TargetArea = TargetArea;
	Transition->TargetEntry = TargetEntry;
	Transition->Extent = Extent;
	Transition->FinishSpawning(SpawnTransform);

	SpawnedContents.Add(Transition);

	const FString Label = FString::Printf(TEXT("Portal_To%s"), DeceiveAreaName(TargetArea));
#if WITH_EDITOR
	Transition->SetActorLabel(Label);
#endif

	UE_LOG(LogDeceive, Log, TEXT("%s spawned at %s, half size %s, entry '%s'"),
		*Label, *Location.ToString(), *Extent.ToString(), *TargetEntry.ToString());

	return Transition;
}
