#include "RoomArea.h"

namespace
{
	/** Floor, back wall, two front wall segments, two side walls, doorway blocker. */
	constexpr int32 NumRoomBoxes = 7;
}

ARoomArea::ARoomArea()
{
	CreateBoxes(NumRoomBoxes);
}

void ARoomArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float HalfDepth = RoomDepth * 0.5f;
	const float HalfWidth = RoomWidth * 0.5f;
	const float WallZ = WallHeight * 0.5f;
	// The side walls reach past the corners so the room closes up.
	const float OuterHalfWidth = HalfWidth + WallThickness;

	LayOutBox(0, FVector(0.f, 0.f, -FloorThickness * 0.5f),
		FVector(RoomDepth, RoomWidth, FloorThickness), FloorTexture);

	// Back wall, the one the camera looks at.
	LayOutBox(1, FVector(HalfDepth + WallThickness * 0.5f, 0.f, WallZ),
		FVector(WallThickness, OuterHalfWidth * 2.f, WallHeight), WallTexture);

	// Front wall, split around the doorway. It faces the camera, so it is hidden.
	const float FrontX = -HalfDepth - WallThickness * 0.5f;
	const float HalfDoor = DoorWidth * 0.5f;
	const float SegmentLength = FMath::Max(OuterHalfWidth - HalfDoor, 1.f);
	const float SegmentOffset = HalfDoor + SegmentLength * 0.5f;

	LayOutBox(2, FVector(FrontX, -SegmentOffset, WallZ),
		FVector(WallThickness, SegmentLength, WallHeight), WallTexture, true);
	LayOutBox(3, FVector(FrontX, SegmentOffset, WallZ),
		FVector(WallThickness, SegmentLength, WallHeight), WallTexture, true);

	LayOutBox(4, FVector(0.f, -HalfWidth - WallThickness * 0.5f, WallZ),
		FVector(RoomDepth, WallThickness, WallHeight), WallTexture);
	LayOutBox(5, FVector(0.f, HalfWidth + WallThickness * 0.5f, WallZ),
		FVector(RoomDepth, WallThickness, WallHeight), WallTexture);

	// Fills the doorway so the player cannot walk out of the room through it.
	LayOutBlocker(6, FVector(FrontX, 0.f, WallZ),
		FVector(WallThickness, DoorWidth, WallHeight));
}

FVector ARoomArea::GetEntryLocation(FName EntryName) const
{
	// Middle of the room, well clear of the doorway trigger.
	return FVector(0.f, 0.f, 120.f);
}

void ARoomArea::SpawnContents()
{
	Super::SpawnContents();

	SpawnTransition(
		FVector(-RoomDepth * 0.5f + 40.f, 0.f, 100.f),
		FVector(60.f, DoorWidth * 0.5f, 120.f),
		ReturnArea, ReturnEntry);
}
