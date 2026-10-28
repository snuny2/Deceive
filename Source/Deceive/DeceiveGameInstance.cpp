#include "DeceiveGameInstance.h"

#include "DeceiveLog.h"

void UDeceiveGameInstance::RecordChoice(const FChoiceRecord& Record)
{
	if (!Record.ChoiceId.IsNone())
	{
		const int32 Existing = ChoiceRecords.IndexOfByPredicate(
			[&Record](const FChoiceRecord& Candidate) { return Candidate.ChoiceId == Record.ChoiceId; });

		if (Existing != INDEX_NONE)
		{
			ChoiceRecords[Existing] = Record;
			return;
		}
	}

	ChoiceRecords.Add(Record);

	UE_LOG(LogDeceive, Log, TEXT("Choice recorded: %s = %s"),
		*Record.ChoiceId.ToString(), *Record.ChosenLabel.ToString());
}

bool UDeceiveGameInstance::TryGetChoice(FName ChoiceId, bool& bOutChoseB) const
{
	const FChoiceRecord* Found = ChoiceRecords.FindByPredicate(
		[ChoiceId](const FChoiceRecord& Candidate) { return Candidate.ChoiceId == ChoiceId; });

	if (!Found)
	{
		return false;
	}

	bOutChoseB = Found->bChoseB;
	return true;
}

void UDeceiveGameInstance::ClearChoices()
{
	ChoiceRecords.Reset();
	bGameFinished = false;
}

void UDeceiveGameInstance::EndGame()
{
	bGameFinished = true;
	UE_LOG(LogDeceive, Log, TEXT("Game finished with %d recorded choice(s)."), ChoiceRecords.Num());
}
