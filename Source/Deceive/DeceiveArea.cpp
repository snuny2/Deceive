#include "DeceiveArea.h"

#include "AreaTransition.h"
#include "Components/StaticMeshComponent.h"
#include "DeceiveLog.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveGameInstance.h"
#include "DeceiveSpriteMaterial.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "InteractableBox.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeMeshSize = 100.f;
}

ADeceiveArea::ADeceiveArea()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	ItemClass = AInteractableBox::StaticClass();
}

void ADeceiveArea::SpawnContents()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	const UDeceiveGameInstance* DeceiveGameInstance = Cast<UDeceiveGameInstance>(GameInstance);

	if (!Data)
	{
		return;
	}

	// What the table says stands here.
	for (const TPair<FName, FItemData>& Entry : Data->GetItems())
	{
		const FItemData& Item = Entry.Value;

		// Items that only ever appear where the player leaves them are built from
		// the placed list below instead.
		if (Item.bPlacedOnly || Item.Area != AreaId)
		{
			continue;
		}

		SpawnItem(Item, Item.Location);
	}

	// Then every copy the player has put down in this area.
	if (!DeceiveGameInstance)
	{
		return;
	}

	for (const FDeceivePlacedItem& Placed : DeceiveGameInstance->GetPlacedItems())
	{
		FItemData Item;
		if (Placed.Area == AreaId && Data->FindItem(Placed.ItemId, Item))
		{
			SpawnItem(Item, Placed.Location, Placed.PlacedId);
		}
	}
}

AInteractableBox* ADeceiveArea::SpawnItem(const FItemData& Item, const FVector& Location, int32 PlacedId)
{
	UWorld* World = GetWorld();
	if (!World || !ItemClass)
	{
		return nullptr;
	}

	const FTransform SpawnTransform(Location);
	AInteractableBox* Spawned = World->SpawnActorDeferred<AInteractableBox>(ItemClass, SpawnTransform);
	if (!Spawned)
	{
		return nullptr;
	}

	Spawned->ApplyItemData(Item);
	Spawned->PlacedId = PlacedId;
	Spawned->FinishSpawning(SpawnTransform);

#if WITH_EDITOR
	if (!Item.Label.IsEmpty())
	{
		Spawned->SetActorLabel(Item.Label);
	}
#endif

	SpawnedContents.Add(Spawned);
	UE_LOG(LogDeceive, Log, TEXT("Item spawned: %s at %s"), *Item.ItemId.ToString(), *Location.ToString());

	return Spawned;
}

void ADeceiveArea::CreateBoxes(int32 Count)
{
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));

	// Numbered on from whatever is already there, so a subclass can add its own
	// boxes in its constructor without clashing with the base class's names.
	const int32 FirstIndex = Boxes.Num();
	Boxes.Reserve(FirstIndex + Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UStaticMeshComponent* Box = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Box%d"), FirstIndex + Index));
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

void ADeceiveArea::DisableBox(int32 Index)
{
	if (UStaticMeshComponent* Box = Boxes.IsValidIndex(Index) ? Boxes[Index].Get() : nullptr)
	{
		Box->SetHiddenInGame(true);
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetRelativeScale3D(FVector(0.01f));
	}
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
