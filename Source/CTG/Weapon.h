// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Projectile.h"
#include "Weapon.generated.h"

UCLASS()
class CTG_API AWeapon : public AActor
{
	GENERATED_BODY()

	class AFPSCharacter* OwningPawn;
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

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION()
		void Fire();

	UFUNCTION(server, reliable)
		void ServerFire();

	void EquipToPlayer(class AFPSCharacter* NewOwner);
};
