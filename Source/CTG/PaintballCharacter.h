// None

#pragma once

#include "CoreMinimal.h"

#include "PaintballWB_Gameplay.h"
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
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
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

	UPROPERTY(BlueprintReadWrite)
	class UPaintballWB_Gameplay* Cpp_WB_Gameplay;

	
	// Replicated player states
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool Cpp_bIsRunning;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool Cpp_bIsWalking;

	UPROPERTY(Replicated, BlueprintReadWrite)
	bool Cpp_bIsDashSliding;


	// Non replicated player states
	UPROPERTY(BlueprintReadWrite)
	bool Cpp_bRunningIsPaused;

	
	// Stamina values
	UPROPERTY(Replicated, BlueprintReadWrite)
	float Cpp_DodgeStamina = 100.f;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStaminaDecrement = 101.f;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	float Cpp_DodgeStaminaDecrementScalar = 1.f;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_DodgeStaminaIncrement = 10.f;

	UPROPERTY(BlueprintReadWrite)
	float Cpp_DodgeStaminaIncrementScalar = 1.f;

	UPROPERTY(BlueprintReadOnly)
	float Cpp_MaxDodgeStamina = 100.f;


	//Replicated methods
	UFUNCTION(Client, Reliable)
	void Cpp_Client_UpdateDodgeStaminaWidget();

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void Cpp_Client_UpdateDodgeStaminaDecrementScalarWidget();

	
	//Non-replicated methods
	UFUNCTION(BlueprintCallable)
	void Cpp_DodgeStaminaTick();
	
	UFUNCTION(BlueprintCallable)
	void Cpp_DetermineDodgeStaminaScalars();

	UFUNCTION(BlueprintCallable)
	void Cpp_LocalServerOnly_SetDodgeStamina(float NewValue);

	UFUNCTION(BlueprintCallable)
	float Cpp_GetDodgeStaminaMinusDecrementByScalar();

	UFUNCTION(BlueprintCallable)
	void Cpp_ApplyDodgeStaminaDecrementAndDetermineDeath(bool bSimulatingForOutput, bool& bDeathOut);
};
