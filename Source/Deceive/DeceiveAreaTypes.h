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
