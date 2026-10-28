#pragma once

#include "CoreMinimal.h"
#include "DeceiveDialogueTypes.h"
#include "GameFramework/Character.h"
#include "SpriteCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USphereComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/** A character drawn as a camera-facing textured quad, HD-2D style. */
UCLASS()
class DECEIVE_API ASpriteCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASpriteCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PawnClientRestart() override;

	/** Swap this to change how the character looks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UTexture2D> SpriteTexture;

	/** Optional override; defaults to Paper2D's MaskedUnlitSpriteMaterial. */
	UPROPERTY(EditAnywhere, Category = "Sprite")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "Sprite", meta = (ClampMin = "1"))
	float SpriteWidth = 120.f;

	UPROPERTY(EditAnywhere, Category = "Sprite", meta = (ClampMin = "1"))
	float SpriteHeight = 180.f;

	/**
	 * Measured from the player's centre to the target's collision, so the capsule's
	 * own radius (34) is most of it: at 60 the prompt appears about 26 units before
	 * the player touches the target.
	 */
	UPROPERTY(EditAnywhere, Category = "Interaction", meta = (ClampMin = "10"))
	float InteractionRadius = 60.f;

	/** Applies SpriteTexture to the quad. Call again after changing it at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void RefreshSprite();

	/** The interactable currently in range, if any. The HUD reads this to place its prompt. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	/** Opens the dialogue box. Does nothing when there is nothing to say. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(const FDialogueData& InDialogue);

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsDialogueActive() const { return bDialogueActive; }

	/** True while the question is up and the player owes an answer. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsAwaitingChoice() const { return bAwaitingChoice; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetDialogueSpeaker() const { return ActiveDialogue.Speaker; }

	/** The question while one is pending, otherwise the current line. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetCurrentDialogueLine() const;

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetChoiceLabelA() const { return ActiveDialogue.ChoiceA.Label; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FText GetChoiceLabelB() const { return ActiveDialogue.ChoiceB.Label; }

	/** 0 for the first option, 1 for the second. The HUD marks this one. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	int32 GetSelectedChoiceIndex() const { return SelectedChoiceIndex; }

	/** True once the game instance says the run is over. */
	UFUNCTION(BlueprintPure, Category = "Flow")
	bool IsGameFinished() const;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Sprite")
	TObjectPtr<UStaticMeshComponent> SpriteQuad;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentInteractable;

	UPROPERTY(Transient)
	FDialogueData ActiveDialogue;

	/** The run being read out: the opening lines first, then a choice's result. */
	UPROPERTY(Transient)
	TArray<FText> CurrentLines;

	int32 LineIndex = 0;
	int32 SelectedChoiceIndex = 0;
	bool bDialogueActive = false;
	bool bAwaitingChoice = false;

	/**
	 * Enhanced Input normally reads these from .uasset files. This project builds
	 * them in C++ instead, so no content assets are needed.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MappingContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> NavigateUpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> NavigateDownAction;

	/** Builds the input objects once; safe to call repeatedly. */
	void EnsureInputObjects();

	void UpdateCurrentInteractable();

	/** Steps to the next line, raising the question or closing after the last one. */
	void AdvanceDialogue();

	/** Answers a pending question and plays that choice's result lines. */
	void SelectChoice(bool bChoiceB);

	void MoveSelection(int32 Delta);

	void CloseDialogue();

	void Move(const FInputActionValue& Value);
	void TryInteract();
	void OnNavigateUp();
	void OnNavigateDown();
};
