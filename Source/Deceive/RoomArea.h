#pragma once

#include "CoreMinimal.h"
#include "DeceiveArea.h"
#include "RoomArea.generated.h"

class AAreaTransition;
class UTexture2D;

/** Which wall the room's doorway goes through. */
UENUM(BlueprintType)
enum class ERoomDoorSide : uint8
{
	/** The camera-facing wall, at the bottom of the screen. */
	Front,
	/** The wall the camera looks at, at the top of the screen. */
	Back,
	/** Screen left. */
	Left,
	/** Screen right. */
	Right
};

/** One more doorway in a room's wall, beside the one it always has. */
USTRUCT(BlueprintType)
struct FRoomDoorway
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Door")
	ERoomDoorSide Side = ERoomDoorSide::Front;

	/**
	 * How far along that wall it sits, from the middle. Positive is screen right
	 * on the front and back walls, and up the screen on the side walls. Two doors
	 * may share a wall as long as they are not in the same place.
	 */
	UPROPERTY(EditAnywhere, Category = "Door")
	float Offset = 0.f;

	UPROPERTY(EditAnywhere, Category = "Door")
	EDeceiveArea Area = EDeceiveArea::Alley;

	/** The name the player arrives under at the other end. */
	UPROPERTY(EditAnywhere, Category = "Door")
	FName Entry;

	/** The name arrivals use to come in through this doorway. */
	UPROPERTY(EditAnywhere, Category = "Door")
	FName EntryName;

	/** Shut until this flag is set. Empty means it is always open. */
	UPROPERTY(EditAnywhere, Category = "Door")
	FName RequiredFlag;

	/** What it says while it is shut. */
	UPROPERTY(EditAnywhere, Category = "Door")
	FName LockedDialogueId;
};

/**
 * A plain rectangular room with one doorway, on whichever wall DoorSide names.
 * Used for the building interior, the long street to the west and the room
 * behind the locked door.
 */
UCLASS()
class DECEIVE_API ARoomArea : public ADeceiveArea
{
	GENERATED_BODY()

public:
	ARoomArea();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual FVector GetEntryLocation(FName EntryName) const override;

	/** Across the screen. */
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "200"))
	float RoomWidth = 900.f;

	/** Into the screen. */
	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "200"))
	float RoomDepth = 700.f;

	UPROPERTY(EditAnywhere, Category = "Room", meta = (ClampMin = "50"))
	float DoorWidth = 300.f;

	/**
	 * A side door lines up with the corridor the room sits beside. The front wall
	 * is the one that is hidden, so a side door leaves the room just as visible.
	 */
	UPROPERTY(EditAnywhere, Category = "Room")
	ERoomDoorSide DoorSide = ERoomDoorSide::Front;

	/** How far along its wall the main doorway sits, from the middle. */
	UPROPERTY(EditAnywhere, Category = "Room")
	float DoorOffset = 0.f;

	/**
	 * More doorways, each on a wall of its own. One that names the wall the main
	 * door is already on is ignored.
	 */
	UPROPERTY(EditAnywhere, Category = "Room|Doors")
	TArray<FRoomDoorway> ExtraDoors;

	/** Floor, the wall segments the doorways leave behind, and their blockers. */
	static constexpr int32 NumRoomBoxes = 20;

	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> FloorTexture;

	UPROPERTY(EditAnywhere, Category = "Room|Texture")
	TObjectPtr<UTexture2D> WallTexture;

	/** Where the doorway leads back to. */
	UPROPERTY(EditAnywhere, Category = "Room")
	EDeceiveArea ReturnArea = EDeceiveArea::Alley;

	UPROPERTY(EditAnywhere, Category = "Room")
	FName ReturnEntry;

private:
	/** The extra doorways that are really usable, in order. */
	TArray<FRoomDoorway> UsableExtraDoors() const;

	/** Middle of a doorway, in the room's own space. */
	FVector GetDoorwayCenter(ERoomDoorSide Side, float Offset) const;

	/** Full size of the thing that fills it: thin across the doorway, door-wide along it. */
	FVector GetDoorwaySize(ERoomDoorSide Side) const;

	/** Standing room just inside a doorway. */
	FVector GetDoorwayEntry(ERoomDoorSide Side, float Offset) const;

	/** Where that doorway's portal goes, and how big its trigger is. */
	AAreaTransition* SpawnDoorway(ERoomDoorSide Side, float Offset, EDeceiveArea Target, FName Entry);

protected:
	virtual void SpawnContents() override;
};
