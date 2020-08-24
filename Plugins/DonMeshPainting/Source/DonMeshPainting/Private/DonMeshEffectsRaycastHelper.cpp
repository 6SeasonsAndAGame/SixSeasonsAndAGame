// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "DonMeshEffectsRaycastHelper.h"
#include "DonMeshPaintingPrivatePCH.h"

#include "DonMeshPaintingHelper.h"

// Internal Use:
enum class EDonTraceType : uint8
{
	LineTraceSingleByChannel,
	LineTraceMultiByChannel,
	SphereTraceSingleByChannel,
	SphereTraceMultiByChannel,

	LineTraceSingleForObjects,
	LineTraceMultiForObjects,
	SphereTraceSingleForObjects,
	SphereTraceMultiForObjects
};

static bool DonTraceRecursive(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const EDonTraceType TraceType, int32& RecursionCount, const FVector Start, const FVector End, const float Radius, ETraceTypeQuery TraceChannel, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	RecursionCount++;

	bool bHit = false;

	switch (TraceType)
	{
	case EDonTraceType::LineTraceSingleByChannel:
	{
		FHitResult outHit;
		OutHits.Empty();
		bHit = UKismetSystemLibrary::LineTraceSingle(WorldContextObject, Start, End, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, outHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Add(outHit);
		break;
	}

	case EDonTraceType::LineTraceSingleForObjects:
	{
		FHitResult outHit;
		OutHits.Empty();
		bHit = UKismetSystemLibrary::LineTraceSingleForObjects(WorldContextObject, Start, End, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, outHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Add(outHit);
		break;
	}

	case EDonTraceType::LineTraceMultiByChannel:
	{
		TArray<FHitResult> outHits;
		bHit = UKismetSystemLibrary::LineTraceMulti(WorldContextObject, Start, End, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Append(outHits);
		break;
	}	

	case EDonTraceType::LineTraceMultiForObjects:
	{
		TArray<FHitResult> outHits;
		bHit = UKismetSystemLibrary::LineTraceMultiForObjects(WorldContextObject, Start, End, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Append(outHits);
		break;
	}

	case EDonTraceType::SphereTraceSingleByChannel:
	{
		FHitResult outHit;
		OutHits.Empty();
		bHit = UKismetSystemLibrary::SphereTraceSingle(WorldContextObject, Start, End, Radius, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, outHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Add(outHit);
		break;
	}

	case EDonTraceType::SphereTraceSingleForObjects:
	{
		FHitResult outHit;
		OutHits.Empty();
		bHit = UKismetSystemLibrary::SphereTraceSingleForObjects(WorldContextObject, Start, End, Radius, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, outHit, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Add(outHit);
		break;
	}

	case EDonTraceType::SphereTraceMultiByChannel:
	{
		TArray<FHitResult> outHits;
		bHit = UKismetSystemLibrary::SphereTraceMulti(WorldContextObject, Start, End, Radius, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Append(outHits);
		break;
	}

	case EDonTraceType::SphereTraceMultiForObjects:
	{
		TArray<FHitResult> outHits;
		bHit = UKismetSystemLibrary::SphereTraceMultiForObjects(WorldContextObject, Start, End, Radius, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
		OutHits.Append(outHits);
		break;
	}

	default:
		ensureMsgf(false, TEXT("Unexpected trace type!"));
		break;
	}

	if (!bHit || OutHits.Num() == 0 || !OutHits.Last().bBlockingHit)
	{
		return false; // No further action necessary - Line trace found nothing.
	}

	// We hit something... is it a paint portal?
	FHitResult& blockingHit = OutHits.Last();

	bool bHitPaintPortal = UDonMeshPaintingHelper::QueryPaintCollision(blockingHit.GetComponent(), PaintPortalSetup.CollisionTag, blockingHit.ImpactPoint, PaintPortalSetup.MinimumPaintBlobSize, PaintPortalSetup.CollisionInflationScale, PaintPortalSetup.CollisionInflationInAbsoluteUnits, PaintPortalSetup.Layer);
	if (bHitPaintPortal)
	{
		if (RecursionCount > PaintPortalSetup.MaxRecursion)
			return true; // max recursion reached  (this would be unusual, even at the default of 10, unless you have a large number of paint portals stacked up for some reason :))

		// Ideally we'd use bFindInitialOverlap and set it to 'false' for the recursion query, but the K2 APIs do not expose this property to Blueprints and I'd like to avoid rewriting the entire K2 function here
		// For C++ users I'd recommend writing your own version of DonLineTrace based on native Collision APIs in GetWorld()->LineTraceSingleByChannel, etc as you have access to bFindInitialOvelap in there
		auto actorsToIngoreCurrent = ActorsToIgnore;
		actorsToIngoreCurrent.Add(blockingHit.GetActor());

		FVector newStart = blockingHit.ImpactPoint;
		OutHits.RemoveAt(OutHits.Num() - 1); // For consistency and in case a user decides to perform some logic on multi-trace hits

		return DonTraceRecursive(WorldContextObject, PaintPortalSetup, TraceType, RecursionCount, newStart, End, Radius, TraceChannel, ObjectTypes, bTraceComplex, actorsToIngoreCurrent, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
	}
	else
		return true; // Hit an impassable obstacle
}

static bool DonTraceRecursive(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const EDonTraceType TraceType, int32& RecursionCount, const FVector Start, const FVector End, const float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return DonTraceRecursive(WorldContextObject, PaintPortalSetup, TraceType, RecursionCount, Start, End, Radius, TraceChannel, TArray<TEnumAsByte<EObjectTypeQuery> >(), bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

static bool DonTraceRecursive(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const EDonTraceType TraceType, int32& RecursionCount, const FVector Start, const FVector End, const float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	return DonTraceRecursive(WorldContextObject, PaintPortalSetup, TraceType, RecursionCount, Start, End, Radius, ETraceTypeQuery::TraceTypeQuery1, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UDonMeshEffectsRaycastHelper::DonLineTraceSingle(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	int32 recursionCount = 0;

	TArray<FHitResult> outHits;
	bool bRes = DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::LineTraceSingleByChannel, recursionCount, Start, End, 0, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	if (ensure(outHits.Num() > 0))
		OutHit = outHits.Last();

	return bRes;
}

bool UDonMeshEffectsRaycastHelper::DonLineTraceMulti(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	int32 recursionCount = 0;

	return DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::LineTraceMultiByChannel, recursionCount, Start, End, 0, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UDonMeshEffectsRaycastHelper::DonSphereTraceSingle(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	if (PaintPortalSetup.MinimumPaintBlobSize == -1)
		PaintPortalSetup.MinimumPaintBlobSize = Radius;

	int32 recursionCount = 0;

	TArray<FHitResult> outHits;
	bool bRes = DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::SphereTraceSingleByChannel, recursionCount, Start, End, Radius, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	if (ensure(outHits.Num() > 0))
		OutHit = outHits.Last();

	return bRes;
}

bool UDonMeshEffectsRaycastHelper::DonSphereTraceMulti(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	if (PaintPortalSetup.MinimumPaintBlobSize == -1)
		PaintPortalSetup.MinimumPaintBlobSize = Radius;

	int32 recursionCount = 0;

	return DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::SphereTraceMultiByChannel, recursionCount, Start, End, Radius, TraceChannel, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

// ~ For Objects ~

bool UDonMeshEffectsRaycastHelper::DonLineTraceSingleForObjects(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	int32 recursionCount = 0;

	TArray<FHitResult> outHits;
	bool bRes = DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::LineTraceSingleForObjects, recursionCount, Start, End, 0, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	if (ensure(outHits.Num() > 0))
		OutHit = outHits.Last();

	return bRes;
}

bool UDonMeshEffectsRaycastHelper::DonLineTraceMultiForObjects(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	int32 recursionCount = 0;

	return DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::LineTraceMultiForObjects, recursionCount, Start, End, 0, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

bool UDonMeshEffectsRaycastHelper::DonSphereTraceSingleForObjects(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	if (PaintPortalSetup.MinimumPaintBlobSize == -1)
		PaintPortalSetup.MinimumPaintBlobSize = Radius;

	int32 recursionCount = 0;

	TArray<FHitResult> outHits;
	bool bRes = DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::SphereTraceSingleForObjects, recursionCount, Start, End, Radius, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, outHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);

	if (ensure(outHits.Num() > 0))
		OutHit = outHits.Last();

	return bRes;
}

bool UDonMeshEffectsRaycastHelper::DonSphereTraceMultiForObjects(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawTime /*= 5.0f*/)
{
	if (PaintPortalSetup.MinimumPaintBlobSize == -1)
		PaintPortalSetup.MinimumPaintBlobSize = Radius;

	int32 recursionCount = 0;

	return DonTraceRecursive(WorldContextObject, PaintPortalSetup, EDonTraceType::SphereTraceMultiForObjects, recursionCount, Start, End, Radius, ObjectTypes, bTraceComplex, ActorsToIgnore, DrawDebugType, OutHits, bIgnoreSelf, TraceColor, TraceHitColor, DrawTime);
}

//