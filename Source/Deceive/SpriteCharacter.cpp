#include "SpriteCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DeceiveSpriteMaterial.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float PlaneMeshSize = 100.f;

	/**
	 * Engine's plane mesh lies in XY with its normal on +Z. This rotation stands it
	 * up so the normal faces -X, the texture's U axis runs along +Y and its V axis
	 * runs down. Yaw is the outermost term, so adding to it keeps that orientation
	 * while turning the quad towards the camera.
	 */
	const FRotator QuadBaseRotation(0.f, 90.f, -90.f);
	constexpr float QuadBaseNormalYaw = 180.f;
}

ASpriteCharacter::ASpriteCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = 350.f;

	SpriteQuad = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SpriteQuad"));
	SpriteQuad->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	SpriteQuad->SetStaticMesh(PlaneMeshFinder.Object);
	SpriteQuad->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpriteQuad->SetRelativeRotation(QuadBaseRotation);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = 2700.f;
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->SetWorldRotation(FRotator(-25.f, 0.f, 0.f));

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->FieldOfView = 40.f;
}

void ASpriteCharacter::BeginPlay()
{
	Super::BeginPlay();
	RefreshSprite();
}

void ASpriteCharacter::RefreshSprite()
{
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SpriteQuad->SetRelativeLocation(FVector(0.f, 0.f, SpriteHeight * 0.5f - HalfHeight));
	SpriteQuad->SetRelativeScale3D(FVector(SpriteWidth / PlaneMeshSize, SpriteHeight / PlaneMeshSize, 1.f));

	if (UMaterialInstanceDynamic* Material = MakeTexturedMaterial(this, BaseMaterial, SpriteTexture))
	{
		SpriteQuad->SetMaterial(0, Material);
	}
}

void ASpriteCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Yaw-only billboarding: turn the quad's normal towards the active camera.
	const FVector ViewLocation = Camera->GetComponentLocation();
	const FVector ToView = ViewLocation - SpriteQuad->GetComponentLocation();
	if (ToView.SizeSquared2D() > KINDA_SMALL_NUMBER)
	{
		const float TargetNormalYaw = FMath::RadiansToDegrees(FMath::Atan2(ToView.Y, ToView.X));
		SpriteQuad->SetWorldRotation(FRotator(
			QuadBaseRotation.Pitch,
			QuadBaseRotation.Yaw + TargetNormalYaw - QuadBaseNormalYaw,
			QuadBaseRotation.Roll));
	}
}

void ASpriteCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ASpriteCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ASpriteCharacter::MoveRight);
}

void ASpriteCharacter::MoveForward(float Value)
{
	// The camera yaw is fixed at 0, so screen-up is world +X.
	AddMovementInput(FVector::ForwardVector, Value);
}

void ASpriteCharacter::MoveRight(float Value)
{
	AddMovementInput(FVector::RightVector, Value);
}
