#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DeceiveGameMode.generated.h"

class ACrossAlley;

/** Spawns the cross alley and a spawn point, so an empty level is enough to play. */
UCLASS()
class DECEIVE_API ADeceiveGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADeceiveGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ACrossAlley> AlleyClass;
};
