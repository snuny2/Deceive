#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.generated.h"

/** The playable areas. Only one of them exists in the world at a time. */
UENUM(BlueprintType)
enum class EDeceiveArea : uint8
{
	Alley,
	Building,
	NextStreet,
	LockedRoom,
	PuzzleRoom,
	StatueHall
};

/** Readable name for logs and editor labels. */
inline const TCHAR* DeceiveAreaName(EDeceiveArea Area)
{
	switch (Area)
	{
	case EDeceiveArea::Alley:		return TEXT("Alley");
	case EDeceiveArea::Building:	return TEXT("Building");
	case EDeceiveArea::NextStreet:	return TEXT("NextStreet");
	case EDeceiveArea::LockedRoom:	return TEXT("LockedRoom");
	case EDeceiveArea::PuzzleRoom:	return TEXT("PuzzleRoom");
	case EDeceiveArea::StatueHall:	return TEXT("StatueHall");
	}

	return TEXT("Unknown");
}

/** What the player is shown, as opposed to the name logs and data files use. */
inline FText DeceiveAreaDisplayName(EDeceiveArea Area)
{
	switch (Area)
	{
	case EDeceiveArea::Alley:		return FText::FromString(TEXT("골목길"));
	case EDeceiveArea::Building:	return FText::FromString(TEXT("건물 안"));
	case EDeceiveArea::NextStreet:	return FText::FromString(TEXT("다음 거리"));
	case EDeceiveArea::LockedRoom:	return FText::FromString(TEXT("잠긴 방"));
	case EDeceiveArea::PuzzleRoom:	return FText::FromString(TEXT("글자 방"));
	case EDeceiveArea::StatueHall:	return FText::FromString(TEXT("조각상 너머의 방"));
	}

	return FText::FromString(TEXT("알 수 없는 장소"));
}

/** Parses the name a data file uses. Returns false when it matches nothing. */
inline bool DeceiveAreaFromName(const FString& Name, EDeceiveArea& OutArea)
{
	if (Name.Equals(TEXT("Alley"), ESearchCase::IgnoreCase))		{ OutArea = EDeceiveArea::Alley; return true; }
	if (Name.Equals(TEXT("Building"), ESearchCase::IgnoreCase))		{ OutArea = EDeceiveArea::Building; return true; }
	if (Name.Equals(TEXT("NextStreet"), ESearchCase::IgnoreCase))	{ OutArea = EDeceiveArea::NextStreet; return true; }
	if (Name.Equals(TEXT("LockedRoom"), ESearchCase::IgnoreCase))	{ OutArea = EDeceiveArea::LockedRoom; return true; }
	if (Name.Equals(TEXT("PuzzleRoom"), ESearchCase::IgnoreCase))	{ OutArea = EDeceiveArea::PuzzleRoom; return true; }
	if (Name.Equals(TEXT("StatueHall"), ESearchCase::IgnoreCase))	{ OutArea = EDeceiveArea::StatueHall; return true; }

	return false;
}

/** Colour a portal leading to this area is drawn in while debugging. */
inline FColor DeceiveAreaColor(EDeceiveArea Area)
{
	switch (Area)
	{
	case EDeceiveArea::Alley:		return FColor::Yellow;
	case EDeceiveArea::Building:	return FColor::Green;
	case EDeceiveArea::NextStreet:	return FColor::Cyan;
	case EDeceiveArea::LockedRoom:	return FColor::Magenta;
	case EDeceiveArea::PuzzleRoom:	return FColor::Orange;
	case EDeceiveArea::StatueHall:	return FColor::Red;
	}

	return FColor::White;
}
