// None

#pragma once

#include "CoreMinimal.h"

#include "Camera/CameraComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "CameraManager.generated.h"

UENUM(BlueprintType)
enum EPosition
{
	POSITION_Original UMETA(DisplayName = "Original"),
	POSITION_Low UMETA(DisplayName = "Low")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CTG_API UCameraManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraManager();

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USpringArmComponent* SpringArmComponent = nullptr;

	UPROPERTY(EditAnywhere)
	FVector OriginalPosition;

	UPROPERTY(EditAnywhere)
	FVector LowPosition;

	UPROPERTY(EditAnywhere)
	FVector LowPositionOffset = FVector(0.f, 0.f, -75.f);


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	void ConstructValues();
	
	void SetPosition(EPosition Position);
};
