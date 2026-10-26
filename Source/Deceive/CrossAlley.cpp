#include "CrossAlley.h"

#include "Components/StaticMeshComponent.h"
#include "DeceiveSpriteMaterial.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 NumBoxes = 14;
	constexpr float CubeMeshSize = 100.f;
}

ACrossAlley::ACrossAlley()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Object;

	Boxes.Reserve(NumBoxes);
	for (int32 Index = 0; Index < NumBoxes; ++Index)
	{
		UStaticMeshComponent* Box = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Box%d"), Index));
		Box->SetupAttachment(Root);
		Box->SetStaticMesh(CubeMesh);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Boxes.Add(Box);
	}
}

void ACrossAlley::LayOutBox(int32 Index, const FVector& Center, const FVector& Size, UTexture2D* Texture, bool bCameraSide)
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
	if (UMaterialInstanceDynamic* Material = MakeTexturedMaterial(this, BaseMaterial, Texture, DeceiveOpaqueSpriteMaterialPath))
	{
		Box->SetMaterial(0, Material);
	}
}

void ACrossAlley::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float Half = CorridorWidth * 0.5f;
	// Length of a wall run: from the edge of the crossing out to the arm's end.
	const float RunLength = FMath::Max(ArmLength - Half, 1.f);
	const float RunCenter = Half + RunLength * 0.5f;
	const float WallOffset = Half + WallThickness * 0.5f;
	const float WallZ = WallHeight * 0.5f;

	// Floors: two overlapping slabs forming the cross.
	const FVector FloorCenter(0.f, 0.f, -FloorThickness * 0.5f);
	LayOutBox(0, FloorCenter, FVector(ArmLength * 2.f, CorridorWidth, FloorThickness), FloorTexture);
	LayOutBox(1, FloorCenter, FVector(CorridorWidth, ArmLength * 2.f, FloorThickness), FloorTexture);

	// Corridor walls, broken where the two corridors cross.
	int32 Index = 2;
	for (int32 SignAlong = -1; SignAlong <= 1; SignAlong += 2)
	{
		for (int32 SignSide = -1; SignSide <= 1; SignSide += 2)
		{
			// The camera sits out beyond the -X arm, so that arm's side walls would
			// frame the view like a tunnel.
			LayOutBox(Index++,
				FVector(SignAlong * RunCenter, SignSide * WallOffset, WallZ),
				FVector(RunLength, WallThickness, WallHeight), WallTexture, SignAlong < 0);

			// The -X side faces the camera, so those runs would stand in front of the player.
			LayOutBox(Index++,
				FVector(SignSide * WallOffset, SignAlong * RunCenter, WallZ),
				FVector(WallThickness, RunLength, WallHeight), WallTexture, SignSide < 0);
		}
	}

	// End caps so the player cannot walk off an arm.
	const float CapOffset = ArmLength + WallThickness * 0.5f;
	const float CapSpan = CorridorWidth + WallThickness * 2.f;
	for (int32 Sign = -1; Sign <= 1; Sign += 2)
	{
		LayOutBox(Index++, FVector(Sign * CapOffset, 0.f, WallZ),
			FVector(WallThickness, CapSpan, WallHeight), WallTexture, Sign < 0);

		LayOutBox(Index++, FVector(0.f, Sign * CapOffset, WallZ),
			FVector(CapSpan, WallThickness, WallHeight), WallTexture);
	}
}
