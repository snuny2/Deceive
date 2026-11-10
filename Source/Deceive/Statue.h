#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable.h"
#include "Statue.generated.h"

class ASpriteCharacter;
class UStaticMeshComponent;
class UTexture2D;
class UMaterialInterface;

/** Which way a statue is looking, in screen terms. */
UENUM(BlueprintType)
enum class EStatueFacing : uint8
{
	Up,
	Right,
	Down,
	Left
};

/**
 * A statue that turns a quarter of the way round each time it is interacted
 * with. What it is looking at is the whole of the puzzle; the room it stands in
 * watches the facings and decides when they are right.
 */
UCLASS()
class DECEIVE_API AStatue : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AStatue();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractPrompt_Implementation() const override { return InteractPrompt; }

	UFUNCTION(BlueprintPure, Category = "Statue")
	EStatueFacing GetFacing() const { return Facing; }

	/** Turns it without the player, for putting a remembered facing back. */
	UFUNCTION(BlueprintCallable, Category = "Statue")
	void SetFacing(EStatueFacing NewFacing);

	/** Tells one statue from another, and names it in what is remembered. */
	UPROPERTY(EditAnywhere, Category = "Statue")
	FName StatueId;

	UPROPERTY(EditAnywhere, Category = "Statue")
	FColor Colour = FColor::White;

	UPROPERTY(EditAnywhere, Category = "Statue", meta = (ClampMin = "20"))
	float StatueSize = 120.f;

	UPROPERTY(EditAnywhere, Category = "Statue")
	FText InteractPrompt;

	/** The question asked when the statue is touched: which way to turn it. */
	UPROPERTY(EditAnywhere, Category = "Statue")
	FName TurnDialogueId;

	/** One line per facing, in the Up/Right/Down/Left order of the enum. */
	UPROPERTY(EditAnywhere, Category = "Statue")
	TArray<FName> FacingDialogueIds;

	UPROPERTY(EditAnywhere, Category = "Statue")
	TObjectPtr<UTexture2D> StatueTexture;

	UPROPERTY(EditAnywhere, Category = "Statue")
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** Where the statue is looking, as a direction in the world. */
	FVector FacingDirection() const;

	void SayFacing() const;

	/** Turns it the way the answer that was just given asks for. */
	void TurnAsAnswered();

	ASpriteCharacter* GetPlayerCharacter() const;

	/** The flag the answer is written to, one per statue so two never clash. */
	FName TurnChoiceId() const;

	UPROPERTY(VisibleAnywhere, Category = "Statue")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Statue")
	EStatueFacing Facing = EStatueFacing::Up;
};
