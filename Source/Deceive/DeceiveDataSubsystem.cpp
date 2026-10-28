#include "DeceiveDataSubsystem.h"

#include "DeceiveLog.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UDeceiveDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadData();
}

void UDeceiveDataSubsystem::ReloadData()
{
	LoadDialogues();
	LoadItems();

	UE_LOG(LogDeceive, Log, TEXT("Data loaded: %d dialogue(s), %d item(s)."),
		Dialogues.Num(), Items.Num());
}

TSharedPtr<FJsonObject> UDeceiveDataSubsystem::LoadDataFile(const TCHAR* FileName)
{
	const FString FullPath = FPaths::ProjectContentDir() / TEXT("Data") / FileName;

	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FullPath))
	{
		UE_LOG(LogDeceive, Warning, TEXT("Data file not found: %s"), *FullPath);
		return nullptr;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogDeceive, Warning, TEXT("Data file is not valid JSON: %s"), *FullPath);
		return nullptr;
	}

	return Root;
}

TArray<FText> UDeceiveDataSubsystem::ReadTextArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
{
	TArray<FText> Result;

	const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
	if (Object->TryGetArrayField(Field, Values))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			Result.Add(FText::FromString(Value->AsString()));
		}
	}

	return Result;
}

FDialogueChoice UDeceiveDataSubsystem::ReadChoice(const TSharedPtr<FJsonObject>& Object)
{
	FDialogueChoice Choice;
	if (!Object.IsValid())
	{
		return Choice;
	}

	Choice.Label = FText::FromString(Object->GetStringField(TEXT("label")));
	Choice.ResultLines = ReadTextArray(Object, TEXT("resultLines"));

	return Choice;
}

void UDeceiveDataSubsystem::LoadDialogues()
{
	Dialogues.Reset();

	const TSharedPtr<FJsonObject> Root = LoadDataFile(TEXT("Dialogues.json"));
	if (!Root.IsValid())
	{
		return;
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Root->Values)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Entry.Value->TryGetObject(Object))
		{
			continue;
		}

		FDialogueData Dialogue;
		Dialogue.Speaker = FText::FromString((*Object)->GetStringField(TEXT("speaker")));
		Dialogue.ChoiceId = FName(*(*Object)->GetStringField(TEXT("choiceId")));
		Dialogue.Lines = ReadTextArray(*Object, TEXT("lines"));
		Dialogue.Question = FText::FromString((*Object)->GetStringField(TEXT("question")));

		const TSharedPtr<FJsonObject>* ChoiceObject = nullptr;
		if ((*Object)->TryGetObjectField(TEXT("choiceA"), ChoiceObject))
		{
			Dialogue.ChoiceA = ReadChoice(*ChoiceObject);
		}
		if ((*Object)->TryGetObjectField(TEXT("choiceB"), ChoiceObject))
		{
			Dialogue.ChoiceB = ReadChoice(*ChoiceObject);
		}

		Dialogues.Add(FName(*Entry.Key), Dialogue);
	}
}

void UDeceiveDataSubsystem::LoadItems()
{
	Items.Reset();

	const TSharedPtr<FJsonObject> Root = LoadDataFile(TEXT("Items.json"));
	if (!Root.IsValid())
	{
		return;
	}

	for (const TPair<FString, TSharedPtr<FJsonValue>>& Entry : Root->Values)
	{
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (!Entry.Value->TryGetObject(Object))
		{
			continue;
		}

		FItemData Item;
		Item.ItemId = FName(*Entry.Key);
		Item.Label = (*Object)->GetStringField(TEXT("label"));
		Item.InteractPrompt = FText::FromString((*Object)->GetStringField(TEXT("prompt")));
		Item.DialogueId = FName(*(*Object)->GetStringField(TEXT("dialogue")));
		Item.Size = (*Object)->GetNumberField(TEXT("size"));

		const FString AreaName = (*Object)->GetStringField(TEXT("area"));
		if (!DeceiveAreaFromName(AreaName, Item.Area))
		{
			UE_LOG(LogDeceive, Warning, TEXT("Item '%s' names an unknown area '%s'; skipped."),
				*Entry.Key, *AreaName);
			continue;
		}

		const TSharedPtr<FJsonObject>* LocationObject = nullptr;
		if ((*Object)->TryGetObjectField(TEXT("location"), LocationObject))
		{
			Item.Location = FVector(
				(*LocationObject)->GetNumberField(TEXT("x")),
				(*LocationObject)->GetNumberField(TEXT("y")),
				(*LocationObject)->GetNumberField(TEXT("z")));
		}

		Items.Add(Item.ItemId, Item);
	}
}

bool UDeceiveDataSubsystem::FindDialogue(FName DialogueId, FDialogueData& OutDialogue) const
{
	if (const FDialogueData* Found = Dialogues.Find(DialogueId))
	{
		OutDialogue = *Found;
		return true;
	}

	return false;
}

bool UDeceiveDataSubsystem::FindItem(FName ItemId, FItemData& OutItem) const
{
	if (const FItemData* Found = Items.Find(ItemId))
	{
		OutItem = *Found;
		return true;
	}

	return false;
}

void UDeceiveDataSubsystem::GetItemsInArea(EDeceiveArea Area, TArray<FItemData>& OutItems) const
{
	OutItems.Reset();

	for (const TPair<FName, FItemData>& Entry : Items)
	{
		if (Entry.Value.Area == Area)
		{
			OutItems.Add(Entry.Value);
		}
	}
}
