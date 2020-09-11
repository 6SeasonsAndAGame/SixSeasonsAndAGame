// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "FPSCharacter.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Scene Component"));

	RootComponent = SceneComponent;

	GunMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Gun Mesh"));
	GunMesh->SetupAttachment(SceneComponent);
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWeapon::Fire()
{
	ServerFire();
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
}

void AWeapon::ServerFire_Implementation()
{
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			FRotator SpawnRotation;
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			FVector SpawnLocation;

			GetOwner()->GetActorEyesViewPoint(SpawnLocation, SpawnRotation);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			// spawn the projectile at the muzzle
			auto ptr = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			if (ptr == nullptr)
				UE_LOG(LogTemp, Warning, TEXT("Projectile could not be spawned"));
			ptr->SetOwner(this);
			ptr->SetInstigator(GetInstigator());
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("world was Null"));
		}
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("ProjectileClass was null"));
	}
}

void AWeapon::EquipToPlayer(AFPSCharacter* NewOwner)
{
	SetOwner(NewOwner);
	SetInstigator(NewOwner);
	OwningPawn = NewOwner;
	if (GunMesh) {
		FAttachmentTransformRules AttachRules(EAttachmentRule::KeepRelative, true);
		GunMesh->AttachToComponent(NewOwner->GetMesh1P(), AttachRules, NewOwner->GetWeaponSocket());
	}
}