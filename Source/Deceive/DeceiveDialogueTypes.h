#pragma once

#include "CoreMinimal.h"
#include "DeceiveDialogueTypes.generated.h"

/** One option on a dialogue question, and what gets said when it is picked. */
USTRUCT(BlueprintType)
struct FDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
	TArray<FText> ResultLines;
};

/** What the player answered at one question, kept for the end-of-game summary. */
USTRUCT(BlueprintType)
struct FChoiceRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName ChoiceId;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText Question;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText ChosenLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	bool bChoseB = false;
};

/** A full exchange: some lines, then optionally a question with two answers. */
USTRUCT(BlueprintType)
struct FDialogueData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText Speaker;

	/**
	 * Identifies this question in the saved records. Give every question its own
	 * name; answering the same one again replaces the earlier record.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName ChoiceId;

	/** Said first, one box per entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
	TArray<FText> Lines;

	/** Leave empty to end after Lines instead of asking anything. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
	FText Question;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FDialogueChoice ChoiceA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FDialogueChoice ChoiceB;
};
