// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#pragma once
#include "GameFramework/ProjectileMovementComponent.h"

#include "DonSmartProjectileComponent.generated.h"

UCLASS(ClassGroup = DonMeshPainter, meta = (BlueprintSpawnableComponent))
class UDonSmartProjectileComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

public:
	UDonSmartProjectileComponent();

	/** Remember to set this to the "portal name" (collision tag) you used while creating your portal with the Paint Stroke node
	Note:- By default Layer 0 is used in this example component. If you need control over layers you'll need to modify the query invocation accordingly*/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Projectile")
	TArray<FName> PaintPortalNames;

	/** Use this to fine-tune the way your projectile collides with painted portals.
	Eg: A value of 0.33 will make the projectile  _less_ likely to pass through a portal. The ratio is expressed in terms of the projectile's spherical collision size*/
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Projectile")
	float PortalCollisionSafetyNet = 0.33f;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Projectile")
	float PortalTravelDuration = 0.075;

	virtual void PostInitProperties() override;

protected:
	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining) override;	

private:
	float SphereCollisionSize = 10.f;

	FTimerHandle timerHandlePortalTravel;
	void PostPortalTravel();

};

