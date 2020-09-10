// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "FPSCharacter.h"

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
}

void AWeapon::ServerFire_Implementation()
{
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