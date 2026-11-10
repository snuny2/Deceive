#include "DeceiveDataSubsystem.h"

#include "DeceiveGameInstance.h"
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
	LoadInventory();
	LoadPlayer();

	UE_LOG(LogDeceive, Log, TEXT("Data loaded: %d dialogue(s), %d item(s), %d inventory entr(ies)."),
		Dialogues.Num(), Items.Num(), InventoryItems.Num());
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

	FString Label;
	if (Object->TryGetStringField(TEXT("label"), Label))
	{
		Choice.Label = FText::FromString(Label);
	}

	Choice.ResultLines = ReadTextArray(Object, TEXT("resultLines"));

	const TArray<TSharedPtr<FJsonValue>>* AddItems = nullptr;
	if (Object->TryGetArrayField(TEXT("addItems"), AddItems))
	{
		for (const TSharedPtr<FJsonValue>& Value : *AddItems)
		{
			Choice.AddItems.Add(FName(*Value->AsString()));
		}
	}

	const TSharedPtr<FJsonObject>* FlagsObject = nullptr;
	if (Object->TryGetObjectField(TEXT("setFlags"), FlagsObject))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Flag : (*FlagsObject)->Values)
		{
			Choice.SetFlags.Add(FName(*Flag.Key), static_cast<int32>(Flag.Value->AsNumber()));
		}
	}

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

		FString Field;
		if ((*Object)->TryGetStringField(TEXT("speaker"), Field))
		{
			Dialogue.Speaker = FText::FromString(Field);
		}
		if ((*Object)->TryGetStringField(TEXT("choiceId"), Field))
		{
			Dialogue.ChoiceId = FName(*Field);
		}
		if ((*Object)->TryGetStringField(TEXT("question"), Field))
		{
			Dialogue.Question = FText::FromString(Field);
		}

		Dialogue.Lines = ReadTextArray(*Object, TEXT("lines"));

		const TSharedPtr<FJsonObject>* ConditionObject = nullptr;
		if ((*Object)->TryGetObjectField(TEXT("condition"), ConditionObject))
		{
			Dialogue.ConditionFlag = FName(*(*ConditionObject)->GetStringField(TEXT("flag")));

			double ConditionValue = 1.0;
			(*ConditionObject)->TryGetNumberField(TEXT("value"), ConditionValue);
			Dialogue.ConditionValue = static_cast<int32>(ConditionValue);
		}

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
		// "dialogue" takes either one name or a list of them.
		const TArray<TSharedPtr<FJsonValue>>* DialogueList = nullptr;
		FString SingleDialogue;
		if ((*Object)->TryGetArrayField(TEXT("dialogue"), DialogueList))
		{
			for (const TSharedPtr<FJsonValue>& Value : *DialogueList)
			{
				Item.DialogueIds.Add(FName(*Value->AsString()));
			}
		}
		else if ((*Object)->TryGetStringField(TEXT("dialogue"), SingleDialogue))
		{
			Item.DialogueIds.Add(FName(*SingleDialogue));
		}

		double Size = 150.0;
		(*Object)->TryGetNumberField(TEXT("size"), Size);
		Item.Size = static_cast<float>(Size);

		(*Object)->TryGetBoolField(TEXT("placedOnly"), Item.bPlacedOnly);
		(*Object)->TryGetBoolField(TEXT("walkThrough"), Item.bWalkThrough);

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

void UDeceiveDataSubsystem::LoadInventory()
{
	InventoryItems.Reset();

	const TSharedPtr<FJsonObject> Root = LoadDataFile(TEXT("Inventory.json"));
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

		FInventoryItemData Item;
		Item.ItemId = FName(*Entry.Key);

		FString Field;
		if ((*Object)->TryGetStringField(TEXT("name"), Field))
		{
			Item.Name = FText::FromString(Field);
		}
		if ((*Object)->TryGetStringField(TEXT("description"), Field))
		{
			Item.Description = FText::FromString(Field);
		}
		(*Object)->TryGetStringField(TEXT("texture"), Item.TexturePath);
		(*Object)->TryGetBoolField(TEXT("placeable"), Item.bPlaceable);

		InventoryItems.Add(Item.ItemId, Item);
	}
}

bool UDeceiveDataSubsystem::FindInventoryItem(FName ItemId, FInventoryItemData& OutItem) const
{
	if (const FInventoryItemData* Found = InventoryItems.Find(ItemId))
	{
		OutItem = *Found;
		return true;
	}

	return false;
}

void UDeceiveDataSubsystem::LoadPlayer()
{
	// Defaults stand in whenever the file is missing a field, so the info tab
	// always has something to draw.
	PlayerData = FDeceivePlayerData();
	PlayerData.Name = FText::FromString(TEXT("이름 없음"));
	PlayerData.Status = FText::FromString(TEXT("멀쩡하다."));

	const TSharedPtr<FJsonObject> Root = LoadDataFile(TEXT("Player.json"));
	if (!Root.IsValid())
	{
		return;
	}

	FString Field;
	if (Root->TryGetStringField(TEXT("name"), Field))
	{
		PlayerData.Name = FText::FromString(Field);
	}
	if (Root->TryGetStringField(TEXT("status"), Field))
	{
		PlayerData.Status = FText::FromString(Field);
	}
	Root->TryGetStringField(TEXT("portrait"), PlayerData.PortraitPath);

	double MaxHealth = PlayerData.MaxHealth;
	Root->TryGetNumberField(TEXT("maxHealth"), MaxHealth);
	PlayerData.MaxHealth = FMath::Max(1, static_cast<int32>(MaxHealth));
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

bool UDeceiveDataSubsystem::ResolveDialogue(const TArray<FName>& Candidates, FDialogueData& OutDialogue, FName& OutDialogueId) const
{
	const UDeceiveGameInstance* GameInstance = Cast<UDeceiveGameInstance>(GetGameInstance());

	for (const FName& Candidate : Candidates)
	{
		const FDialogueData* Found = Dialogues.Find(Candidate);
		if (!Found)
		{
			UE_LOG(LogDeceive, Warning, TEXT("Dialogues.json has no entry '%s'."), *Candidate.ToString());
			continue;
		}

		// No condition means the entry always applies, so it belongs last.
		if (!Found->ConditionFlag.IsNone())
		{
			const int32 Current = GameInstance ? GameInstance->GetFlag(Found->ConditionFlag) : 0;
			if (Current != Found->ConditionValue)
			{
				continue;
			}
		}

		OutDialogue = *Found;
		OutDialogueId = Candidate;
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
