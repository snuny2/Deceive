#include "DeceiveGameInstance.h"

#include "DeceiveLog.h"

void UDeceiveGameInstance::SetFlag(FName FlagName, int32 Value)
{
	if (FlagName.IsNone())
	{
		return;
	}

	Flags.Add(FlagName, Value);
	UE_LOG(LogDeceive, Log, TEXT("Flag set: %s = %d"), *FlagName.ToString(), Value);
}

int32 UDeceiveGameInstance::GetFlag(FName FlagName, int32 DefaultValue) const
{
	const int32* Found = Flags.Find(FlagName);
	return Found ? *Found : DefaultValue;
}

bool UDeceiveGameInstance::HasFlag(FName FlagName) const
{
	return Flags.Contains(FlagName);
}

void UDeceiveGameInstance::ClearFlag(FName FlagName)
{
	Flags.Remove(FlagName);
}

void UDeceiveGameInstance::RecordChoice(const FChoiceRecord& Record)
{
	// The answer itself lives in the flags; the record is only for the summary.
	SetFlag(Record.ChoiceId, Record.bChoseB ? 1 : 0);

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
	if (!HasFlag(ChoiceId))
	{
		return false;
	}

	bOutChoseB = GetFlag(ChoiceId) != 0;
	return true;
}

void UDeceiveGameInstance::ClearChoices()
{
	ChoiceRecords.Reset();
	Flags.Reset();
	bGameFinished = false;
}

void UDeceiveGameInstance::EndGame()
{
	bGameFinished = true;
	UE_LOG(LogDeceive, Log, TEXT("Game finished with %d recorded choice(s)."), ChoiceRecords.Num());
}
