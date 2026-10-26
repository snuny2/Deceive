#include "DeceiveSpriteMaterial.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"

const FName DeceiveSpriteTextureParam(TEXT("SpriteTexture"));
const TCHAR* DeceiveMaskedSpriteMaterialPath = TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");
const TCHAR* DeceiveOpaqueSpriteMaterialPath = TEXT("/Paper2D/OpaqueUnlitSpriteMaterial.OpaqueUnlitSpriteMaterial");

UMaterialInstanceDynamic* MakeTexturedMaterial(UObject* Outer, UMaterialInterface* BaseMaterial, UTexture2D* Texture, const TCHAR* FallbackPath)
{
	if (!BaseMaterial)
	{
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, FallbackPath);
	}

	if (!BaseMaterial)
	{
		return nullptr;
	}

	if (!Texture)
	{
		// Without this the Paper2D placeholder (a soft circle) shows instead, which
		// makes the untextured prototype unreadable.
		Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	}

	UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(BaseMaterial, Outer);
	if (Dynamic && Texture)
	{
		Dynamic->SetTextureParameterValue(DeceiveSpriteTextureParam, Texture);
	}

	return Dynamic;
}
