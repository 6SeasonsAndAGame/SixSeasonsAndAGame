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
}

// Called every frame
void APaintballCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Cpp_WB_Gameplay)
	{
		
	}
}

// Called to bind functionality to input
void APaintballCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void APaintballCharacter::Cpp_Client_UpdateDodgeStamina_Implementation()
{
	if (Cpp_WB_Gameplay)
	{
		// CPP_WB_Gameplay->UpdateDodgeStamina();
	}
}

