// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "FPSCharacter.h"
#include "Weapon.h"
#include "Kismet/GameplayStatics.h"
#include "TDPlayerState.h"

// Sets default values
AProjectile::AProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(15.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	ProjectileSpeed = 12000.0f;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
	SetReplicates(true);
	SetReplicateMovement(true);
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	auto Character = Cast<AFPSCharacter>(OtherActor);


	if (Character != nullptr && Character != GetOwner())
	{
		int OtherTeam = Character->GetPlayerState<ATDPlayerState>()->Team;
		int MyTeam = Cast<AWeapon>(GetOwner())->GetOwningPawn()->GetPlayerState<ATDPlayerState>()->Team;
		if (MyTeam != OtherTeam) {
			if (!HasAuthority()) {
				ServerOnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
			}
			else {
				UGameplayStatics::ApplyPointDamage(Character, Cast<AWeapon>(GetOwner())->GetDamage(), NormalImpulse, Hit, GetInstigator()->GetController(), this, Cast<AWeapon>(GetOwner())->GetDamageType());
			}
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("Players are on the same team"))
		}
	}
	if (OtherActor != GetOwner() && OtherActor != GetInstigator()) {
		Destroy();
	}
	UE_LOG(LogTemp, Warning, TEXT("Projectile collided with Actor %s"), *OtherActor->GetFName().ToString());
}

void AProjectile::ServerOnHit_Implementation(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	auto Character = Cast<AFPSCharacter>(OtherActor);
	UGameplayStatics::ApplyPointDamage(Character, Cast<AWeapon>(GetOwner())->GetDamage(), NormalImpulse, Hit, GetInstigator()->GetController(), this, Cast<AWeapon>(GetOwner())->GetDamageType());
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	CollisionComp->MoveIgnoreActors.Add(GetInstigator());
	CollisionComp->MoveIgnoreActors.Add(GetOwner());
}

// Called every frame
void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

