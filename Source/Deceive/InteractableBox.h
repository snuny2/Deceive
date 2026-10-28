#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DeceiveDialogueTypes.h"
#include "Interactable.h"
#include "InteractableBox.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;

/** A test target: walk up to it, press the interact key, watch the log. */
UCLASS()
class DECEIVE_API AInteractableBox : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableBox();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Interact_Implementation(AActor* InteractingActor) override;
	virtual FText GetInteractPrompt_Implementation() const override { return InteractPrompt; }

	/** Wording on the prompt while this is in range. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText InteractPrompt;

	UPROPERTY(EditAnywhere, Category = "Box", meta = (ClampMin = "10"))
	float BoxSize = 150.f;

	/** Swap this to retexture the box. */
	UPROPERTY(EditAnywhere, Category = "Box")
	TObjectPtr<UTexture2D> BoxTexture;

	/** What gets said, asked and answered when the player interacts. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FDialogueData Dialogue;

	/** Optional override; defaults to Paper2D's OpaqueUnlitSpriteMaterial. */
	UPROPERTY(EditAnywhere, Category = "Box")
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	UPROPERTY(VisibleAnywhere, Category = "Box")
	TObjectPtr<UStaticMeshComponent> Mesh;
};
