#include "DeceiveGameMode.h"

#include "CrossAlley.h"
#include "GameFramework/PlayerStart.h"
#include "SpriteCharacter.h"

ADeceiveGameMode::ADeceiveGameMode()
{
	DefaultPawnClass = ASpriteCharacter::StaticClass();
	AlleyClass = ACrossAlley::StaticClass();
}

void ADeceiveGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AlleyClass && World->SpawnActor<ACrossAlley>(AlleyClass, FTransform::Identity))
	{
		// Drop the player into the crossing, just above the floor.
		World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), FTransform(FVector(0.f, 0.f, 120.f)));
	}
}
