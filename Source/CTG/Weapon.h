// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Projectile.h"
#include "GameFramework/DamageType.h"
#include "Weapon.generated.h"

UCLASS()
class CTG_API AWeapon : public AActor
{
	GENERATED_BODY()

	class AFPSCharacter* OwningPawn;

	UPROPERTY(EditDefaultsOnly, Category = Weapon)
		FVector MuzzleOffset {
		100.0f, 0.0f, 10.0f
	};

	UPROPERTY(EditDefaultsOnly, Category = Damage)
		float Damage;

public:	
	// Sets default values for this actor's properties
	AWeapon();

protected:
	/** weapon mesh: 1st person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USkeletalMeshComponent* GunMesh;

	/** weapon mesh: 3rd person view */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Projectile)
		TSubclassOf<class AProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		class USoundBase* FireSound;	
	
	//The damage type and damage that will be done by this projectile
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Damage)
		TSubclassOf<UDamageType> DamageType;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void Fire();

	UFUNCTION(Server, Reliable, WithValidation)
		void ServerFire();

	void EquipToPlayer(class AFPSCharacter* NewOwner);

	float GetDamage() { return Damage; }

	void SetOwningPawn(class AFPSCharacter* NewOwner) { OwningPawn = NewOwner; }

	AFPSCharacter* GetOwningPawn() { return OwningPawn; }

	TSubclassOf<UDamageType> GetDamageType() { return DamageType; }
};
