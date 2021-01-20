// None

#pragma once

#include "CoreMinimal.h"


#include "DodgeStamina.h"
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
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDodgeStamina* DodgeStamina = nullptr;
	
	UPROPERTY(BlueprintReadWrite)
	class UPaintballWB_Gameplay* Cpp_WB_Gameplay = nullptr;

	
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
};
