#include "RoomArea.h"

#include "AreaTransition.h"

ARoomArea::ARoomArea()
{
	CreateBoxes(NumRoomBoxes);
}

TArray<FRoomDoorway> ARoomArea::UsableExtraDoors() const
{
	TArray<FRoomDoorway> Doors;
	for (const FRoomDoorway& Door : ExtraDoors)
	{
		// Two doors may share a wall; two doors in the same spot may not.
		auto SamePlace = [&Door](ERoomDoorSide Side, float Offset)
		{
			return Side == Door.Side && FMath::IsNearlyEqual(Offset, Door.Offset, 1.f);
		};

		const bool bTaken = SamePlace(DoorSide, DoorOffset)
			|| Doors.ContainsByPredicate([&SamePlace](const FRoomDoorway& Seen)
				{ return SamePlace(Seen.Side, Seen.Offset); });

		if (!bTaken)
		{
			Doors.Add(Door);
		}
	}

	return Doors;
}

void ARoomArea::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const float HalfDepth = RoomDepth * 0.5f;
	const float HalfWidth = RoomWidth * 0.5f;
	const float WallZ = WallHeight * 0.5f;
	// The front and back walls reach past the corners so the room closes up.
	const float OuterHalfWidth = HalfWidth + WallThickness;

	LayOutBox(0, FVector(0.f, 0.f, -FloorThickness * 0.5f),
		FVector(RoomDepth, RoomWidth, FloorThickness), FloorTexture);

	int32 Index = 1;

	const TArray<FRoomDoorway> Extras = UsableExtraDoors();

	// Every doorway in the room, so each wall knows the holes it has to leave.
	auto GapsIn = [this, &Extras](ERoomDoorSide Side)
	{
		TArray<float> Gaps;
		if (DoorSide == Side)
		{
			Gaps.Add(DoorOffset);
		}

		for (const FRoomDoorway& Door : Extras)
		{
			if (Door.Side == Side)
			{
				Gaps.Add(Door.Offset);
			}
		}

		Gaps.Sort();
		return Gaps;
	};

	// Lays a wall out as the runs left between its doorways, in order along it.
	auto LayOutWall = [&](ERoomDoorSide Side, const FVector& Center, const FVector& Size, bool bAlongY, bool bHideFromCamera)
	{
		const float RunLength = bAlongY ? Size.Y : Size.X;
		float Cursor = -RunLength * 0.5f;

		auto Segment = [&](float From, float To)
		{
			const float Length = To - From;
			if (Length <= 1.f)
			{
				// A doorway at the very end of a wall leaves nothing to build.
				return;
			}

			FVector SegmentCenter = Center;
			FVector SegmentSize = Size;
			if (bAlongY)
			{
				SegmentCenter.Y += (From + To) * 0.5f;
				SegmentSize.Y = Length;
			}
			else
			{
				SegmentCenter.X += (From + To) * 0.5f;
				SegmentSize.X = Length;
			}

			LayOutBox(Index++, SegmentCenter, SegmentSize, WallTexture, bHideFromCamera);
		};

		for (const float Gap : GapsIn(Side))
		{
			Segment(Cursor, Gap - DoorWidth * 0.5f);
			Cursor = Gap + DoorWidth * 0.5f;
		}

		Segment(Cursor, RunLength * 0.5f);
	};

	// Back wall, the one the camera looks at.
	LayOutWall(ERoomDoorSide::Back,
		FVector(HalfDepth + WallThickness * 0.5f, 0.f, WallZ),
		FVector(WallThickness, OuterHalfWidth * 2.f, WallHeight), true, false);

	// Front wall. It stands between the camera and the player, so it is hidden.
	LayOutWall(ERoomDoorSide::Front,
		FVector(-HalfDepth - WallThickness * 0.5f, 0.f, WallZ),
		FVector(WallThickness, OuterHalfWidth * 2.f, WallHeight), true, true);

	LayOutWall(ERoomDoorSide::Left,
		FVector(0.f, -HalfWidth - WallThickness * 0.5f, WallZ),
		FVector(RoomDepth, WallThickness, WallHeight), false, false);

	LayOutWall(ERoomDoorSide::Right,
		FVector(0.f, HalfWidth + WallThickness * 0.5f, WallZ),
		FVector(RoomDepth, WallThickness, WallHeight), false, false);

	// Doorways are real holes, so each one needs filling in to keep the player
	// from walking out through it.
	LayOutBlocker(Index++, GetDoorwayCenter(DoorSide, DoorOffset), GetDoorwaySize(DoorSide));

	for (const FRoomDoorway& Door : Extras)
	{
		LayOutBlocker(Index++, GetDoorwayCenter(Door.Side, Door.Offset), GetDoorwaySize(Door.Side));
	}

	// A room with fewer doors leaves the boxes it did not need unused.
	for (; Index < NumRoomBoxes; ++Index)
	{
		DisableBox(Index);
	}
}

FVector ARoomArea::GetDoorwayCenter(ERoomDoorSide Side, float Offset) const
{
	const float WallZ = WallHeight * 0.5f;

	switch (Side)
	{
	case ERoomDoorSide::Left:
		return FVector(Offset, -RoomWidth * 0.5f - WallThickness * 0.5f, WallZ);

	case ERoomDoorSide::Right:
		return FVector(Offset, RoomWidth * 0.5f + WallThickness * 0.5f, WallZ);

	case ERoomDoorSide::Back:
		return FVector(RoomDepth * 0.5f + WallThickness * 0.5f, Offset, WallZ);

	default:
		return FVector(-RoomDepth * 0.5f - WallThickness * 0.5f, Offset, WallZ);
	}
}

FVector ARoomArea::GetDoorwaySize(ERoomDoorSide Side) const
{
	const bool bSideDoor = (Side == ERoomDoorSide::Left || Side == ERoomDoorSide::Right);
	return bSideDoor
		? FVector(DoorWidth, WallThickness, WallHeight)
		: FVector(WallThickness, DoorWidth, WallHeight);
}

FVector ARoomArea::GetDoorwayEntry(ERoomDoorSide Side, float Offset) const
{
	// Just inside the doorway, far enough in not to be standing on its trigger.
	// A long room would otherwise drop the player somewhere off in the distance.
	const FVector Doorway = GetDoorwayCenter(Side, Offset);

	switch (Side)
	{
	case ERoomDoorSide::Left:
		return FVector(Doorway.X, Doorway.Y + FMath::Min(250.f, RoomWidth * 0.4f), 120.f);

	case ERoomDoorSide::Right:
		return FVector(Doorway.X, Doorway.Y - FMath::Min(250.f, RoomWidth * 0.4f), 120.f);

	case ERoomDoorSide::Back:
		return FVector(Doorway.X - FMath::Min(250.f, RoomDepth * 0.4f), Doorway.Y, 120.f);

	default:
		return FVector(Doorway.X + FMath::Min(250.f, RoomDepth * 0.4f), Doorway.Y, 120.f);
	}
}

FVector ARoomArea::GetEntryLocation(FName EntryName) const
{
	for (const FRoomDoorway& Door : UsableExtraDoors())
	{
		if (!Door.EntryName.IsNone() && EntryName == Door.EntryName)
		{
			return GetDoorwayEntry(Door.Side, Door.Offset);
		}
	}

	return GetDoorwayEntry(DoorSide, DoorOffset);
}

AAreaTransition* ARoomArea::SpawnDoorway(ERoomDoorSide Side, float Offset, EDeceiveArea Target, FName Entry)
{
	// Sits in the doorway itself, pulled a little into the room.
	const bool bSideDoor = (Side == ERoomDoorSide::Left || Side == ERoomDoorSide::Right);
	const FVector Doorway = GetDoorwayCenter(Side, Offset);
	const float Inset = WallThickness * 0.5f + 40.f;

	FVector Location(Doorway.X, Doorway.Y, 100.f);
	FVector Extent(60.f, DoorWidth * 0.5f, 120.f);

	if (bSideDoor)
	{
		Location.Y += (Side == ERoomDoorSide::Left) ? Inset : -Inset;
		Extent = FVector(DoorWidth * 0.5f, 60.f, 120.f);
	}
	else
	{
		Location.X += (Side == ERoomDoorSide::Back) ? -Inset : Inset;
	}

	return SpawnTransition(Location, Extent, Target, Entry);
}

void ARoomArea::SpawnContents()
{
	Super::SpawnContents();

	SpawnDoorway(DoorSide, DoorOffset, ReturnArea, ReturnEntry);

	for (const FRoomDoorway& Door : UsableExtraDoors())
	{
		AAreaTransition* Portal = SpawnDoorway(Door.Side, Door.Offset, Door.Area, Door.Entry);
		if (!Portal)
		{
			continue;
		}

		Portal->RequiredFlag = Door.RequiredFlag;
		if (!Door.LockedDialogueId.IsNone())
		{
			Portal->LockedDialogueIds.Add(Door.LockedDialogueId);
		}
	}
}
