// None


#include "PaintballCharacter.h"

#include "GeneratedCodeHelpers.h"

// Sets default values
APaintballCharacter::APaintballCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
}

// Called when the game starts or when spawned
void APaintballCharacter::BeginPlay()
{
	Super::BeginPlay();

	
	
	//GetWorld()->GetFirstLocalPlayerFromController();
}

void APaintballCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APaintballCharacter, Cpp_DodgeStamina);
	DOREPLIFETIME(APaintballCharacter, Cpp_DodgeStaminaDecrementScalar);

	DOREPLIFETIME(APaintballCharacter, Cpp_bIsRunning);
	DOREPLIFETIME(APaintballCharacter, Cpp_bIsWalking);
	DOREPLIFETIME(APaintballCharacter, Cpp_bIsDashSliding);
}

// Called every frame
void APaintballCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APaintballCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void APaintballCharacter::Cpp_DodgeStaminaTick()
{
	if (Cpp_WB_Gameplay)
	{
		Cpp_WB_Gameplay->Cpp_UpdateDodgeStaminaDamageBar();
	}

	if (!HasAuthority())
	{
		return;
	}

	Cpp_DetermineDodgeStaminaScalars();

	if (Cpp_DodgeStamina < Cpp_MaxDodgeStamina)
	{
		Cpp_LocalServerOnly_SetDodgeStamina(FMath::Clamp(
			Cpp_DodgeStamina + (Cpp_DodgeStaminaIncrement * Cpp_DodgeStaminaIncrementScalar * GetWorld()->GetDeltaSeconds()),
			0.f,
			Cpp_MaxDodgeStamina
		));
	}
	
}

void APaintballCharacter::Cpp_DetermineDodgeStaminaScalars()
{
	if (Cpp_bIsRunning || Cpp_bIsDashSliding)
	{
		Cpp_DodgeStaminaDecrementScalar = .25f;
		Cpp_DodgeStaminaIncrementScalar = .1f;
		//UE_LOG(LogTemp, Warning, TEXT("Running / Dash Sliding"));
	}
	else
	{
		if (Cpp_bIsWalking)
		{
			Cpp_DodgeStaminaDecrementScalar = .5f;
			Cpp_DodgeStaminaIncrementScalar = .5f;
			//UE_LOG(LogTemp, Warning, TEXT("Walking"));
		}
		else
		{
			Cpp_DodgeStaminaDecrementScalar = 1.f;
			Cpp_DodgeStaminaIncrementScalar = 1.f;
			//UE_LOG(LogTemp, Warning, TEXT("Idle"));
		}
	}

	Cpp_Client_UpdateDodgeStaminaDecrementScalarWidget();
}

void APaintballCharacter::Cpp_LocalServerOnly_SetDodgeStamina(float NewValue)
{
	if (!HasAuthority())
	{
		return;
	}

	Cpp_DodgeStamina = NewValue;

	Cpp_Client_UpdateDodgeStaminaWidget();
}

float APaintballCharacter::Cpp_GetDodgeStaminaMinusDecrementByScalar()
{
	return Cpp_DodgeStamina - (Cpp_DodgeStaminaDecrement * Cpp_DodgeStaminaDecrementScalar);
}

void APaintballCharacter::Cpp_ApplyDodgeStaminaDecrementAndDetermineDeath(bool bSimulatingForOutput, bool& bDeathOut)
{
	Cpp_DetermineDodgeStaminaScalars();

	const float SimulatedDodgeStamina = Cpp_GetDodgeStaminaMinusDecrementByScalar();

	if (!bSimulatingForOutput)
	{
		Cpp_LocalServerOnly_SetDodgeStamina(SimulatedDodgeStamina);
	}

	bDeathOut = (SimulatedDodgeStamina < 0.f);
}

void APaintballCharacter::Cpp_Client_UpdateDodgeStaminaWidget_Implementation()
{
	if (!Cpp_WB_Gameplay)
	{
		return;
	}
	Cpp_WB_Gameplay->Cpp_UpdateDodgeStamina();
}

void APaintballCharacter::Cpp_Client_UpdateDodgeStaminaDecrementScalarWidget_Implementation()
{
	if (!Cpp_WB_Gameplay)
	{
		return;
	}
	
	Cpp_WB_Gameplay->Cpp_UpdateDodgeStaminaDecrementScalar();
}
