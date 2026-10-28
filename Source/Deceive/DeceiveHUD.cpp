#include "DeceiveHUD.h"

#include "DeceiveGameInstance.h"
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

	if (PlayerCharacter->IsDialogueActive())
	{
		DrawDialogueBox(*PlayerCharacter);
	}
	else
	{
		DrawInteractPrompt(*PlayerCharacter);
	}
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
