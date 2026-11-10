#include "PushableBox.h"

#include "Components/StaticMeshComponent.h"
#include "DeceiveGameInstance.h"
#include "DeceiveSpriteMaterial.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float CubeMeshSize = 100.f;

	/** Crates this near each other count as touching, and so as a queue. */
	constexpr float ContactGap = 12.f;
}

APushableBox::APushableBox()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Mesh->SetStaticMesh(CubeMeshFinder.Object);
	// Solid: the player leans on it rather than walking through it.
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void APushableBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Mesh->SetRelativeScale3D(FVector(BoxSize / CubeMeshSize));

	if (UMaterialInstanceDynamic* Material =
		MakeTexturedMaterial(this, BaseMaterial, BoxTexture, DeceiveOpaqueSpriteMaterialPath))
	{
		Mesh->SetMaterial(0, Material);
	}
}

void APushableBox::Seat(const FVector& SlotLocation)
{
	bSeated = true;
	SetActorLocation(FVector(SlotLocation.X, SlotLocation.Y, GetActorLocation().Z));
	Remember();
}

void APushableBox::Remember() const
{
	if (UDeceiveGameInstance* Game = GetGameInstance<UDeceiveGameInstance>())
	{
		Game->RememberCratePosition(ColourId, GetActorLocation());
	}
}

void APushableBox::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bSeated)
	{
		FollowPlayer(DeltaSeconds);
	}

	// Colour is all that tells the crates apart until there are textures, and a
	// drawn box reads through the desks it is being pushed between.
	const float Half = BoxSize * 0.5f;
	DrawDebugBox(GetWorld(), GetActorLocation(), FVector(Half), Colour,
		false, -1.f, SDPG_Foreground, 6.f);
}

void APushableBox::FollowPlayer(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}

	// The player presses into a solid crate and so stands still; the crate must
	// never block the crate's own move either. Both are the same fix.
	Mesh->IgnoreActorWhenMoving(Player, true);

	// What the player is *trying* to do, since leaning on the crate leaves them
	// with no velocity to read.
	FVector Push = Player->GetLastMovementInputVector();
	if (Push.IsNearlyZero())
	{
		Push = Player->GetVelocity();
	}

	Push = Push.GetSafeNormal2D();
	if (Push.IsNearlyZero())
	{
		Report(TEXT("no input"));
		return;
	}

	// One axis at a time, picked by the way the player is walking.
	const bool bAlongX = FMath::Abs(Push.X) > FMath::Abs(Push.Y);
	const float Direction = FMath::Sign(bAlongX ? Push.X : Push.Y);

	const FVector ToBox = GetActorLocation() - Player->GetActorLocation();
	const float Along = bAlongX ? ToBox.X : ToBox.Y;
	const float Across = FMath::Abs(bAlongX ? ToBox.Y : ToBox.X);

	// Only a push, never a pull: the crate has to be the way they are walking.
	if (Along * Direction <= 0.f)
	{
		Report(TEXT("behind"));
		return;
	}

	const float Half = BoxSize * 0.5f;
	const float PlayerHalf = FMath::Max(Player->GetSimpleCollisionRadius(), 1.f);

	// Touching along the push, and squarely enough across it: a shoulder barely
	// catching the corner leaves the crate where it is.
	if (FMath::Abs(Along) > Half + PlayerHalf + PushReach)
	{
		Report(FString::Printf(TEXT("far %.0f > %.0f"), FMath::Abs(Along), Half + PlayerHalf + PushReach));
		return;
	}

	const float Contact = (Half + PlayerHalf) - Across;
	const float Needed = FMath::Min(BoxSize, PlayerHalf * 2.f) * MinContactFraction;
	if (Contact < Needed)
	{
		Report(FString::Printf(TEXT("corner %.0f < %.0f"), Contact, Needed));
		return;
	}

	// Crates ahead come along, up to a point: two move slowly, three do not move.
	TArray<APushableBox*> Chain;
	Chain.Add(this);

	for (APushableBox* Last = this;;)
	{
		APushableBox* Next = Last->FindCrateAhead(bAlongX, Direction);
		if (!Next || Chain.Contains(Next))
		{
			break;
		}

		if (Next->IsSeated())
		{
			// A settled crate is part of the word now, so it is a wall.
			Report(TEXT("seated ahead"));
			return;
		}

		Chain.Add(Next);
		Last = Next;

		if (Chain.Num() >= MaxPushedCrates + 1)
		{
			break;
		}
	}

	if (Chain.Num() > MaxPushedCrates)
	{
		Report(FString::Printf(TEXT("too many (%d)"), Chain.Num()));
		return;
	}

	const float Speed = (Chain.Num() >= 2) ? PushSpeed / ChainSlowdown : PushSpeed;
	const float Step = Direction * Speed * DeltaSeconds;
	const FVector Offset = bAlongX ? FVector(Step, 0.f, 0.f) : FVector(0.f, Step, 0.f);

	// Front of the queue first, so none of them is ever shoved into another.
	for (int32 Index = Chain.Num() - 1; Index >= 0; --Index)
	{
		Chain[Index]->Shove(Offset);
	}

	Report(FString::Printf(TEXT("push %.1f x%d"), Step, Chain.Num()));
}

void APushableBox::Shove(const FVector& Offset)
{
	// Swept, so a crate stops against a desk or a wall instead of sliding into it.
	const FVector Before = GetActorLocation();
	FHitResult Hit;
	AddActorWorldOffset(Offset, true, &Hit);

	if (Hit.bStartPenetrating && GetActorLocation().Equals(Before)
		&& !IsBlockedAt(Before + Offset))
	{
		// Already touching something at the start of the sweep, which stops the
		// move dead even when the way ahead is clear. Step out of it by hand,
		// but never into another crate.
		AddActorWorldOffset(Offset, false);
	}

	if (!GetActorLocation().Equals(Before))
	{
		Remember();
	}
}

APushableBox* APushableBox::FindCrateAhead(bool bAlongX, float Direction) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APushableBox* Nearest = nullptr;
	float NearestGap = TNumericLimits<float>::Max();

	for (TActorIterator<APushableBox> It(World); It; ++It)
	{
		APushableBox* Other = *It;
		if (Other == this || !IsValid(Other))
		{
			continue;
		}

		const FVector ToOther = Other->GetActorLocation() - GetActorLocation();
		const float Along = bAlongX ? ToOther.X : ToOther.Y;
		const float Across = FMath::Abs(bAlongX ? ToOther.Y : ToOther.X);
		const float HalfSpan = (BoxSize + Other->BoxSize) * 0.5f;

		// Lined up across the push, and all but touching along it.
		if (Along * Direction <= 0.f || Across >= HalfSpan)
		{
			continue;
		}

		const float Gap = FMath::Abs(Along) - HalfSpan;
		if (Gap <= ContactGap && Gap < NearestGap)
		{
			Nearest = Other;
			NearestGap = Gap;
		}
	}

	return Nearest;
}

bool APushableBox::IsBlockedAt(const FVector& At) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	// Slightly under size, so merely touching a desk is not being inside one.
	const FCollisionShape Shape = FCollisionShape::MakeBox(FVector(BoxSize * 0.49f));

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PushableBoxFreeSpace), false, this);
	if (const APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		Params.AddIgnoredActor(PlayerController->GetPawn());
	}

	return World->OverlapBlockingTestByChannel(At, FQuat::Identity,
		Mesh->GetCollisionObjectType(), Shape, Params);
}

void APushableBox::Report(const FString& What) const
{
	if (!bDebugPush || !GEngine)
	{
		return;
	}

	GEngine->AddOnScreenDebugMessage(static_cast<uint64>(GetUniqueID()), 0.5f, Colour,
		FString::Printf(TEXT("%s: %s"), *ColourId.ToString(), *What));
}
