#pragma once

#include "CoreMinimal.h"
#include "DeceiveDialogueTypes.h"
#include "DeceiveItemTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DeceiveDataSubsystem.generated.h"

class FJsonObject;

/**
 * Reads the dialogue and item tables from Content/Data. Keeping them out of C++
 * means writing lines does not need a rebuild.
 */
UCLASS()
class DECEIVE_API UDeceiveDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** False when no entry carries that id. */
	UFUNCTION(BlueprintCallable, Category = "Data")
	bool FindDialogue(FName DialogueId, FDialogueData& OutDialogue) const;

	UFUNCTION(BlueprintCallable, Category = "Data")
	bool FindItem(FName ItemId, FItemData& OutItem) const;

	/** Everything that belongs in one area, in file order. */
	void GetItemsInArea(EDeceiveArea Area, TArray<FItemData>& OutItems) const;

	/** Re-reads both files. Also reachable from the console as `ReloadData`. */
	UFUNCTION(Exec, BlueprintCallable, Category = "Data")
	void ReloadData();

private:
	/** Content/Data/<FileName>, parsed as an object of named entries. */
	static TSharedPtr<FJsonObject> LoadDataFile(const TCHAR* FileName);

	void LoadDialogues();
	void LoadItems();

	static FDialogueChoice ReadChoice(const TSharedPtr<FJsonObject>& Object);
	static TArray<FText> ReadTextArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field);

	TMap<FName, FDialogueData> Dialogues;
	TMap<FName, FItemData> Items;
};
