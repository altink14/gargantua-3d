#include "GargSlinger.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GargantuaRules.h"

AGargSlinger::AGargSlinger()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);

	// The character turns to face where it is going rather than where the camera
	// looks. In a game about walking up to things and looking at them, a body
	// that strafes sideways while staring ahead reads as wrong.
	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	Move->MaxWalkSpeed = 520.0f;
	Move->BrakingDecelerationWalking = 2200.0f;
	Move->GroundFriction = 8.0f;

	Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Boom"));
	Boom->SetupAttachment(RootComponent);
	// Far enough back to see the cavern, angled down enough to see the floor the
	// slugs are sitting on.
	Boom->TargetArmLength = 720.0f;
	Boom->SetRelativeRotation(FRotator(-38.0f, 0.0f, 0.0f));
	Boom->bUsePawnControlRotation = true;
	Boom->bDoCollisionTest = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Boom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AGargSlinger::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogGargantua, Display, TEXT("Slinger ready at %s."), *GetActorLocation().ToCompactString());
}

void AGargSlinger::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxis(TEXT("MoveForward"), this, &AGargSlinger::MoveForward);
	Input->BindAxis(TEXT("MoveRight"), this, &AGargSlinger::MoveRight);
	Input->BindAxis(TEXT("Turn"), this, &AGargSlinger::AddControllerYawInput);
	Input->BindAxis(TEXT("LookUp"), this, &AGargSlinger::AddControllerPitchInput);
}

void AGargSlinger::MoveForward(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}
	// Movement is relative to where the camera is pointing, flattened, so that
	// looking down at the floor does not slow you to a crawl.
	const FRotator Facing(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::X), Value);
}

void AGargSlinger::MoveRight(float Value)
{
	if (Controller == nullptr || FMath::IsNearlyZero(Value))
	{
		return;
	}
	const FRotator Facing(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(Facing).GetUnitAxis(EAxis::Y), Value);
}
