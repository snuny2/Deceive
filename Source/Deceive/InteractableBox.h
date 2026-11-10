#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveItemTypes.h"
#include "Interactable.h"
#include "InteractableBox.generated.h"

class ASpriteCharacter;
class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;

/** An object in the world that says something when interacted with. */
UCLASS()
class DECEIVE_API AInteractableBox : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableBox();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Interact_Implementation(AActor* InteractingActor) override;

	/** Fills this object in from the item table. Call before FinishSpawning. */
	void ApplyItemData(const FItemData& Item);
	virtual FText GetInteractPrompt_Implementation() const override { return ItemData.InteractPrompt; }

	UPROPERTY(EditAnywhere, Category = "Box", meta = (ClampMin = "10"))
	float BoxSize = 150.f;

	/** Swap this to retexture the box. */
	UPROPERTY(EditAnywhere, Category = "Box")
	TObjectPtr<UTexture2D> BoxTexture;

	/**
	 * Non-zero when this is a copy the player put down, which is what lets it be
	 * picked up again. Set by the area before FinishSpawning.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Item")
	int32 PlacedId = 0;

	/** Everything this object is, from Items.json. */
	UPROPERTY(VisibleAnywhere, Category = "Item")
	FItemData ItemData;

	/** Optional override; defaults to Paper2D's OpaqueUnlitSpriteMaterial. */
	UPROPERTY(EditAnywhere, Category = "Box")
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** Takes this copy back into the bag when the question was answered yes. */
	void TakeIfAccepted(ASpriteCharacter* PlayerCharacter, FName ChoiceId);

	UPROPERTY(VisibleAnywhere, Category = "Box")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
