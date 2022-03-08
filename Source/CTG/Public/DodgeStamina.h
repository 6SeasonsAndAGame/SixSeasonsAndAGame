// None

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DodgeStamina.generated.h"

USTRUCT(BlueprintType)
struct FScalars
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Idle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Walking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Running;
	
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CTG_API UDodgeStamina : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDodgeStamina();

	void Init(class UPaintballWB_Gameplay** WB_GameplayPtr, bool* bIsRunningPtr, bool* bIsWalkingPtr, bool* bIsDashSlidingPtr);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class UPaintballWB_Gameplay** WB_Gameplay = nullptr;
	
	// Replicated player states
	bool* bIsRunning;
	bool* bIsWalking;
	bool* bIsDashSliding;

	
	// Stamina values
	
	FScalars DecrementScalars = {1.f, .5f, .25f};
	FScalars IncrementScalars = {1.f, .5f, .1f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxStamina = 100.f;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	float Stamina = 100.f;
	
	UPROPERTY(BlueprintReadOnly)
	float Decrement = 101.f;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	float DecrementScalar = 1.f;

	UPROPERTY(BlueprintReadOnly)
	float Increment = 10.f;

	UPROPERTY(BlueprintReadWrite)
	float IncrementScalar = 1.f;

	


	//Replicated methods
	UFUNCTION(Client, Reliable)
    void Client_UpdateStaminaWidget();

	UFUNCTION(BlueprintCallable, Client, Reliable)
    void Client_UpdateDecrementScalarWidget();

	
	//Non-replicated methods
	UFUNCTION(BlueprintCallable)
    void DetermineScalars();

	UFUNCTION(BlueprintCallable)
    void LocalServerOnly_SetStamina(float NewValue);

	UFUNCTION(BlueprintCallable)
    float GetStaminaMinusDecrementByScalar();

	UFUNCTION(BlueprintCallable)
    void ApplyDecrementByScalarAndDetermineDeath(bool bSimulatingForOutput, bool& bDeathOut);	
};
