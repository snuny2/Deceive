#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.h"
#include "GameFramework/GameModeBase.h"
#include "DeceiveGameMode.generated.h"

class ACrossAlley;
class ADeceiveArea;
class ARoomArea;
class AWordPuzzleRoom;

/** Builds the starting area and swaps areas as the player walks between them. */
UCLASS()
class DECEIVE_API ADeceiveGameMode : public AGameModeBase
{
	GENERATED_BODY()

private:
	enum class EFadePhase : uint8
	{
		None,
		Out,
		In,
	};

public:
	ADeceiveGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Tears down the current area, builds the target one and moves the player into it. */
	UFUNCTION(BlueprintCallable, Category = "Deceive")
	void TravelTo(EDeceiveArea Area, FName EntryName);

	/**
	 * Fades to black, travels while the screen is covered, then fades back in.
	 * Ignored while a fade is already running, so a second interaction cannot
	 * start a travel on top of one in progress.
	 */
	UFUNCTION(BlueprintCallable, Category = "Deceive")
	void TravelToWithFade(EDeceiveArea Area, FName EntryName);

	/** The area actor standing in the world right now, if any. */
	ADeceiveArea* GetCurrentArea() const { return CurrentArea; }

	/** The area standing in the world right now. */
	UFUNCTION(BlueprintPure, Category = "Deceive")
	EDeceiveArea GetCurrentAreaId() const;

	/**
	 * True from the moment a fade starts until the screen is clear again. The
	 * fade's alpha is still 0 on its first frame, so this is what the HUD asks
	 * before drawing anything that should not outlive the doorway.
	 */
	UFUNCTION(BlueprintPure, Category = "Deceive")
	bool IsTravelling() const { return FadePhase != EFadePhase::None; }

	/** How black the screen is, eased: 0 is clear, 1 hides everything. */
	UFUNCTION(BlueprintPure, Category = "Deceive")
	float GetFadeAlpha() const;

	/** Seconds the screen takes to go black, and the same again to come back. */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "0.0"))
	float FadeDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ACrossAlley> AlleyClass;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> BuildingClass;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> NextStreetClass;

	/** The room behind the locked door at the end of the +Y arm. */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> LockedRoomClass;

	/** The long corridor west of the alley, with the statues in it. */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> StatueCorridorClass;

	/** The wide room above the corridor, behind the statue door. */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<ARoomArea> StatueHallClass;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	float StatueHallWidth = 2600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	float StatueHallDepth = 1400.f;

	/** The room below the alley, with the word puzzle in it. */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive")
	TSubclassOf<AWordPuzzleRoom> PuzzleRoomClass;

	/**
	 * The street west of the alley is a long corridor running up the screen: narrow
	 * across, deep into the screen, with its door on the side facing the alley.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "200"))
	float NextStreetWidth = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "200"))
	float NextStreetDepth = 2600.f;

	/** The room behind the locked door is a wide square, with room for a puzzle. */
	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "200"))
	float LockedRoomWidth = 1600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Deceive", meta = (ClampMin = "200"))
	float LockedRoomDepth = 1600.f;

private:
	ADeceiveArea* SpawnArea(EDeceiveArea Area);

	/** Gives a spawned area a readable name in the outliner. */
	static void LabelArea(ADeceiveArea* Area, EDeceiveArea AreaId);

	UPROPERTY(Transient)
	TObjectPtr<ADeceiveArea> CurrentArea;

	EFadePhase FadePhase = EFadePhase::None;

	/** Raw, uneased progress of the current phase. */
	float FadeProgress = 0.f;

	/** Where the pending travel goes once the screen is fully black. */
	EDeceiveArea PendingArea = EDeceiveArea::Alley;
	FName PendingEntry;
};
