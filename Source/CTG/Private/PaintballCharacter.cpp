// None


#include "PaintballCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PaintballWB_Gameplay.h"
#include "GeneratedCodeHelpers.h"
#include "CameraManager.h"
#include "DodgeStamina.h"

// Sets default values
APaintballCharacter::APaintballCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);

	CameraManager = CreateDefaultSubobject<UCameraManager>(FName("CameraManager"));
	AddOwnedComponent(CameraManager);
	
	DodgeStamina = CreateDefaultSubobject<UDodgeStamina>(FName("DodgeStamina"));
	AddOwnedComponent(DodgeStamina);
	DodgeStamina->SetIsReplicated(true);
	DodgeStamina->Init(&Cpp_WB_Gameplay, &Cpp_bIsRunning, &Cpp_bIsWalking, &Cpp_bIsDashSliding);
}

// Called in-editor whenever any of the actor's properties have been changed
void APaintballCharacter::OnConstruction(const FTransform& Transform)
{
	
}

// Called when the game starts or when spawned
void APaintballCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void APaintballCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaintballCharacter, Cpp_bIsRunning);
	DOREPLIFETIME(APaintballCharacter, Cpp_bIsWalking);
	DOREPLIFETIME(APaintballCharacter, Cpp_bIsDashSliding);
}

// Called every frame
void APaintballCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!Cpp_WB_Gameplay)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Original WB Gameplay invalid"));
	}

	if (HasAuthority())
	{
		TickWithAuthority(DeltaTime);
	}
}

void APaintballCharacter::TickWithAuthority(float DeltaTime)
{
	TimeTillForceStopDashSlide = FMath::Clamp(TimeTillForceStopDashSlide - DeltaTime, 0.f, DashSlideTime); // Decrease TimeTillForceStopDashSlide by time passed every frame
	
	if (FMath::IsNearlyEqual(TimeTillForceStopDashSlide, 0.f)) // Use IsNearlyEqual to account for floating point errors
	{
		TryStopDashSlide();
	}
}

// Called to bind functionality to input
void APaintballCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	
	PlayerInputComponent->BindAction(FName("Jump"), IE_Pressed, this, &APaintballCharacter::OnJumpPressed);
	PlayerInputComponent->BindAction(FName("Jump"), IE_Released, this, &APaintballCharacter::OnJumpPressed);
	
	PlayerInputComponent->BindAction(FName("Crouch"), IE_Pressed, this, &APaintballCharacter::OnCrouchPressed);
}

void APaintballCharacter::OnRep_IsDashSliding()
{
	if (!Cpp_bIsDashSliding)
	{
		CameraManager->SetPosition(POSITION_Original);
	}
}


/* Input functions
 * Called on Key down and up
 * This allows for automatically jumping after crouching/sliding */
void APaintballCharacter::OnJumpPressed()
{
	if (Cpp_bIsDashSliding)
	{
		TryStopDashSlide();
	}

	if (bIsCrouched)
	{
		Blueprint_ForceUncrouch();
	}
	else
	{
		Jump();
	}

	
}

void APaintballCharacter::OnJumpReleased()
{
	StopJumping();
}

void APaintballCharacter::OnCrouchPressed()
{
	//UE_LOG(LogTemp, Warning, TEXT("OnCrouchPressed"));
	TryDashSlide();

	Blueprint_OnCrouchPressed();
}

// May be called from the server or the client
void APaintballCharacter::TryDashSlide()
{
	if (Cpp_bIsRunning && !Cpp_bIsDashSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		CameraManager->SetPosition(POSITION_Low);
		Server_DashSlide();
	}
}

// May be called from the server or the client
void APaintballCharacter::TryStopDashSlide()
{
	if (Cpp_bIsDashSliding)
	{
		// To-do: Add Relative Camera Position
		CameraManager->SetPosition(POSITION_Original);
		Server_Walk();
		Server_SetIsDashSlidingFalse(); // Setting replicated variable via server function, in case TryStopDashSlide is called from the client
	}
}


void APaintballCharacter::Server_SetIsDashSlidingFalse_Implementation()
{
	Cpp_bIsDashSliding = false;
}

void APaintballCharacter::Server_DashSlide_Implementation()
{
	// To-do: Add Relative Camera Position
	TimeTillForceStopDashSlide = DashSlideTime;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed * DashSlideSpeedModifier;
	Cpp_bIsDashSliding = true;
}

void APaintballCharacter::Server_Run_Implementation()
{
	if (Cpp_bIsDashSliding)
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed * RunSpeedModifier;
	Cpp_bIsRunning = true;
}

// Replicated functions
void APaintballCharacter::Server_Walk_Implementation()
{
	GetCharacterMovement()->MaxWalkSpeed = DefaultMovementSpeed;
	Cpp_bIsRunning = false;
}
