// None


#include "PaintballCharacter.h"

#include "GeneratedCodeHelpers.h"
#include "UObject/Script.h"

// Sets default values
APaintballCharacter::APaintballCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SetReplicates(true);
	
	DodgeStamina = CreateDefaultSubobject<UDodgeStamina>(FName("DodgeStamina"));
	AddOwnedComponent(DodgeStamina);
	DodgeStamina->SetIsReplicated(true);
	DodgeStamina->Init(&Cpp_WB_Gameplay, &Cpp_bIsRunning, &Cpp_bIsWalking, &Cpp_bIsDashSliding);
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
	
}

// Called to bind functionality to input
void APaintballCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}











