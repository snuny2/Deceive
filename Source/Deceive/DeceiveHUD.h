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
	FString DialogueHint = TEXT("Z");

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
	FString ChoiceHint = TEXT("↑↓ 선택    Z 확정    X 취소");

	/** Marks the highlighted option. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FString ChoiceCursor = TEXT("▶ ");

	/** The highlighted option. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FLinearColor ChoiceColor = FLinearColor(0.6f, 0.85f, 1.f, 1.f);

	/** The option that is not highlighted. */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue")
	FLinearColor ChoiceDimColor = FLinearColor(0.55f, 0.55f, 0.55f, 1.f);

	/** Shown in the description box when the highlighted slot is empty. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FString InventoryNoSelectionText = TEXT("빈 칸입니다.");

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FString InventoryHint = TEXT("방향키 이동    Z 선택 / 내려놓기    X 뒤로    C 닫기");

	/** Row that writes a new save, drawn above the saved runs. */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FString SaveNowText = TEXT("현재 진행사항 저장");

	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FString SaveEmptyText = TEXT("저장한 기록이 없다.");

	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FString SaveHint = TEXT("방향키 이동    Z 선택 / 저장 / 불러오기    X 뒤로    C 닫기");

	/** Outline of the "save now" row. */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FLinearColor SaveNowColor = FLinearColor(0.95f, 0.85f, 0.25f, 1.f);

	/** Outline of a saved run. */
	UPROPERTY(EditDefaultsOnly, Category = "Save")
	FLinearColor SaveRecordColor = FLinearColor(0.95f, 0.55f, 0.2f, 1.f);

	/** Labels on the info tab. */
	UPROPERTY(EditDefaultsOnly, Category = "Info")
	FString HealthLabel = TEXT("체력");

	UPROPERTY(EditDefaultsOnly, Category = "Info")
	FString StatusLabel = TEXT("상태");

	/** The filled part of the health bar. */
	UPROPERTY(EditDefaultsOnly, Category = "Info")
	FLinearColor HealthColor = FLinearColor(0.85f, 0.25f, 0.25f, 1.f);

	/** Stands in for the tabs that have nothing in them yet. */
	UPROPERTY(EditDefaultsOnly, Category = "Menu")
	FString MenuPlaceholderText = TEXT("아직 준비되지 않았다.");

	/** Inside of a slot and of the two boxes on the right. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FLinearColor InventorySlotColor = FLinearColor(0.08f, 0.08f, 0.11f, 0.95f);

	/** Outline of a slot that is not highlighted. */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FLinearColor InventoryBorderColor = FLinearColor(0.45f, 0.45f, 0.5f, 1.f);

	/**
	 * Stands in for items that name no picture of their own, so the screen is
	 * readable before any art exists.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	FString InventoryPlaceholderTexturePath = TEXT("/Engine/EngineResources/DefaultTexture");

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
	/** The whole menu screen: the tab strip, then whichever tab is open. */
	void DrawMenu(const ASpriteCharacter& PlayerCharacter);

	/** Tabs down the left edge. Returns where the tab's own content starts. */
	float DrawMenuTabs(const ASpriteCharacter& PlayerCharacter, float PanelLeft, float PanelTop, float PanelWidth);

	/** Slot grid on the left, the highlighted item's picture and text on the right. */
	void DrawInventoryTab(const ASpriteCharacter& PlayerCharacter, float Left, float Top, float Width, float Height);

	/** Portrait, name, status and health bar. */
	void DrawInfoTab(float Left, float Top, float Width, float Height);

	/** The "save now" row, then one row per saved run. */
	void DrawSaveTab(const ASpriteCharacter& PlayerCharacter, float Left, float Top, float Width, float Height);

	void DrawPlaceholderTab(float Left, float Top, float Width, float Height);

	/** The short "저장되었습니다." box. Drawn over the menu and over the world. */
	void DrawNotice(const ASpriteCharacter& PlayerCharacter);

	/** Four thin rects making an outline; the canvas has no outline call. */
	void DrawBorder(float X, float Y, float Width, float Height, const FLinearColor& Color, float Thickness);

	/**
	 * The parts of a rect between ClipTop and ClipBottom. The canvas has no
	 * clipping of its own, so a list that scrolls has to cut its own rows.
	 */
	void DrawClippedRect(const FLinearColor& Color, float X, float Y, float Width, float Height, float ClipTop, float ClipBottom);

	/** DrawBorder, cut the same way. */
	void DrawClippedBorder(float X, float Y, float Width, float Height, const FLinearColor& Color, float Thickness, float ClipTop, float ClipBottom);

	/** The item's own picture, or the placeholder when it names none. */
	UTexture2D* ResolveItemTexture(const FString& TexturePath);

	/** Full-screen black covering the area swap. Drawn over everything else. */
	void DrawTravelFade();

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

	/** Keyed by asset path, so a picture is only looked up once. */
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> ItemTextureCache;

	/** Breaks Text into lines that each fit within MaxWidth, preferring spaces. */
	void WrapText(const FString& Text, UFont* Font, float Scale, float MaxWidth, TArray<FString>& OutLines);
};
