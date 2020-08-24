// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "PaintBlobExamples/DonSmartProjectileComponent.h"
#include "../DonMeshPaintingPrivatePCH.h"

#include "Runtime/Engine/Classes/Engine/StaticMeshActor.h"
#include "DonMeshPaintingHelper.h"

UDonSmartProjectileComponent::UDonSmartProjectileComponent() 
{
	PaintPortalNames.Add(FName("PaintPortal")); // simple default, mainly for the sample project
}

void UDonSmartProjectileComponent::PostInitProperties()
{
	Super::PostInitProperties();

	auto owner = GetOwner();
	auto root = owner ? owner->GetRootComponent() : nullptr;
	if (!root)
		return;

	auto sphereCollision = Cast<USphereComponent>(root);
	if (sphereCollision)
		SphereCollisionSize = sphereCollision->GetScaledSphereRadius();
	else
		UE_LOG(LogDonMeshPainting, Warning, TEXT("Unable to find sphere collision root for smart projectile. Projectile collision may not be accurate!"));
	
}

UProjectileMovementComponent::EHandleBlockingHitResult UDonSmartProjectileComponent::HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	const float minimumPaintBlobSize = SphereCollisionSize;
	const float CollisionInflationInAbsoluteUnits = -1 * SphereCollisionSize * PortalCollisionSafetyNet; // negative, because it's a safety net. We're literally shrinking the available portal size
	const float portalCollisionInflationScale = 1.f; // this is typically managed by the entity painting the portal, the projectile itself should leave it alone!
	
	// Prepare the query:		
	TArray<FDonPaintCollisionQuery> queries;
	FDonPaintCollisionQuery outQuery;
	const int32 layer = 0; // hardocded for this sample component; please parametrize as necessary for your usecases
	for(const FName& portalName : PaintPortalNames)
		queries.Add(FDonPaintCollisionQuery(layer, portalName, minimumPaintBlobSize, 1.f, CollisionInflationInAbsoluteUnits));

	// Check for paint collisions:
	bool bPaintHit = UDonMeshPaintingHelper::QueryPaintCollisionMulti(Hit.GetComponent(), queries, outQuery, Hit.Location);

	// ~~~ Portal-travel ! ~~~
	if (bPaintHit)
	{
		GetOwner()->SetActorEnableCollision(false);
		GetWorld()->GetTimerManager().SetTimer(timerHandlePortalTravel, this, &UDonSmartProjectileComponent::PostPortalTravel, PortalTravelDuration, false);

		return EHandleBlockingHitResult::AdvanceNextSubstep;
	}
	else
		return EHandleBlockingHitResult::Deflect;
}

void UDonSmartProjectileComponent::PostPortalTravel()
{
	GetOwner()->SetActorEnableCollision(true);
}
