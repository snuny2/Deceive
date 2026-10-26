#pragma once

#include "CoreMinimal.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;

/** Name of the texture parameter on Paper2D's sprite materials. */
extern const FName DeceiveSpriteTextureParam;

/** Cut-out sprites, for characters. */
extern const TCHAR* DeceiveMaskedSpriteMaterialPath;

/** Solid surfaces, for walls and floors. */
extern const TCHAR* DeceiveOpaqueSpriteMaterialPath;

/**
 * Builds a dynamic material instance whose texture can be swapped.
 * When BaseMaterial is null, the Paper2D material at FallbackPath is used.
 */
UMaterialInstanceDynamic* MakeTexturedMaterial(UObject* Outer, UMaterialInterface* BaseMaterial, UTexture2D* Texture, const TCHAR* FallbackPath = DeceiveMaskedSpriteMaterialPath);
