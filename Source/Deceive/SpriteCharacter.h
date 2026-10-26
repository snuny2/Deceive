#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SpriteCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UTexture2D;

/** A character drawn as a camera-facing textured quad, HD-2D style. */
UCLASS()
class DECEIVE_API ASpriteCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASpriteCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Swap this to change how the character looks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UTexture2D> SpriteTexture;

	/** Optional override; defaults to Paper2D's MaskedUnlitSpriteMaterial. */
	UPROPERTY(EditAnywhere, Category = "Sprite")
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY(EditAnywhere, Category = "Sprite", meta = (ClampMin = "1"))
	float SpriteWidth = 120.f;

	UPROPERTY(EditAnywhere, Category = "Sprite", meta = (ClampMin = "1"))
	float SpriteHeight = 180.f;

	/** Applies SpriteTexture to the quad. Call again after changing it at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Sprite")
	void RefreshSprite();

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Sprite")
	TObjectPtr<UStaticMeshComponent> SpriteQuad;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	void MoveForward(float Value);
	void MoveRight(float Value);
};
