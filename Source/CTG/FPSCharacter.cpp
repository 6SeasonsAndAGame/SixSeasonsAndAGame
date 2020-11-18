// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "FPSPlayerController.h"
#include "TDGameMode.h"
#include "Weapon.h"
#include "Blueprint/UserWidget.h"


// Sets default values
AFPSCharacter::AFPSCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(48.f, 87.0f);
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	WeaponSocket = FName(TEXT("GripPoint"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	MaxHealth = 1.0f;
	Health = MaxHealth;
}

// Called when the game starts or when spawned
void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AFPSCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	GetWorldTimerManager().SetTimerForNextTick([this]() {
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Weapon = GetWorld()->SpawnActor<AWeapon>(WeaponClass, SpawnParams);
		if (Weapon) {
			Weapon->EquipToPlayer(this);
		}
	});
}

void AFPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSCharacter, Health);
	DOREPLIFETIME(AFPSCharacter, Weapon);
}

void AFPSCharacter::OnRep_Health()
{
	OnUpdateHealth();
}

void AFPSCharacter::OnUpdateHealth()
{
	if (Health <= 0) {
		GetMesh()->SetSimulatePhysics(true);
		// Also display death screen on respawn option
	}
}

void AFPSCharacter::OnRep_Weapon()
{
	OnUpdateWeapon();
}

void AFPSCharacter::OnUpdateWeapon()
{
	Weapon->SetOwningPawn(this);

	Weapon->EquipToPlayer(this);
}

// Called every frame
void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (StunnedSec > 0) {
		StunnedSec -= DeltaTime;
	}
	if (StunDelaySec > 0) {
		StunDelaySec -= DeltaTime;
	}
}

// Called to bind functionality to input
void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AFPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AFPSCharacter::MoveRight);

	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AFPSCharacter::StartCrouching);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &AFPSCharacter::StopCrouching);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AFPSCharacter::Fire);
}

void AFPSCharacter::MoveForward(float Axis)
{
	if (StunnedSec <= 0) {
		AddMovementInput(GetActorForwardVector(), Axis);
	}
}

void AFPSCharacter::MoveRight(float Axis)
{
	if (StunnedSec <= 0) {
		AddMovementInput(GetActorRightVector(), Axis);
	}
}

void AFPSCharacter::Fire()
{
	if (StunnedSec > 0) {
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::Fire() called"))
	
	if (Weapon) {
		Weapon->Fire();
	}
	
	if (FireAnimation)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.0f);
		}
	}
	
}

void AFPSCharacter::ServerFire_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("AFPSCharacter::ServerFire() called"))
	if (Weapon) {
		Weapon->Fire();
	}
}

void AFPSCharacter::StartCrouching()
{
	Crouch();
}

void AFPSCharacter::StopCrouching()
{
	UnCrouch();
}

void AFPSCharacter::EquipWeapon(AWeapon* _Weapon)
{
	if (_Weapon) {
		Weapon = _Weapon;
		Weapon->EquipToPlayer(this);
	}
}

FName AFPSCharacter::GetWeaponSocket()
{
	return WeaponSocket;
}

void AFPSCharacter::Stun() {
	StunnedSec = 1;
	StunDelaySec = 5;
}

float AFPSCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (StunDelaySec <= 0){
		Stun();
	}

	float StartingHealth = Health;
	Health -= DamageAmount;
	Health = FMath::Clamp(Health, 0.0f, MaxHealth);
	if (Health == 0) {
		if (EventInstigator != nullptr && GetController() != nullptr) {
			GetWorld()->GetAuthGameMode<ATDGameMode>()->OnPlayerEliminated(EventInstigator, GetController());
		} else {
			UE_LOG(LogTemp, Warning, TEXT("Controller was nullptr"));
		}
	}
	OnUpdateHealth();
	return StartingHealth - Health;
}

