// None


#include "PaintballCharacter.h"

// Sets default values
APaintballCharacter::APaintballCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APaintballCharacter::BeginPlay()
{
	Super::BeginPlay();

	
	
	//GetWorld()->GetFirstLocalPlayerFromController();
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

