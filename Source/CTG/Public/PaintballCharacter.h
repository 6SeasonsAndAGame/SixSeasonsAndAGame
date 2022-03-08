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
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DashSlideTime = 1.2f;

	float TimeTillForceStopDashSlide = 0.f;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Component references
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UCameraManager* CameraManager = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UDodgeStamina* DodgeStamina = nullptr;
	

	// Other references
	UPROPERTY(BlueprintReadWrite)
	class UPaintballWB_Gameplay* Cpp_WB_Gameplay = nullptr;

	
	// Replicated player states
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool Cpp_bIsRunning;
	
	UPROPERTY(Replicated, BlueprintReadWrite)
	bool Cpp_bIsWalking;
	
	UPROPERTY(ReplicatedUsing=OnRep_IsDashSliding, BlueprintReadWrite)
	bool Cpp_bIsDashSliding;


	// Non replicated player states
	UPROPERTY(BlueprintReadWrite)
	bool Cpp_bRunningIsPaused;


	// Default values
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultMovementSpeed = 475.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RunSpeedModifier = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DashSlideSpeedModifier = 1.75f;
	
	// Blueprint Implementable Events
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Blueprint_ForceUncrouch();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Blueprint_OnCrouchPressed();
	
	
	// Input functions
	UFUNCTION()
	void OnJumpPressed();

	UFUNCTION()
	void OnJumpReleased();

	UFUNCTION()
	void OnCrouchPressed();


	// Regular functions
	void TickWithAuthority(float DeltaTime);

	void TryDashSlide();

	UFUNCTION(BlueprintCallable)
	void TryStopDashSlide();

	
	// Replicated functions
	UFUNCTION(Server, Reliable, BlueprintCallable)
    void Server_Walk();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_Run();

	UFUNCTION(Server, Reliable, BlueprintCallable)
    void Server_DashSlide();

	UFUNCTION(Server, Reliable, BlueprintCallable)
    void Server_SetIsDashSlidingFalse();

	UFUNCTION()
    void OnRep_IsDashSliding();
};
