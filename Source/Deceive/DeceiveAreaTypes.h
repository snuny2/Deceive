#pragma once

#include "CoreMinimal.h"
#include "DeceiveAreaTypes.generated.h"

/** The playable areas. Only one of them exists in the world at a time. */
UENUM(BlueprintType)
enum class EDeceiveArea : uint8
{
	Alley,
	Building,
	NextStreet
};

/** Readable name for logs and editor labels. */
inline const TCHAR* DeceiveAreaName(EDeceiveArea Area)
{
	switch (Area)
	{
	case EDeceiveArea::Alley:		return TEXT("Alley");
	case EDeceiveArea::Building:	return TEXT("Building");
	case EDeceiveArea::NextStreet:	return TEXT("NextStreet");
	}

	return TEXT("Unknown");
}

/** Parses the name a data file uses. Returns false when it matches nothing. */
inline bool DeceiveAreaFromName(const FString& Name, EDeceiveArea& OutArea)
{
	if (Name.Equals(TEXT("Alley"), ESearchCase::IgnoreCase))		{ OutArea = EDeceiveArea::Alley; return true; }
	if (Name.Equals(TEXT("Building"), ESearchCase::IgnoreCase))		{ OutArea = EDeceiveArea::Building; return true; }
	if (Name.Equals(TEXT("NextStreet"), ESearchCase::IgnoreCase))	{ OutArea = EDeceiveArea::NextStreet; return true; }

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
	}

	return FColor::White;
}
