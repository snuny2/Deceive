#pragma once

#include "CoreMinimal.h"
#include "DeceiveMenuTypes.generated.h"

/** Tabs down the left edge of the menu screen, in the order they are drawn. */
UENUM(BlueprintType)
enum class EDeceiveMenuTab : uint8
{
	Info,
	Inventory,
	Save,
	Settings
};

static constexpr int32 DeceiveMenuTabCount = 4;

/** Wording on a tab. */
inline FText DeceiveMenuTabName(EDeceiveMenuTab Tab)
{
	switch (Tab)
	{
	case EDeceiveMenuTab::Info:			return FText::FromString(TEXT("정보"));
	case EDeceiveMenuTab::Inventory:	return FText::FromString(TEXT("인벤토리"));
	case EDeceiveMenuTab::Save:			return FText::FromString(TEXT("저장"));
	case EDeceiveMenuTab::Settings:		return FText::FromString(TEXT("설정"));
	}

	return FText::GetEmpty();
}
