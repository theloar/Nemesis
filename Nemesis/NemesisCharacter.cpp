// Copyright Epic Games, Inc. All Rights Reserved.

#include "NemesisCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

#include "NemesisAnimInstance.h"
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>

//////////////////////////////////////////////////////////////////////////
// ANemesisCharacter

ANemesisCharacter::ANemesisCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)


	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshContainer(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Skins/GlacialEmpress/Meshes/Aurora_GlacialEmpress"));
	
	if (MeshContainer.Object)
		GetMesh()->SetSkeletalMesh(MeshContainer.Object);

	static ConstructorHelpers::FObjectFinder<UAnimBlueprint> AnimBP(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Aurora_AnimBP"));

	if (AnimBP.Object)
		GetMesh()->SetAnimInstanceClass(AnimBP.Object->GeneratedClass);

	//GetMesh()->SetAnimInstanceClass(UNemesisAnimInstance::StaticClass());

	FVector location = GetMesh()->GetRelativeLocation();
	location.Z -= 97;
	GetMesh()->SetRelativeLocation(location);

	FRotator rotation = GetMesh()->GetRelativeRotation();
	FRotator rotated(rotation.Pitch, rotation.Yaw + 270, rotation.Roll);
	GetMesh()->SetRelativeRotation(rotated);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackA(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_A_Montage"));
	PrimaryAttackA = AttackA.Object;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackB(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_B_Montage"));
	PrimaryAttackB = AttackB.Object;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackC(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Primary_Attack_C_Montage"));
	PrimaryAttackC = AttackC.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> AttackCueObj(TEXT("/Game/ParagonAurora/Audio/Wavs/Aurora_Ability_LMB_Engage_030"));
	Attack1 = AttackCueObj.Object;

	//static ConstructorHelpers::FObjectFinder<UAnimMontage> MoveFWDAnim(TEXT("/Game/ParagonAurora/Characters/Heroes/Aurora/Animations/Jog_Fwd"));
	//MoveForwardAnim = MoveFWDAnim.Object;
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANemesisCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Attack", IE_Pressed, this, &ANemesisCharacter::Attack);

	PlayerInputComponent->BindAxis("MoveForward", this, &ANemesisCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ANemesisCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ANemesisCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ANemesisCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ANemesisCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ANemesisCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ANemesisCharacter::OnResetVR);
}


void ANemesisCharacter::OnResetVR()
{
	// If Nemesis is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in Nemesis.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ANemesisCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ANemesisCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ANemesisCharacter::Attack()
{
	//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::White, TEXT("Attacking!"));

	if (IsAttacking)
		SaveAttack = true;
	else {
		IsAttacking = true;
		PlayAttackAnimation();
	}
		
	//UGameplayStatics::SpawnSoundAtLocation(this, Attack1, GetActorLocation());
}

void ANemesisCharacter::PlayAttackAnimation()
{
	switch (AttackCount)
	{
	case 0:
		AttackCount = 1;
		PlayAnimMontage(PrimaryAttackA);
		break;
	case 1:
		AttackCount = 2;
		PlayAnimMontage(PrimaryAttackB);
		break;
	case 2:
		AttackCount = 3;
		PlayAnimMontage(PrimaryAttackC);
		break;
	}
}

void ANemesisCharacter::ComboSaveAttack()
{
	if (SaveAttack) {
		SaveAttack = false;
		PlayAttackAnimation();
	}
}

void ANemesisCharacter::ResetCombo()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::White, TEXT("Reset!"));
	AttackCount = 0;
	SaveAttack = false;
	IsAttacking = false;
}

void ANemesisCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANemesisCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ANemesisCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);

		PlayAnimMontage(MoveForwardAnim);
	}
}

void ANemesisCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}