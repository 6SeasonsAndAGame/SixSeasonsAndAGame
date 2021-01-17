// None

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PaintballCharacter.generated.h"

UCLASS()
class CTG_API APaintballCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APaintballCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//void DodgeStaminaTick();

	// should only be called on the server
	//void LocalServerOnly_SetDodgeStamina(float NewValue);

	// executes on owning client
	//void Client_UpdateDodgeStamina();

	// executes on owning client
	//void Client_UpdateDodgeStaminaDecrementScalar();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStamina;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStaminaDecrement;
	
	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStaminaDecrementScalar;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStaminaIncrement;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStaminaIncrementScalar;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_MaxDodgeStamina;
	
};
