#include "CrossAlley.h"

#include "AreaTransition.h"

namespace
{
	/**
	 * Two floor slabs, eight corridor walls (one of which is split in two by the
	 * building doorway), four arm end caps (the +Y and -X ones each split in two
	 * by a doorway) and the three doorways' invisible walls.
	 */
	constexpr int32 NumAlleyBoxes = 22;
}

ACrossAlley::ACrossAlley()
{
	CreateBoxes(NumAlleyBoxes);
	AreaId = EDeceiveArea::Alley;
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

			// The far side of the -Y arm carries the building doorway, so that one
			// run is laid out as two segments with a gap between them.
			if (SignAlong < 0 && SignSide > 0)
			{
				const float NearEnd = -Half;
				const float FarEnd = -ArmLength;
				const float DoorNear = DoorCenterY + DoorWidth * 0.5f;
				const float DoorFar = DoorCenterY - DoorWidth * 0.5f;

				const float NearLength = FMath::Max(NearEnd - DoorNear, 1.f);
				const float FarLength = FMath::Max(DoorFar - FarEnd, 1.f);

				LayOutBox(Index++,
					FVector(SignSide * WallOffset, (NearEnd + DoorNear) * 0.5f, WallZ),
					FVector(WallThickness, NearLength, WallHeight), WallTexture);

				LayOutBox(Index++,
					FVector(SignSide * WallOffset, (DoorFar + FarEnd) * 0.5f, WallZ),
					FVector(WallThickness, FarLength, WallHeight), WallTexture);

				continue;
			}

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
		if (Sign < 0)
		{
			// The -X cap carries the doorway down into the room below, so it is
			// laid out as two segments with a gap in the middle.
			const float SegmentLength = FMath::Max((CapSpan - DoorWidth) * 0.5f, 1.f);
			const float SegmentCenter = (DoorWidth + SegmentLength) * 0.5f;
			for (int32 SegmentSign = -1; SegmentSign <= 1; SegmentSign += 2)
			{
				LayOutBox(Index++,
					FVector(Sign * CapOffset, SegmentSign * SegmentCenter, WallZ),
					FVector(WallThickness, SegmentLength, WallHeight), WallTexture, true);
			}
		}
		else
		{
			// The +X cap carries the doorway up into the wide room, so it is laid
			// out as two segments with a gap in the middle as well.
			const float SegmentLength = FMath::Max((CapSpan - DoorWidth) * 0.5f, 1.f);
			const float SegmentCenter = (DoorWidth + SegmentLength) * 0.5f;
			for (int32 SegmentSign = -1; SegmentSign <= 1; SegmentSign += 2)
			{
				LayOutBox(Index++,
					FVector(Sign * CapOffset, SegmentSign * SegmentCenter, WallZ),
					FVector(WallThickness, SegmentLength, WallHeight), WallTexture, false);
			}
		}

		if (Sign > 0)
		{
			// The +Y cap carries the locked door, so it is laid out as two
			// segments with the doorway between them.
			const float SegmentLength = FMath::Max((CapSpan - DoorWidth) * 0.5f, 1.f);
			const float SegmentCenter = (DoorWidth + SegmentLength) * 0.5f;
			for (int32 SegmentSign = -1; SegmentSign <= 1; SegmentSign += 2)
			{
				LayOutBox(Index++,
					FVector(SegmentSign * SegmentCenter, Sign * CapOffset, WallZ),
					FVector(SegmentLength, WallThickness, WallHeight), WallTexture);
			}

			continue;
		}

		LayOutBox(Index++, FVector(0.f, Sign * CapOffset, WallZ),
			FVector(CapSpan, WallThickness, WallHeight), WallTexture);
	}

	// Fills the building doorway so the player cannot walk through the hole and
	// off the floor. The portal there is entered by interacting, not by walking.
	LayOutBlocker(Index++,
		FVector(WallOffset, DoorCenterY, WallZ),
		FVector(WallThickness, DoorWidth, WallHeight));

	// Same for the locked door at the end of the +Y arm.
	LayOutBlocker(Index++,
		FVector(0.f, CapOffset, WallZ),
		FVector(DoorWidth, WallThickness, WallHeight));

	// And for the doorway at the end of the -X arm.
	LayOutBlocker(Index++,
		FVector(-CapOffset, 0.f, WallZ),
		FVector(WallThickness, DoorWidth, WallHeight));

	// And the one at the top of the +X arm, up into the wide room.
	LayOutBlocker(Index++,
		FVector(CapOffset, 0.f, WallZ),
		FVector(WallThickness, DoorWidth, WallHeight));
}

FVector ACrossAlley::GetEntryLocation(FName EntryName) const
{
	if (EntryName == TEXT("FromBuilding"))
	{
		// In the corridor, opposite the doorway.
		return FVector(0.f, DoorCenterY, 120.f);
	}

	if (EntryName == TEXT("FromPuzzle"))
	{
		// Back up the -X arm, in front of the door down to the room below.
		return FVector(-(ArmLength - 300.f), 0.f, 120.f);
	}

	if (EntryName == TEXT("FromEast"))
	{
		// Just inside the +Y arm, in front of the locked door.
		return FVector(0.f, ArmLength - 300.f, 120.f);
	}

	if (EntryName == TEXT("FromHall"))
	{
		// Inside the +X arm, below the doorway up into the wide room.
		return FVector(ArmLength - 300.f, 0.f, 120.f);
	}

	if (EntryName == TEXT("FromWest"))
	{
		// Back inside the -Y arm, clear of the map edge.
		return FVector(0.f, -(ArmLength - 400.f), 120.f);
	}

	// Where a new game starts: down the near arm, out of interaction range.
	return FVector(-500.f, 0.f, 120.f);
}

void ACrossAlley::SpawnContents()
{
	Super::SpawnContents();

	// Doorway into the building, on the far side of the -Y arm.
	// Exactly the hole in the wall: the wall's thickness, the doorway's width and
	// the wall's full height.
	SpawnTransition(
		FVector(CorridorWidth * 0.5f + WallThickness * 0.5f, DoorCenterY, WallHeight * 0.5f),
		FVector(WallThickness * 0.5f, DoorWidth * 0.5f, WallHeight * 0.5f),
		EDeceiveArea::Building, TEXT("Door"));

	// The map edge, pressed up against the end cap so it is nowhere near the
	// doorway. The cap stays in place to keep the player on the floor.
	const float EdgeDepth = 50.f;
	SpawnTransition(
		FVector(0.f, -(ArmLength - EdgeDepth), WallHeight * 0.5f),
		FVector(CorridorWidth * 0.5f, EdgeDepth, WallHeight * 0.5f),
		EDeceiveArea::NextStreet, TEXT("Door"));

	// The door at the end of the -X arm, down into the room with the word in it.
	// It needs what the barred room gives up, so the rooms are done in order.
	if (AAreaTransition* DownDoor = SpawnTransition(
		FVector(-(ArmLength + WallThickness * 0.5f), 0.f, WallHeight * 0.5f),
		FVector(WallThickness * 0.5f, DoorWidth * 0.5f, WallHeight * 0.5f),
		EDeceiveArea::PuzzleRoom, TEXT("FromAlley")))
	{
		DownDoor->InteractPrompt = FText::FromString(TEXT("문을 연다"));
		DownDoor->RequiredItem = TEXT("Trinket");
		DownDoor->bConsumeRequiredItem = false;
		DownDoor->UnlockedFlag = TEXT("PuzzleRoomDoor_Unlocked");
		DownDoor->LockedDialogueIds.Add(TEXT("PuzzleDoor"));
		DownDoor->UnlockDialogueIds.Add(TEXT("PuzzleDoor_Unlock"));
	}

	// The doorway at the top of the +X arm, into the wide room beyond the statues.
	SpawnTransition(
		FVector(ArmLength + WallThickness * 0.5f, 0.f, WallHeight * 0.5f),
		FVector(WallThickness * 0.5f, DoorWidth * 0.5f, WallHeight * 0.5f),
		EDeceiveArea::StatueHall, TEXT("FromAlley"));

	// The locked door at the end of the +Y arm, filling the gap in that end cap.
	if (AAreaTransition* LockedDoor = SpawnTransition(
		FVector(0.f, ArmLength + WallThickness * 0.5f, WallHeight * 0.5f),
		FVector(DoorWidth * 0.5f, WallThickness * 0.5f, WallHeight * 0.5f),
		EDeceiveArea::LockedRoom, TEXT("Door")))
	{
		LockedDoor->InteractPrompt = FText::FromString(TEXT("문을 연다"));
		LockedDoor->RequiredItem = TEXT("RustyKey");
		LockedDoor->LockedDialogueIds.Add(TEXT("LockedDoor"));
		LockedDoor->UnlockDialogueIds.Add(TEXT("LockedDoor_Unlock"));
		LockedDoor->UnlockedFlag = TEXT("LockedDoor_Unlocked");
	}
}
