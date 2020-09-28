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

	Damage = 1.0f;
	bReplicates = true;
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
	UE_LOG(LogTemp, Warning, TEXT("AWeapon::Fire() called"))
	//if (!HasAuthority())
	//{
		ServerFire();
		if (FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}
	//}
}

bool AWeapon::ServerFire_Validate() {
	return true;
}

void AWeapon::ServerFire_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("AWeapon::ServerFire() called"))
	// try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			FRotator SpawnRotation;
			// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
			FVector SpawnLocation;

			OwningPawn->GetActorEyesViewPoint(SpawnLocation, SpawnRotation);

			//Set Spawn Collision Handling Override
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			ActorSpawnParams.Owner = this;
			ActorSpawnParams.Instigator = OwningPawn;

			// spawn the projectile at the muzzle
			auto ptr = World->SpawnActor<AProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			if (ptr == nullptr) {
				UE_LOG(LogTemp, Warning, TEXT("Projectile could not be spawned"));
			}
		}
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
