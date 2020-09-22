// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Weapon.h"
#include "FPSCharacter.generated.h"

UCLASS()
class AFPSCharacter : public ACharacter
{
	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere,ReplicatedUsing= OnRep_Weapon , Category = Weapon)
	class AWeapon* Weapon;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		FName WeaponSocket;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		TSubclassOf<class AWeapon> WeaponClass;

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		class USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FirstPersonCameraComponent;

public:
	// Sets default values for this character's properties
	AFPSCharacter();
protected:

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		class UAnimMontage* FireAnimation;

	UPROPERTY(EditDefaultsOnly, Category = Health)
		float MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = Health)
		float Health;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PostInitializeComponents() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnUpdateHealth();

	UFUNCTION()
		void OnRep_Weapon();

	UFUNCTION()
		void OnUpdateWeapon();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void MoveForward(float Axis);
	void MoveRight(float Axis);

	UFUNCTION()
	void Fire();

	UFUNCTION(Server, Reliable)
		void ServerFire();

	void StartCrouching();
	void StopCrouching();

	UFUNCTION()
	void EquipWeapon(AWeapon* _Weapon);

	FName GetWeaponSocket();

	USkeletalMeshComponent* GetMesh1P() { return Mesh1P; }
	
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, class AActor* DamageCauser);
};
