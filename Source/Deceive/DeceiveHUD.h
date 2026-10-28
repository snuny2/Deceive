#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DeceiveHUD.generated.h"

class ASpriteCharacter;
class UFont;

/** One drawn line of a panel, already wrapped. */
struct FDialogueRow
{
	FString Text;
	FLinearColor Color = FLinearColor::White;
};

/**
 * Draws the interact prompt and the dialogue box straight onto the canvas. UMG
 * widgets would need content assets, which this project deliberately does without.
 */
UCLASS()
class DECEIVE_API ADeceiveHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	/** Used when the interactable in range does not name its own wording. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	FString InteractPrompt = TEXT("조사한다");

	/** Shown in the corner of the dialogue box to say which key continues. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FString DialogueHint = TEXT("F");

	/** Fraction of the screen width the dialogue box spans. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float DialogueWidthFraction = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (ClampMin = "0.1"))
	float DialogueTextScale = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FLinearColor DialogueBackgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.8f);

	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FLinearColor DialogueSpeakerColor = FLinearColor(1.f, 0.85f, 0.4f, 1.f);

	/** Shown in the corner while a question is waiting on an answer. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FString ChoiceHint = TEXT("↑↓ 선택    F 확정");

	/** Marks the highlighted option. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FString ChoiceCursor = TEXT("▶ ");

	/** The highlighted option. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FLinearColor ChoiceColor = FLinearColor(0.6f, 0.85f, 1.f, 1.f);

	/** The option that is not highlighted. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FLinearColor ChoiceDimColor = FLinearColor(0.55f, 0.55f, 0.55f, 1.f);

	/** Heading of the end-of-game summary. */
	UPROPERTY(EditDefaultsOnly, Category = "Summary")
	FString SummaryTitle = TEXT("선택 기록");

	/** Shown instead of the list when nothing was answered. */
	UPROPERTY(EditDefaultsOnly, Category = "Summary")
	FString SummaryEmptyText = TEXT("기록된 선택이 없습니다.");

	UPROPERTY(EditDefaultsOnly, Category = "Summary", meta = (ClampMin = "0.1"))
	float SummaryTextScale = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Summary")
	FLinearColor SummaryBackgroundColor = FLinearColor(0.02f, 0.02f, 0.04f, 0.94f);

private:
	void DrawInteractPrompt(const ASpriteCharacter& PlayerCharacter);
	void DrawDialogueBox(const ASpriteCharacter& PlayerCharacter);

	/** The end-of-game list. Nothing of it is shown while the game is running. */
	void DrawChoiceSummary();

	/** Options panel, sitting on top of the dialogue box and flush with its right edge. */
	void DrawChoicePanel(const ASpriteCharacter& PlayerCharacter, float BoxTop);

	/** Draws rows in a right-aligned panel whose bottom edge sits at PanelBottom. */
	void DrawSidePanel(const TArray<FDialogueRow>& Rows, float PanelBottom);

	/** Where a dialogue box's top edge would be, so the prompt lands in the same place. */
	float GetNominalDialogueTop();

	/** Breaks Text into lines that each fit within MaxWidth, preferring spaces. */
	void WrapText(const FString& Text, UFont* Font, float Scale, float MaxWidth, TArray<FString>& OutLines);
};
