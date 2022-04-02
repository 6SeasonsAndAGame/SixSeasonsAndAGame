// None


#include "CTG/Public/DodgeStamina.h"
#include "GeneratedCodeHelpers.h"
#include "CTG/Public/PaintballWB_Gameplay.h"


// Sets default values for this component's properties
UDodgeStamina::UDodgeStamina()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	
}

void UDodgeStamina::Init(class UPaintballWB_Gameplay** WB_GameplayPtr, bool* bIsRunningPtr, bool* bIsWalkingPtr, bool* bIsDashSlidingPtr)
{
	WB_Gameplay = WB_GameplayPtr;
	
	bIsRunning = bIsRunningPtr;
	bIsWalking = bIsWalkingPtr;
	bIsDashSliding = bIsDashSlidingPtr;
}


// Called when the game starts
void UDodgeStamina::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UDodgeStamina::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UDodgeStamina, Stamina);
	DOREPLIFETIME(UDodgeStamina, DecrementScalar);
}


// Called every frame
void UDodgeStamina::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (*WB_Gameplay)
	{
		(*WB_Gameplay)->Cpp_UpdateDodgeStaminaDamageBar();
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	DetermineScalars();

	if (Stamina < MaxStamina)
	{
		LocalServerOnly_SetStamina(FMath::Clamp(
            Stamina + (Increment * IncrementScalar * GetWorld()->GetDeltaSeconds()),
            0.f,
            MaxStamina
        ));
	}
}

void UDodgeStamina::DetermineScalars()
{
	// Use of FScalars struct is disabled, causes issues

	if (*bIsRunning || *bIsDashSliding)
	{
		DecrementScalar = .25f; //DecrementScalars.Running;
		IncrementScalar = .1f; //IncrementScalars.Running;
		//UE_LOG(LogTemp, Warning, TEXT("%f"), IncrementScalar);
	}
	else
	{
		if (*bIsWalking)
		{
			DecrementScalar = .5f; //DecrementScalars.Walking;
			IncrementScalar = .5f; //IncrementScalars.Walking;
		}
		else
		{
			DecrementScalar = 1.f; //DecrementScalars.Idle;
			IncrementScalar = 1.f; //IncrementScalars.Idle;
		}
	}

	Client_UpdateDecrementScalarWidget();
}

void UDodgeStamina::LocalServerOnly_SetStamina(float NewValue)
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	Stamina = NewValue;

	Client_UpdateStaminaWidget();
}

float UDodgeStamina::GetStaminaMinusDecrementByScalar()
{
	return Stamina - (Decrement * DecrementScalar);
}

void UDodgeStamina::ApplyDecrementByScalarAndDetermineDeath(bool bSimulatingForOutput, bool& bDeathOut)
{
	UE_LOG(LogTemp, Warning, TEXT("ApplyDecrementByScalarAndDetermineDeath"));
	DetermineScalars();
	
	const float SimulatedDodgeStamina = GetStaminaMinusDecrementByScalar();

	UE_LOG(LogTemp, Warning, TEXT("%f"), SimulatedDodgeStamina);
	
	if (!bSimulatingForOutput)
	{
		LocalServerOnly_SetStamina(SimulatedDodgeStamina);
	}

	bDeathOut = (SimulatedDodgeStamina <= 0.f);
}

void UDodgeStamina::Client_UpdateStaminaWidget_Implementation()
{
	if (!*WB_Gameplay)
	{
		return;
	}
	(*WB_Gameplay)->Cpp_UpdateDodgeStamina(Stamina, MaxStamina);
}

void UDodgeStamina::Client_UpdateDecrementScalarWidget_Implementation()
{
	if (!*WB_Gameplay)
	{
		UE_LOG(LogTemp, Warning, TEXT("INVALID WB_GAMEPLAY"));
		return;
	}
	
	(*WB_Gameplay)->Cpp_UpdateDodgeStaminaDecrementScalar(DecrementScalar);
}

