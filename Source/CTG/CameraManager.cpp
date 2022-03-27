// None


#include "CameraManager.h"

// Sets default values for this component's properties
UCameraManager::UCameraManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCameraManager::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UCameraManager::SetSpringArmComponent(class USpringArmComponent* SpringArmComponentToSet)
{
	SpringArmComponent = SpringArmComponentToSet;
	ConstructValues();
}


// Called every frame
void UCameraManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCameraManager::ConstructValues()
{
	if (!SpringArmComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't ConstructValues, no SpringArmComponent set"))
		return;
	}

	OriginalPosition = SpringArmComponent->GetRelativeLocation();
	LowPosition = SpringArmComponent->GetRelativeLocation() + LowPositionOffset;
}

void UCameraManager::SetPosition(const EPosition Position)
{
	UE_LOG(LogTemp, Warning, TEXT("CameraManager::SetPosition"));
	if (!SpringArmComponent)
	{
		return;
	}

	FVector* SelectedPosition = nullptr;
	
	switch(Position)
	{
		case POSITION_Original:
			SelectedPosition = &OriginalPosition;
			break;
		case POSITION_Low:
			SelectedPosition = &LowPosition;
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("Invalid EPosition passed to CameraManager::SetPosition"));
			break;
	}

	if (SelectedPosition)
	{
		SpringArmComponent->SetRelativeLocation(*SelectedPosition);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *SelectedPosition->ToString())
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CameraManager::SetPosition selection failed!"));
	}
}

