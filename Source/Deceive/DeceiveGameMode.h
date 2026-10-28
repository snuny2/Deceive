#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "GameFramework/GameModeBase.h"
#include "DeceiveGameMode.generated.h"

class ACrossAlley;
class ADeceiveArea;
class ARoomArea;

/** Builds the starting area and swaps areas as the player walks between them. */
UCLASS()
class DECEIVE_API ADeceiveGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADeceiveGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** Tears down the current area, builds the target one and moves the player into it. */
	UFUNCTION(BlueprintCallable, Category = "Deceive")
	void TravelTo(EDeceiveArea Area, FName EntryName);

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ACrossAlley> AlleyClass;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> BuildingClass;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> NextStreetClass;

	/**
	 * The placeholder beyond the map edge is laid out wider and shallower than the
	 * building, so the two placeholder rooms are easy to tell apart.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "200"))
	float NextStreetWidth = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "200"))
	float NextStreetDepth = 500.f;

private:
	ADeceiveArea* SpawnArea(EDeceiveArea Area);

	/** Gives a spawned area a readable name in the outliner. */
	static void LabelArea(ADeceiveArea* Area, EDeceiveArea AreaId);

	UPROPERTY(Transient)
	TObjectPtr<ADeceiveArea> CurrentArea;
};
