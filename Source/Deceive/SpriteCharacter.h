#pragma once

#include "CoreMinimal.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveMenuTypes.h"
#include "DeceiveSaveGame.h"
#include "GameFramework/Character.h"
#include "SpriteCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class USphereComponent;
class AInteractableBox;
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

	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "1"))
	float WalkSpeed = 350.f;

	/** Walking speed is multiplied by this while the run key is held. */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "1"))
	float RunSpeedMultiplier = 1.5f;

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

	/**
	 * The interactable currently in range, if any. Checked for validity because a
	 * portal destroys itself as it travels, leaving this pointing at a dead actor
	 * for the rest of that frame.
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetCurrentInteractable() const { return IsValid(CurrentInteractable) ? CurrentInteractable : nullptr; }

	/** Said in the dialogue box when something will not fit in the bag. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText InventoryFullLine = FText::FromString(TEXT("더는 들 수 없다. 소지품이 가득 찼다."));

	/** Opens the box on InventoryFullLine, with nothing to answer. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SayInventoryFull();

	/** Opens the dialogue box. Does nothing when there is nothing to say. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void StartDialogue(const FDialogueData& InDialogue);

	/**
	 * Same, but runs OnClosed once the box is done. Used by the locked door, which
	 * waits for its line to be read before travelling.
	 */
	void StartDialogueThen(const FDialogueData& InDialogue, FSimpleDelegate OnClosed);

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

	// --- Putting things down ------------------------------------------------

	/**
	 * Builds the highlighted inventory item in the world, where the player is
	 * standing, and takes that one copy out of the bag.
	 */
	UFUNCTION(BlueprintCallable, Category = "Items")
	bool PlaceSelectedItem();

	// --- Menu ---------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "Menu")
	bool IsMenuOpen() const { return bMenuOpen; }

	UFUNCTION(BlueprintPure, Category = "Menu")
	EDeceiveMenuTab GetActiveTab() const { return ActiveTab; }

	/** True while the cursor is on the tab strip rather than inside the tab. */
	UFUNCTION(BlueprintPure, Category = "Menu")
	bool IsFocusOnTabs() const { return bFocusOnTabs; }

	/** Which inventory slot the cursor sits on, 0-based. */
	UFUNCTION(BlueprintPure, Category = "Menu")
	int32 GetSelectedItemIndex() const { return SelectedItemIndex; }

	/** 0 is the "save now" row; 1 and up are the saved runs. */
	UFUNCTION(BlueprintPure, Category = "Menu")
	int32 GetSelectedSaveIndex() const { return SelectedSaveIndex; }

	/** The save list as it was when the menu opened or was last written to. */
	const TArray<FDeceiveSaveInfo>& GetSaveSlots() const { return SaveSlots; }

	/**
	 * A short notice such as "저장되었습니다.", shown in its own box for a few
	 * seconds. Empty once it has run its time.
	 */
	UFUNCTION(BlueprintPure, Category = "Menu")
	FText GetNoticeText() const;

	/** Puts a notice on screen for NoticeDuration seconds. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ShowNotice(const FText& Text);

	/** How long a notice stays up. */
	UPROPERTY(EditAnywhere, Category = "Menu", meta = (ClampMin = "0.1"))
	float NoticeDuration = 2.5f;

	/** Opens or closes the menu. Ignored while the dialogue box is up. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ToggleMenu();

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

	/** Run when the box closes, then cleared. */
	FSimpleDelegate DialogueClosedHandler;

	int32 LineIndex = 0;
	int32 SelectedChoiceIndex = 0;
	bool bDialogueActive = false;
	bool bAwaitingChoice = false;

	int32 SelectedItemIndex = 0;
	int32 SelectedSaveIndex = 0;
	bool bMenuOpen = false;
	bool bFocusOnTabs = false;
	EDeceiveMenuTab ActiveTab = EDeceiveMenuTab::Inventory;

	UPROPERTY(Transient)
	TArray<FDeceiveSaveInfo> SaveSlots;

	UPROPERTY(Transient)
	FText NoticeText;

	/** World time the notice stops being drawn. */
	float NoticeExpiry = 0.f;

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
	TObjectPtr<UInputAction> InventoryAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CancelAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> RunAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> NavigateUpAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> NavigateLeftAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> NavigateRightAction;

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

	/** Backs out: a pending question, then the menu, one zone at a time. */
	void Cancel();

	/** Drops a question without answering it. Nothing is recorded or picked up. */
	void CancelDialogue();

	/** Said in place of an answer when a question is backed out of. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FText CancelledLine = FText::FromString(TEXT("다시 생각해보자."));

	void StartRunning();
	void StopRunning();
	void OnNavigateUp();
	void OnNavigateDown();
	void OnNavigateLeft();
	void OnNavigateRight();

	/** Walks the inventory cursor over the grid, clamped at the edges. */
	void MoveInventoryCursor(int32 Delta);

	/** Up and down inside the menu: the tab strip, the grid or the save list. */
	void MoveMenuCursor(int32 Delta);

	/** Left and right inside the menu, which also steps in and out of the tabs. */
	void MoveMenuCursorSideways(int32 Delta);

	/** Saves or loads, depending on which row of the save tab is highlighted. */
	void ActivateMenuRow();

	void RefreshSaveSlots();
};
