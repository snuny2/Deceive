#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PushableBox.generated.h"

class UMaterialInterface;
class UStaticMeshComponent;
class UTexture2D;

/**
 * A crate the player shoves around by walking into it. It slides along one axis
 * at a time, so it lines up with a grid without any snapping while it moves,
 * and it stops dead against anything solid.
 */
UCLASS()
class DECEIVE_API APushableBox : public AActor
{
	GENERATED_BODY()

public:
	APushableBox();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Which slot this one belongs in. Only a slot of the same id accepts it. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FName ColourId;

	/** Drawn around the crate, since there are no textures to tell them apart yet. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	FColor Colour = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "20"))
	float BoxSize = 140.f;

	/** The gap still counted as touching, past the two half widths. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "0"))
	float PushReach = 20.f;

	/** The crate slides at this speed: the player's own walking speed. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "10"))
	float PushSpeed = 350.f;

	/**
	 * How much of the narrower of the two has to be overlapping across the push
	 * before the crate moves. A third of a shoulder against it does nothing.
	 */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinContactFraction = 2.f / 3.f;

	/** How many crates in a row can be shoved at once. Beyond this, none move. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "1"))
	int32 MaxPushedCrates = 2;

	/** How much slower a queue of crates is than a single one. */
	UPROPERTY(EditAnywhere, Category = "Puzzle", meta = (ClampMin = "1.0"))
	float ChainSlowdown = 3.f;

	/** Temporary: prints on screen why a crate is or is not moving. */
	UPROPERTY(EditAnywhere, Category = "Puzzle")
	bool bDebugPush = false;

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	TObjectPtr<UTexture2D> BoxTexture;

	UPROPERTY(EditAnywhere, Category = "Puzzle")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Settled into its slot: it stops moving and stops being pushable. */
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void Seat(const FVector& SlotLocation);

	UFUNCTION(BlueprintPure, Category = "Puzzle")
	bool IsSeated() const { return bSeated; }

private:
	/** Moves the crate the way the player is walking into it. */
	void FollowPlayer(float DeltaSeconds);

	/** Moves this one crate, never letting it end up inside another. */
	void Shove(const FVector& Offset);

	/** Writes where the crate stands down, so leaving the room does not undo it. */
	void Remember() const;

	/** The crate touching this one on the pushed side, if there is one. */
	APushableBox* FindCrateAhead(bool bAlongX, float Direction) const;

	/** True when the crate would be inside a desk, a wall or another crate there. */
	bool IsBlockedAt(const FVector& At) const;

	/** On-screen note of what the push did, while this is being worked out. */
	void Report(const FString& What) const;

	UPROPERTY(VisibleAnywhere, Category = "Puzzle")
	TObjectPtr<UStaticMeshComponent> Mesh;

	bool bSeated = false;
};
