#include "DeceiveHUD.h"

#include "DeceiveGameInstance.h"
#include "DeceiveDataSubsystem.h"
#include "DeceiveGameMode.h"
#include "DeceiveLog.h"
#include "DeceiveMenuTypes.h"
#include "Engine/Texture2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Interactable.h"
#include "SpriteCharacter.h"

namespace
{
	constexpr float BoxPadding = 24.f;
	constexpr float BoxBottomMargin = 48.f;
	constexpr float SpeakerGap = 8.f;
	constexpr float ChoicePanelGap = 6.f;
}

void ADeceiveHUD::DrawHUD()
{
	Super::DrawHUD();

	const APlayerController* PlayerController = GetOwningPlayerController();
	const ASpriteCharacter* PlayerCharacter = PlayerController
		? Cast<ASpriteCharacter>(PlayerController->GetPawn())
		: nullptr;

	if (!PlayerCharacter || !GEngine || !Canvas)
	{
		return;
	}

	const UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	if (GameInstance && GameInstance->IsGameFinished())
	{
		DrawChoiceSummary();
		return;
	}

	if (PlayerCharacter->IsMenuOpen())
	{
		DrawMenu(*PlayerCharacter);
	}
	else if (PlayerCharacter->IsDialogueActive())
	{
		DrawDialogueBox(*PlayerCharacter);
	}
	else
	{
		DrawInteractPrompt(*PlayerCharacter);
	}

	DrawNotice(*PlayerCharacter);
	DrawTravelFade();
}

UTexture2D* ADeceiveHUD::ResolveItemTexture(const FString& TexturePath)
{
	const FString Path = TexturePath.IsEmpty() ? InventoryPlaceholderTexturePath : TexturePath;

	if (const TObjectPtr<UTexture2D>* Cached = ItemTextureCache.Find(Path))
	{
		return *Cached;
	}

	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
	if (!Texture && Path != InventoryPlaceholderTexturePath)
	{
		UE_LOG(LogDeceive, Warning, TEXT("Item picture '%s' did not load; using the placeholder."), *Path);
		Texture = LoadObject<UTexture2D>(nullptr, *InventoryPlaceholderTexturePath);
	}

	ItemTextureCache.Add(Path, Texture);
	return Texture;
}

void ADeceiveHUD::DrawBorder(float X, float Y, float Width, float Height, const FLinearColor& Color, float Thickness)
{
	DrawRect(Color, X, Y, Width, Thickness);
	DrawRect(Color, X, Y + Height - Thickness, Width, Thickness);
	DrawRect(Color, X, Y, Thickness, Height);
	DrawRect(Color, X + Width - Thickness, Y, Thickness, Height);
}

void ADeceiveHUD::DrawClippedRect(const FLinearColor& Color, float X, float Y, float Width, float Height, float ClipTop, float ClipBottom)
{
	const float Top = FMath::Max(Y, ClipTop);
	const float Bottom = FMath::Min(Y + Height, ClipBottom);
	if (Bottom <= Top)
	{
		return;
	}

	DrawRect(Color, X, Top, Width, Bottom - Top);
}

void ADeceiveHUD::DrawClippedBorder(float X, float Y, float Width, float Height, const FLinearColor& Color, float Thickness, float ClipTop, float ClipBottom)
{
	DrawClippedRect(Color, X, Y, Width, Thickness, ClipTop, ClipBottom);
	DrawClippedRect(Color, X, Y + Height - Thickness, Width, Thickness, ClipTop, ClipBottom);
	DrawClippedRect(Color, X, Y, Thickness, Height, ClipTop, ClipBottom);
	DrawClippedRect(Color, X + Width - Thickness, Y, Thickness, Height, ClipTop, ClipBottom);
}

void ADeceiveHUD::DrawMenu(const ASpriteCharacter& PlayerCharacter)
{
	const float ScreenWidth = Canvas->SizeX;
	const float ScreenHeight = Canvas->SizeY;

	DrawRect(SummaryBackgroundColor, 0.f, 0.f, ScreenWidth, ScreenHeight);

	const float PanelLeft = ScreenWidth * 0.06f;
	const float PanelTop = ScreenHeight * 0.14f;
	const float PanelWidth = ScreenWidth * 0.88f;
	const float PanelHeight = ScreenHeight * 0.72f;

	const float ContentLeft = DrawMenuTabs(PlayerCharacter, PanelLeft, PanelTop, PanelWidth);
	const float ContentWidth = PanelLeft + PanelWidth - ContentLeft;

	switch (PlayerCharacter.GetActiveTab())
	{
	case EDeceiveMenuTab::Info:
		DrawInfoTab(ContentLeft, PanelTop, ContentWidth, PanelHeight);
		break;

	case EDeceiveMenuTab::Inventory:
		DrawInventoryTab(PlayerCharacter, ContentLeft, PanelTop, ContentWidth, PanelHeight);
		break;

	case EDeceiveMenuTab::Save:
		DrawSaveTab(PlayerCharacter, ContentLeft, PanelTop, ContentWidth, PanelHeight);
		break;

	default:
		DrawPlaceholderTab(ContentLeft, PanelTop, ContentWidth, PanelHeight);
		break;
	}

	const FString& Hint = (PlayerCharacter.GetActiveTab() == EDeceiveMenuTab::Save) ? SaveHint : InventoryHint;
	if (!Hint.IsEmpty())
	{
		UFont* Font = GEngine->GetLargeFont();
		float HintWidth = 0.f;
		float HintHeight = 0.f;
		GetTextSize(Hint, HintWidth, HintHeight, Font, 1.f);
		DrawText(Hint, FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
			PanelLeft + PanelWidth - HintWidth, PanelTop + PanelHeight - HintHeight, Font, 1.f);
	}
}

float ADeceiveHUD::DrawMenuTabs(const ASpriteCharacter& PlayerCharacter, float PanelLeft, float PanelTop, float PanelWidth)
{
	UFont* Font = GEngine->GetLargeFont();

	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, Font, DialogueTextScale);

	const float TabWidth = PanelWidth * 0.14f;
	const float TabHeight = RowHeight + BoxPadding * 0.6f;
	const float TabGap = TabHeight * 0.45f;

	const int32 ActiveTab = static_cast<int32>(PlayerCharacter.GetActiveTab());
	const bool bFocused = PlayerCharacter.IsFocusOnTabs();

	for (int32 Index = 0; Index < DeceiveMenuTabCount; ++Index)
	{
		const float TabTop = PanelTop + Index * (TabHeight + TabGap);
		const bool bActive = (Index == ActiveTab);

		DrawRect(InventorySlotColor, PanelLeft, TabTop, TabWidth, TabHeight);
		// The outline only brightens while the cursor is actually on the strip, so
		// it is clear which side of the screen the arrow keys are driving.
		DrawBorder(PanelLeft, TabTop, TabWidth, TabHeight,
			(bActive && bFocused) ? ChoiceColor : InventoryBorderColor,
			bActive ? 4.f : 2.f);

		DrawText(DeceiveMenuTabName(static_cast<EDeceiveMenuTab>(Index)).ToString(),
			bActive ? ChoiceColor : ChoiceDimColor,
			PanelLeft + BoxPadding * 0.5f, TabTop + BoxPadding * 0.3f, Font, DialogueTextScale);
	}

	return PanelLeft + TabWidth + PanelWidth * 0.03f;
}

void ADeceiveHUD::DrawInfoTab(float Left, float Top, float Width, float Height)
{
	const UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	const UDeceiveDataSubsystem* Data = GameInstance ? GameInstance->GetSubsystem<UDeceiveDataSubsystem>() : nullptr;
	if (!GameInstance || !Data)
	{
		return;
	}

	const FDeceivePlayerData& Player = Data->GetPlayerData();
	UFont* Font = GEngine->GetLargeFont();

	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, Font, DialogueTextScale);

	const float Gap = BoxPadding * 0.6f;
	const float BarHeight = RowHeight + BoxPadding * 0.6f;
	const float HealthHeight = BarHeight * 1.4f;

	// Everything about the player sits inside one box, rather than floating on
	// the tab's background.
	const float Inset = BoxPadding * 0.5f;
	const float PortraitSize = FMath::Min(Width * 0.22f, Height * 0.42f);
	const float BlockHeight = FMath::Max(PortraitSize, BarHeight + Gap + HealthHeight) + Inset * 2.f;

	DrawRect(InventorySlotColor, Left, Top, Width, BlockHeight);
	DrawBorder(Left, Top, Width, BlockHeight, InventoryBorderColor, 2.f);

	Left += Inset;
	Top += Inset;
	Width -= Inset * 2.f;

	// Portrait on the left, the three readings beside it.
	DrawRect(DialogueBackgroundColor, Left, Top, PortraitSize, PortraitSize);
	if (UTexture2D* Portrait = ResolveItemTexture(Player.PortraitPath))
	{
		const float PortraitInset = PortraitSize * 0.06f;
		DrawTexture(Portrait, Left + PortraitInset, Top + PortraitInset,
			PortraitSize - PortraitInset * 2.f, PortraitSize - PortraitInset * 2.f, 0.f, 0.f, 1.f, 1.f);
	}
	DrawBorder(Left, Top, PortraitSize, PortraitSize, InventoryBorderColor, 2.f);

	const float DetailLeft = Left + PortraitSize + Gap;
	const float DetailWidth = Left + Width - DetailLeft;

	// The two rows are shorter than the portrait, so they are centred against it
	// rather than left sitting at the top with a gap underneath.
	const float StackHeight = BarHeight + Gap + HealthHeight;
	const float StackTop = Top + FMath::Max((PortraitSize - StackHeight) * 0.5f, 0.f);

	// Name, then status beside it on the same line.
	const float NameWidth = DetailWidth * 0.45f;
	DrawRect(DialogueBackgroundColor, DetailLeft, StackTop, NameWidth, BarHeight);
	DrawBorder(DetailLeft, StackTop, NameWidth, BarHeight, InventoryBorderColor, 2.f);
	DrawText(Player.Name.ToString(), DialogueSpeakerColor,
		DetailLeft + BoxPadding * 0.5f, StackTop + BoxPadding * 0.3f, Font, DialogueTextScale);

	const float StatusLeft = DetailLeft + NameWidth + Gap;
	const float StatusWidth = DetailLeft + DetailWidth - StatusLeft;
	DrawRect(DialogueBackgroundColor, StatusLeft, StackTop, StatusWidth, BarHeight);
	DrawBorder(StatusLeft, StackTop, StatusWidth, BarHeight, InventoryBorderColor, 2.f);

	const FString Status = GameInstance->GetStatus().IsEmpty()
		? Player.Status.ToString()
		: GameInstance->GetStatus().ToString();
	DrawText(FString::Printf(TEXT("%s: %s"), *StatusLabel, *Status), FLinearColor::White,
		StatusLeft + BoxPadding * 0.5f, StackTop + BoxPadding * 0.3f, Font, DialogueTextScale);

	// Health, as a bar that fills from the left plus the numbers themselves.
	const float HealthTop = StackTop + BarHeight + Gap;
	DrawRect(DialogueBackgroundColor, DetailLeft, HealthTop, DetailWidth, HealthHeight);

	const int32 MaxHealth = FMath::Max(Player.MaxHealth, 1);
	const float Fraction = FMath::Clamp(static_cast<float>(GameInstance->GetHealth()) / MaxHealth, 0.f, 1.f);
	const float BarInset = 3.f;
	DrawRect(HealthColor, DetailLeft + BarInset, HealthTop + BarInset,
		(DetailWidth - BarInset * 2.f) * Fraction, HealthHeight - BarInset * 2.f);

	DrawBorder(DetailLeft, HealthTop, DetailWidth, HealthHeight, InventoryBorderColor, 2.f);

	DrawText(FString::Printf(TEXT("%s  %d / %d"), *HealthLabel, GameInstance->GetHealth(), MaxHealth),
		FLinearColor::White, DetailLeft + BoxPadding * 0.5f,
		HealthTop + (HealthHeight - RowHeight) * 0.5f, Font, DialogueTextScale);
}

void ADeceiveHUD::DrawPlaceholderTab(float Left, float Top, float Width, float Height)
{
	DrawRect(InventorySlotColor, Left, Top, Width, Height);
	DrawBorder(Left, Top, Width, Height, InventoryBorderColor, 2.f);
	DrawText(MenuPlaceholderText, ChoiceDimColor, Left + BoxPadding, Top + BoxPadding,
		GEngine->GetLargeFont(), DialogueTextScale);
}

void ADeceiveHUD::DrawSaveTab(const ASpriteCharacter& PlayerCharacter, float Left, float Top, float Width, float Height)
{
	UFont* Font = GEngine->GetLargeFont();

	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, Font, DialogueTextScale);

	const float EntryHeight = RowHeight * 2.f + BoxPadding;
	const float EntryGap = BoxPadding * 0.5f;
	const float EntryStride = EntryHeight + EntryGap;

	const TArray<FDeceiveSaveInfo>& Slots = PlayerCharacter.GetSaveSlots();
	const int32 Selected = PlayerCharacter.GetSelectedSaveIndex();
	// Nothing in here is picked out while the cursor is still on the tab strip:
	// the arrow keys are driving the tabs, not these rows.
	const bool bFocused = !PlayerCharacter.IsFocusOnTabs();

	// The row that writes a new save, on its own above the list.
	const bool bSaveNowSelected = (Selected == 0) && bFocused;
	DrawRect(InventorySlotColor, Left, Top, Width, EntryHeight);
	DrawBorder(Left, Top, Width, EntryHeight,
		bSaveNowSelected ? ChoiceColor : SaveNowColor, bSaveNowSelected ? 4.f : 2.f);
	DrawText(SaveNowText, bSaveNowSelected ? ChoiceColor : FLinearColor::White,
		Left + BoxPadding, Top + BoxPadding * 0.5f, Font, DialogueTextScale);

	// --- the records, inside a box of their own -----------------------------
	const float ListTop = Top + EntryStride;
	const float ListHeight = FMath::Max(Height - EntryStride - RowHeight * 1.5f, EntryStride);

	DrawRect(InventorySlotColor, Left, ListTop, Width, ListHeight);
	DrawBorder(Left, ListTop, Width, ListHeight, InventoryBorderColor, 2.f);

	if (Slots.Num() == 0)
	{
		DrawText(SaveEmptyText, ChoiceDimColor, Left + BoxPadding, ListTop + BoxPadding,
			Font, DialogueTextScale);
		return;
	}

	const float Inset = BoxPadding * 0.5f;
	const float InnerTop = ListTop + Inset;
	const float InnerHeight = ListHeight - Inset * 2.f;
	const float InnerBottom = InnerTop + InnerHeight;

	const float ContentHeight = Slots.Num() * EntryStride - EntryGap;
	const float MaxScroll = FMath::Max(ContentHeight - InnerHeight, 0.f);

	const bool bScrolls = MaxScroll > 0.f;
	const float ScrollbarWidth = bScrolls ? BoxPadding * 0.5f : 0.f;
	const float RowWidth = Width - Inset * 2.f - (bScrolls ? ScrollbarWidth + Inset : 0.f);

	// Scrolled just far enough to bring the highlighted row fully into the box.
	// Rows on either side of it are drawn cut off, the way a web page would.
	float Scroll = 0.f;
	if (Selected > 0)
	{
		const float RowTop = (Selected - 1) * EntryStride;
		Scroll = FMath::Clamp(RowTop + EntryHeight - InnerHeight, 0.f, RowTop);
	}
	Scroll = FMath::Clamp(Scroll, 0.f, MaxScroll);

	for (int32 Row = 0; Row < Slots.Num(); ++Row)
	{
		const float EntryTop = InnerTop - Scroll + Row * EntryStride;
		if (EntryTop + EntryHeight <= InnerTop || EntryTop >= InnerBottom)
		{
			continue;
		}

		const FDeceiveSaveInfo& Info = Slots[Row];
		const bool bSelected = (Selected == Row + 1) && bFocused;

		DrawClippedRect(DialogueBackgroundColor, Left + Inset, EntryTop, RowWidth, EntryHeight,
			InnerTop, InnerBottom);
		DrawClippedBorder(Left + Inset, EntryTop, RowWidth, EntryHeight,
			bSelected ? ChoiceColor : SaveRecordColor, bSelected ? 4.f : 2.f, InnerTop, InnerBottom);

		// Glyphs cannot be drawn in half, so a line is left out until its own
		// band of the row is inside the box.
		const float FirstLineTop = EntryTop + BoxPadding * 0.4f;
		if (FirstLineTop >= InnerTop && FirstLineTop + RowHeight <= InnerBottom)
		{
			DrawText(FString::Printf(TEXT("%d. %s"),
				Info.Slot + 1, *Info.SavedAt.ToString(TEXT("%Y-%m-%d %H:%M"))),
				bSelected ? ChoiceColor : FLinearColor::White,
				Left + Inset + BoxPadding, FirstLineTop, Font, DialogueTextScale);
		}

		const float SecondLineTop = FirstLineTop + RowHeight;
		if (SecondLineTop >= InnerTop && SecondLineTop + RowHeight <= InnerBottom)
		{
			DrawText(FString::Printf(TEXT("%s · 아이템 %d개"),
				*DeceiveAreaDisplayName(Info.Area).ToString(), Info.ItemCount),
				ChoiceDimColor,
				Left + Inset + BoxPadding, SecondLineTop, Font, DialogueTextScale);
		}
	}

	if (!bScrolls)
	{
		return;
	}

	// Scrollbar: a track down the right inside edge, with a thumb as tall a
	// fraction of it as the box is of the whole list.
	const float TrackLeft = Left + Width - Inset - ScrollbarWidth;
	DrawRect(DialogueBackgroundColor, TrackLeft, InnerTop, ScrollbarWidth, InnerHeight);

	const float ThumbHeight = InnerHeight * InnerHeight / ContentHeight;
	const float ThumbTop = InnerTop + (InnerHeight - ThumbHeight) * Scroll / MaxScroll;
	DrawRect(SaveRecordColor, TrackLeft, ThumbTop, ScrollbarWidth, ThumbHeight);
}

void ADeceiveHUD::DrawNotice(const ASpriteCharacter& PlayerCharacter)
{
	const FString Text = PlayerCharacter.GetNoticeText().ToString();
	if (Text.IsEmpty())
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();

	float TextWidth = 0.f;
	float TextHeight = 0.f;
	GetTextSize(Text, TextWidth, TextHeight, Font, DialogueTextScale);

	const float BoxWidth = TextWidth + BoxPadding * 3.f;
	const float BoxHeight = TextHeight + BoxPadding * 1.5f;
	const float BoxLeft = (Canvas->SizeX - BoxWidth) * 0.5f;
	// Above the middle, so it is clear of the dialogue box and of the menu's rows.
	const float BoxTop = Canvas->SizeY * 0.3f;

	DrawRect(DialogueBackgroundColor, BoxLeft, BoxTop, BoxWidth, BoxHeight);
	DrawBorder(BoxLeft, BoxTop, BoxWidth, BoxHeight, InventoryBorderColor, 2.f);
	DrawText(Text, FLinearColor::White, BoxLeft + BoxPadding * 1.5f, BoxTop + BoxPadding * 0.75f,
		Font, DialogueTextScale);
}

void ADeceiveHUD::DrawInventoryTab(const ASpriteCharacter& PlayerCharacter, float Left, float Top, float Width, float Height)
{
	const UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	if (!GameInstance)
	{
		return;
	}

	const UDeceiveDataSubsystem* Data = GameInstance->GetSubsystem<UDeceiveDataSubsystem>();
	UFont* Font = GEngine->GetLargeFont();

	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, Font, DialogueTextScale);

	const TArray<FName>& Carried = GameInstance->GetInventory();
	const int32 Selected = PlayerCharacter.GetSelectedItemIndex();
	const bool bGridFocused = !PlayerCharacter.IsFocusOnTabs();

	// --- left: a grid of slots, six to a row --------------------------------
	constexpr int32 Columns = UDeceiveGameInstance::InventoryColumns;
	const float GridWidth = Width * 0.46f;
	// Columns of slots with a quarter-slot gap between them fill GridWidth.
	const float SlotSize = GridWidth / (Columns + (Columns - 1) * 0.25f);
	const float SlotGap = SlotSize * 0.25f;
	const float IconInset = SlotSize * 0.12f;

	for (int32 Slot = 0; Slot < UDeceiveGameInstance::InventoryCapacity; ++Slot)
	{
		const float SlotLeft = Left + (Slot % Columns) * (SlotSize + SlotGap);
		const float SlotTop = Top + (Slot / Columns) * (SlotSize + SlotGap);

		DrawRect(InventorySlotColor, SlotLeft, SlotTop, SlotSize, SlotSize);

		if (Carried.IsValidIndex(Slot))
		{
			FInventoryItemData Entry;
			const FString TexturePath = (Data && Data->FindInventoryItem(Carried[Slot], Entry))
				? Entry.TexturePath
				: FString();

			if (UTexture2D* Icon = ResolveItemTexture(TexturePath))
			{
				DrawTexture(Icon, SlotLeft + IconInset, SlotTop + IconInset,
					SlotSize - IconInset * 2.f, SlotSize - IconInset * 2.f, 0.f, 0.f, 1.f, 1.f);
			}
		}

		const bool bSelected = (Slot == Selected);
		DrawBorder(SlotLeft, SlotTop, SlotSize, SlotSize,
			(bSelected && bGridFocused) ? ChoiceColor : InventoryBorderColor,
			bSelected ? 4.f : 2.f);
	}

	// --- right: the highlighted slot, blown up ------------------------------
	const float DetailLeft = Left + Width * 0.54f;
	const float DetailWidth = Left + Width - DetailLeft;
	const float ImageSize = FMath::Min(DetailWidth * 0.40f, Height * 0.42f);

	const bool bFilled = Carried.IsValidIndex(Selected);

	FInventoryItemData Item;
	if (bFilled && (!Data || !Data->FindInventoryItem(Carried[Selected], Item)))
	{
		// Still name it, so a missing data entry is obvious rather than silent.
		Item.Name = FText::FromName(Carried[Selected]);
	}

	// Picture.
	DrawRect(InventorySlotColor, DetailLeft, Top, ImageSize, ImageSize);
	if (bFilled)
	{
		if (UTexture2D* Texture = ResolveItemTexture(Item.TexturePath))
		{
			DrawTexture(Texture, DetailLeft + IconInset, Top + IconInset,
				ImageSize - IconInset * 2.f, ImageSize - IconInset * 2.f, 0.f, 0.f, 1.f, 1.f);
		}
	}
	DrawBorder(DetailLeft, Top, ImageSize, ImageSize, InventoryBorderColor, 2.f);

	// Name, on a bar beside the picture.
	const float NameLeft = DetailLeft + ImageSize + SlotGap;
	const float NameWidth = DetailLeft + DetailWidth - NameLeft;
	const float NameHeight = RowHeight + BoxPadding;
	DrawRect(InventorySlotColor, NameLeft, Top, NameWidth, NameHeight);
	DrawBorder(NameLeft, Top, NameWidth, NameHeight, InventoryBorderColor, 2.f);
	if (bFilled)
	{
		DrawText(Item.Name.ToString(), DialogueSpeakerColor,
			NameLeft + BoxPadding * 0.5f, Top + BoxPadding * 0.5f, Font, DialogueTextScale);
	}

	// Description, in a box under the picture.
	const float DescriptionTop = Top + ImageSize + SlotGap;
	const float DescriptionHeight = Top + Height - DescriptionTop - RowHeight * 1.5f;
	DrawRect(InventorySlotColor, DetailLeft, DescriptionTop, DetailWidth, DescriptionHeight);
	DrawBorder(DetailLeft, DescriptionTop, DetailWidth, DescriptionHeight, InventoryBorderColor, 2.f);

	TArray<FString> Wrapped;
	WrapText(bFilled ? Item.Description.ToString() : InventoryNoSelectionText,
		Font, DialogueTextScale, DetailWidth - BoxPadding * 2.f, Wrapped);

	float TextCursor = DescriptionTop + BoxPadding;
	for (const FString& Line : Wrapped)
	{
		DrawText(Line, bFilled ? FLinearColor::White : ChoiceDimColor,
			DetailLeft + BoxPadding, TextCursor, Font, DialogueTextScale);
		TextCursor += RowHeight;
	}
}

void ADeceiveHUD::DrawTravelFade()
{
	const ADeceiveGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ADeceiveGameMode>() : nullptr;
	const float Alpha = GameMode ? GameMode->GetFadeAlpha() : 0.f;
	if (Alpha <= 0.f)
	{
		return;
	}

	DrawRect(FLinearColor(0.f, 0.f, 0.f, Alpha), 0.f, 0.f, Canvas->SizeX, Canvas->SizeY);
}

float ADeceiveHUD::GetNominalDialogueTop()
{
	// A speaker line plus one line of text: what most boxes come out as. Using a
	// fixed height keeps the prompt from drifting with whatever was last said.
	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, GEngine->GetLargeFont(), DialogueTextScale);

	const float BoxHeight = BoxPadding * 2.f + SpeakerGap + RowHeight * 2.f;
	return Canvas->SizeY - BoxHeight - BoxBottomMargin;
}

void ADeceiveHUD::DrawSidePanel(const TArray<FDialogueRow>& Rows, float PanelBottom)
{
	if (Rows.Num() == 0)
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();

	const float ScreenWidth = Canvas->SizeX;
	const float BoxWidth = ScreenWidth * DialogueWidthFraction;
	const float BoxLeft = (ScreenWidth - BoxWidth) * 0.5f;

	float RowHeight = 0.f;
	float WidestText = 0.f;
	for (const FDialogueRow& Row : Rows)
	{
		float TextWidth = 0.f;
		float TextHeight = 0.f;
		GetTextSize(Row.Text, TextWidth, TextHeight, Font, DialogueTextScale);
		WidestText = FMath::Max(WidestText, TextWidth);
		RowHeight = FMath::Max(RowHeight, TextHeight);
	}

	const float PanelWidth = FMath::Min(WidestText + BoxPadding * 2.f, BoxWidth);
	const float PanelHeight = BoxPadding * 2.f + RowHeight * Rows.Num();
	const float PanelLeft = BoxLeft + BoxWidth - PanelWidth;
	const float PanelTop = PanelBottom - PanelHeight;

	DrawRect(DialogueBackgroundColor, PanelLeft, PanelTop, PanelWidth, PanelHeight);

	float Cursor = PanelTop + BoxPadding;
	for (const FDialogueRow& Row : Rows)
	{
		DrawText(Row.Text, Row.Color, PanelLeft + BoxPadding, Cursor, Font, DialogueTextScale);
		Cursor += RowHeight;
	}
}

void ADeceiveHUD::DrawInteractPrompt(const ASpriteCharacter& PlayerCharacter)
{
	AActor* Target = PlayerCharacter.GetCurrentInteractable();
	if (!Target)
	{
		return;
	}

	// Mid-travel the doorway is still in range, so its prompt would flash back up
	// between the dialogue box closing and the screen going black.
	const ADeceiveGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ADeceiveGameMode>() : nullptr;
	if (GameMode && GameMode->IsTravelling())
	{
		return;
	}

	// The target names its own wording, so a crate says one thing and a doorway another.
	const FText Label = IInteractable::Execute_GetInteractPrompt(Target);
	const FString Text = Label.IsEmpty() ? InteractPrompt : Label.ToString();

	TArray<FDialogueRow> Rows;
	Rows.Add({ChoiceCursor + Text, ChoiceColor});

	// Same place the choice list appears, so the eye never has to move.
	DrawSidePanel(Rows, GetNominalDialogueTop() - ChoicePanelGap);
}

void ADeceiveHUD::DrawDialogueBox(const ASpriteCharacter& PlayerCharacter)
{
	UFont* Font = GEngine->GetLargeFont();

	const float ScreenWidth = Canvas->SizeX;
	const float ScreenHeight = Canvas->SizeY;
	const float BoxWidth = ScreenWidth * DialogueWidthFraction;
	const float BoxLeft = (ScreenWidth - BoxWidth) * 0.5f;
	const float TextWidthLimit = BoxWidth - BoxPadding * 2.f;

	const bool bAwaitingChoice = PlayerCharacter.IsAwaitingChoice();

	// Build every row first so the box can be sized to fit them.
	TArray<FDialogueRow> Rows;

	const FString Speaker = PlayerCharacter.GetDialogueSpeaker().ToString();
	if (!Speaker.IsEmpty())
	{
		Rows.Add({Speaker, DialogueSpeakerColor});
	}

	TArray<FString> Wrapped;
	WrapText(PlayerCharacter.GetCurrentDialogueLine().ToString(), Font, DialogueTextScale, TextWidthLimit, Wrapped);
	for (const FString& WrappedLine : Wrapped)
	{
		Rows.Add({WrappedLine, FLinearColor::White});
	}

	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, Font, DialogueTextScale);

	// The speaker row gets a little air under it.
	const float ExtraSpeakerGap = Speaker.IsEmpty() ? 0.f : SpeakerGap;
	const float BoxHeight = BoxPadding * 2.f + ExtraSpeakerGap + RowHeight * Rows.Num();
	const float BoxTop = ScreenHeight - BoxHeight - BoxBottomMargin;

	DrawRect(DialogueBackgroundColor, BoxLeft, BoxTop, BoxWidth, BoxHeight);

	float Cursor = BoxTop + BoxPadding;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (!Rows[Index].Text.IsEmpty())
		{
			DrawText(Rows[Index].Text, Rows[Index].Color, BoxLeft + BoxPadding, Cursor, Font, DialogueTextScale);
		}

		Cursor += RowHeight;
		if (Index == 0 && !Speaker.IsEmpty())
		{
			Cursor += SpeakerGap;
		}
	}

	if (bAwaitingChoice)
	{
		DrawChoicePanel(PlayerCharacter, BoxTop);
	}

	const FString& Hint = bAwaitingChoice ? ChoiceHint : DialogueHint;
	if (!Hint.IsEmpty())
	{
		float HintWidth = 0.f;
		float HintHeight = 0.f;
		GetTextSize(Hint, HintWidth, HintHeight, Font, 1.f);

		DrawText(Hint, FLinearColor(0.7f, 0.7f, 0.7f, 1.f),
			BoxLeft + BoxWidth - BoxPadding - HintWidth,
			BoxTop + BoxHeight - BoxPadding * 0.5f - HintHeight * 0.5f,
			Font, 1.f);
	}
}

void ADeceiveHUD::DrawChoicePanel(const ASpriteCharacter& PlayerCharacter, float BoxTop)
{
	const int32 Selected = PlayerCharacter.GetSelectedChoiceIndex();

	// A blank cursor-width indent keeps both labels on the same left edge.
	FString Blank;
	for (int32 Index = 0; Index < ChoiceCursor.Len(); ++Index)
	{
		Blank.AppendChar(TEXT(' '));
	}

	TArray<FDialogueRow> Options;
	Options.Add({(Selected == 0 ? ChoiceCursor : Blank) + PlayerCharacter.GetChoiceLabelA().ToString(),
		(Selected == 0) ? ChoiceColor : ChoiceDimColor});
	Options.Add({(Selected == 1 ? ChoiceCursor : Blank) + PlayerCharacter.GetChoiceLabelB().ToString(),
		(Selected == 1) ? ChoiceColor : ChoiceDimColor});

	DrawSidePanel(Options, BoxTop - ChoicePanelGap);
}

void ADeceiveHUD::DrawChoiceSummary()
{
	const UDeceiveGameInstance* GameInstance = GetGameInstance<UDeceiveGameInstance>();
	if (!GameInstance)
	{
		return;
	}

	UFont* Font = GEngine->GetLargeFont();

	const float ScreenWidth = Canvas->SizeX;
	const float ScreenHeight = Canvas->SizeY;
	const float TextWidthLimit = ScreenWidth * DialogueWidthFraction - BoxPadding * 2.f;
	const float Left = (ScreenWidth - ScreenWidth * DialogueWidthFraction) * 0.5f + BoxPadding;

	DrawRect(SummaryBackgroundColor, 0.f, 0.f, ScreenWidth, ScreenHeight);

	TArray<FDialogueRow> Rows;
	Rows.Add({SummaryTitle, DialogueSpeakerColor});
	Rows.Add({FString(), FLinearColor::White});

	const TArray<FChoiceRecord>& Records = GameInstance->GetChoiceRecords();
	if (Records.Num() == 0)
	{
		Rows.Add({SummaryEmptyText, FLinearColor::White});
	}

	TArray<FString> Wrapped;
	for (const FChoiceRecord& Record : Records)
	{
		const FString Question = Record.Speaker.IsEmpty()
			? Record.Question.ToString()
			: FString::Printf(TEXT("%s — %s"), *Record.Speaker.ToString(), *Record.Question.ToString());

		WrapText(Question, Font, SummaryTextScale, TextWidthLimit, Wrapped);
		for (const FString& Line : Wrapped)
		{
			Rows.Add({Line, FLinearColor(0.75f, 0.75f, 0.75f, 1.f)});
		}

		const FString Answer = FString::Printf(TEXT("    → %s. %s"),
			Record.bChoseB ? TEXT("B") : TEXT("A"), *Record.ChosenLabel.ToString());

		WrapText(Answer, Font, SummaryTextScale, TextWidthLimit, Wrapped);
		for (const FString& Line : Wrapped)
		{
			Rows.Add({Line, ChoiceColor});
		}

		Rows.Add({FString(), FLinearColor::White});
	}

	float Unused = 0.f;
	float RowHeight = 0.f;
	GetTextSize(TEXT("Ag"), Unused, RowHeight, Font, SummaryTextScale);

	float Cursor = (ScreenHeight - RowHeight * Rows.Num()) * 0.5f;
	for (const FDialogueRow& Row : Rows)
	{
		if (!Row.Text.IsEmpty())
		{
			DrawText(Row.Text, Row.Color, Left, Cursor, Font, SummaryTextScale);
		}
		Cursor += RowHeight;
	}
}

void ADeceiveHUD::WrapText(const FString& Text, UFont* Font, float Scale, float MaxWidth, TArray<FString>& OutLines)
{
	OutLines.Reset();

	FString Current;
	for (const TCHAR Character : Text)
	{
		if (Character == TEXT('\n'))
		{
			OutLines.Add(Current);
			Current.Empty();
			continue;
		}

		float Width = 0.f;
		float Height = 0.f;
		GetTextSize(Current + Character, Width, Height, Font, Scale);

		if (Width > MaxWidth && !Current.IsEmpty())
		{
			// Break at the last space when there is one, so words stay whole.
			// Korean runs often have none, in which case the break is mid-run.
			int32 SpaceIndex = INDEX_NONE;
			if (Current.FindLastChar(TEXT(' '), SpaceIndex))
			{
				OutLines.Add(Current.Left(SpaceIndex));
				Current = Current.RightChop(SpaceIndex + 1);
			}
			else
			{
				OutLines.Add(Current);
				Current.Empty();
			}
		}

		Current.AppendChar(Character);
	}

	OutLines.Add(Current);
}
