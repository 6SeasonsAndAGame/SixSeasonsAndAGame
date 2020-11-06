// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "DonMeshPaintingHelper.h"
#include "DonMeshPaintingPrivatePCH.h"
#include "DonMeshPainterGlobalActor.h"

#include "DonMeshPainterComponent.h"
#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Engine/Classes/Kismet/KismetRenderingLibrary.h"
#include "Runtime/Engine/Public/ImageUtils.h"
#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"

#include "Kismet/KismetRenderingLibrary.h"
#include "Runtime/RHI/Public/RHICommandList.h"

bool UDonMeshPaintingHelper::PaintStroke_Internal(const FHitResult& Hit, FDonPaintStroke& Stroke, const bool bAllowDuplicateStrokes, bool bAllowContextFreePainting, const bool bPerformNetworkReplication /*= true*/)
{
	auto globalActor = UDonMeshPaintingHelper::GetGlobalManager(Hit.GetActor());
	if (!globalActor)
		return false;

	UDonMeshPainterComponent* meshPainter = globalActor->GetMeshPainterComponent(Hit.GetActor());
	if (!meshPainter)
		return false;

	// If no render brush was specified, use the default smart material that handles all conceivable UV painting usecases out-of-the-box
	if (Stroke.StrokeParams.BrushRenderMaterial == nullptr)
	{
		if (Stroke.bEraseMode)
			Stroke.StrokeParams.BrushRenderMaterial = globalActor->EraserBrushRenderMaterial;
		else
			Stroke.StrokeParams.BrushRenderMaterial = globalActor->DefaultBrushRenderMaterial;
	}
	
	bool bStatus = meshPainter->PaintStroke(Hit, Stroke, bAllowDuplicateStrokes, bAllowContextFreePainting, bPerformNetworkReplication);

	return bStatus;
}

bool UDonMeshPaintingHelper::PaintStroke(const FHitResult& Hit, float BrushSize /*= 10.f*/, FLinearColor BrushColor /*= FLinearColor::White*/, UTexture2D* BrushDecalTexture /*= nullptr*/, int32 PaintLayer /*= 0*/, const bool bEraseMode/* = false*/, const FName CollisionTag /*= NAME_None*/, const float BrushOpacity /*= 1.f*/, float BrushHardness /*= 0.1f*/, const int32 BrushDecalRotation /*= 0*/, UMaterialInterface* BrushRenderMaterial /*= nullptr*/, const bool bAllowDuplicateStrokes /*= true*/, const float CollisionInflationScale /*= 1.f*/)
{
	const bool bAllowContextFreePainting = false;
	const auto& strokeParams = FDonPaintStrokeParameters(PaintLayer, BrushDecalTexture, BrushRenderMaterial, BrushOpacity, BrushHardness, CollisionTag, CollisionInflationScale);
	FDonPaintStroke stroke = FDonPaintStroke(Hit.ImpactPoint, BrushSize, BrushDecalRotation, BrushColor, Hit.FaceIndex, strokeParams, FText(), FDonPaintableTextStyle(), bEraseMode);

	return PaintStroke_Internal(Hit, stroke, bAllowDuplicateStrokes, bAllowContextFreePainting);
}

bool UDonMeshPaintingHelper::PaintText(const FHitResult& Hit, const FText Text, const FDonPaintableTextStyle& Style, const FLinearColor Color /*= FLinearColor::White*/, const int32 Rotation /*= 0*/, int32 PaintLayer /*= 0*/)
{
	auto globalActor = UDonMeshPaintingHelper::GetGlobalManager(Hit.GetActor());
	if (!globalActor)
		return false;

	UDonMeshPainterComponent* meshPainter = globalActor->GetMeshPainterComponent(Hit.GetActor());
	if (!meshPainter)
		return false;	

	const bool bAllowContextFreePainting = false;
	const auto& strokeParams = FDonPaintStrokeParameters(PaintLayer, nullptr, nullptr, 1.f, 0.f, NAME_None, 1.f);
	FDonPaintStroke stroke = FDonPaintStroke(Hit.ImpactPoint, 1.f, Rotation, Color, Hit.FaceIndex, strokeParams, Text, Style);
	bool bStatus = meshPainter->PaintStroke(Hit, stroke, true, bAllowContextFreePainting);

	//bool bStatus = meshPainter->PaintStroke(Hit, 1.f, Color, nullptr, Rotation, nullptr, 1.f, 0.f, Text, Style, PaintLayer);

	return bStatus;
}

bool UDonMeshPaintingHelper::PaintStrokeAtComponent(class UPrimitiveComponent* PrimitiveComponent, FVector RelativeLocation, const FName SocketName /*= NAME_None*/, float BrushSize /*= 10.f*/, FLinearColor BrushColor /*= FLinearColor::White*/, UTexture2D* BrushDecalTexture /*= nullptr*/, int32 PaintLayer /*= 0*/, const bool bEraseMode/* = false*/, const FName CollisionTag /*= NAME_None*/, const float BrushOpacity /*= 1.f*/, float BrushHardness /*= 0.1f*/, const int32 BrushDecalRotation /*= 0*/, UMaterialInterface* BrushRenderMaterial /*= nullptr*/, const bool bAllowDuplicateStrokes /*= true*/, const float CollisionInflationScale /*= 1.f*/, bool bDrawDebugLocation /*= false*/)
{
	if (!PrimitiveComponent)
		return false;

	const FTransform componentToWorld = PrimitiveComponent->GetComponentTransform();
	FVector stampLocationWS = PrimitiveComponent->DoesSocketExist(SocketName) ? PrimitiveComponent->GetSocketLocation(SocketName) : componentToWorld.TransformPosition(RelativeLocation);

	// Provide a uniform interface for the painter, we wrap the input parameters into a Hit
	FHitResult requestAsHit;
	requestAsHit.Component = PrimitiveComponent;
	requestAsHit.Actor = PrimitiveComponent->GetOwner();
	requestAsHit.ImpactPoint = requestAsHit.Location = stampLocationWS; // for the sake of API consistency we always store as a world-space location

	if (bDrawDebugLocation)
	{
		DrawDebugPoint(PrimitiveComponent->GetWorld(), requestAsHit.ImpactPoint, 16.f, FColor::Green, true);
		DrawDebugSphere(PrimitiveComponent->GetWorld(), requestAsHit.ImpactPoint, 120.f, 24, FColor::Red, true);
	}

	return PaintStroke(requestAsHit, BrushSize, BrushColor, BrushDecalTexture, PaintLayer, bEraseMode, CollisionTag, BrushOpacity, BrushHardness, BrushDecalRotation, BrushRenderMaterial, bAllowDuplicateStrokes, CollisionInflationScale);

	return true;
}

bool UDonMeshPaintingHelper::PaintTextAtComponent(class UPrimitiveComponent* PrimitiveComponent, FVector RelativeLocation, const FText Text, const FDonPaintableTextStyle& Style, const FName SocketName /*= NAME_None*/, const FLinearColor Color /*= FLinearColor::White*/, const int32 Rotation /*= 0*/, int32 PaintLayer /*= 0*/, bool bDrawDebugLocation /*= false*/)
{
	if (!PrimitiveComponent)
		return false;
	
	const FTransform componentToWorld = PrimitiveComponent->GetComponentTransform();
	FVector stampLocationWS = PrimitiveComponent->DoesSocketExist(SocketName) ? PrimitiveComponent->GetSocketLocation(SocketName) : componentToWorld.TransformPosition(RelativeLocation);	

	// Provide a uniform interface for the painter, we wrap the input parameters into a Hit
	FHitResult requestAsHit;
	requestAsHit.Component = PrimitiveComponent;
	requestAsHit.Actor = PrimitiveComponent->GetOwner();
	requestAsHit.ImpactPoint = requestAsHit.Location = stampLocationWS; // for the sake of API consistency we always store as a world-space location

	if (bDrawDebugLocation)
	{
		DrawDebugPoint(PrimitiveComponent->GetWorld(), requestAsHit.ImpactPoint, 16.f, FColor::Green, true);
		DrawDebugSphere(PrimitiveComponent->GetWorld(), requestAsHit.ImpactPoint, 120.f, 24, FColor::Red, true);
	}
	
	return PaintText(requestAsHit, Text, Style, Color, Rotation, PaintLayer);

	return true;
}

bool UDonMeshPaintingHelper::PaintWorldDirect(UObject* WorldContextObject, const FVector WorldLocation, const EDonUvAxes WorldAxes, float BrushSize /*= 10.f*/, FLinearColor BrushColor /*= FLinearColor::White*/, UTexture2D* BrushDecalTexture /*= nullptr*/, int32 PaintLayer /*= 0*/, const bool bEraseMode/* = false*/, const FName CollisionTag /*= NAME_None*/, const float BrushOpacity /*= 1.f*/, float BrushHardness /*= 0.1f*/, const int32 BrushDecalRotation /*= 0*/, UMaterialInterface* BrushRenderMaterial /*= nullptr*/, const bool bAllowDuplicateStrokes /*= true*/, const float CollisionInflationScale /*= 1.f*/, const bool bPerformNetworkReplication /*= true*/)
{
	// To provide a uniform interface for the painter, we wrap the input parameters into a Hit:
	FHitResult requestAsHit;
	requestAsHit.Component = nullptr;
	requestAsHit.Actor = GetGlobalManager(WorldContextObject);
	requestAsHit.ImpactPoint = requestAsHit.Location = WorldLocation;

	const bool bAllowContextFreePainting = true;
	const auto& strokeParams = FDonPaintStrokeParameters(PaintLayer, BrushDecalTexture, BrushRenderMaterial, BrushOpacity, BrushHardness, CollisionTag, CollisionInflationScale);
	FDonPaintStroke stroke = FDonPaintStroke(requestAsHit.ImpactPoint, BrushSize, BrushDecalRotation, BrushColor, requestAsHit.FaceIndex, strokeParams, FText(), FDonPaintableTextStyle(), bEraseMode);

	// For painting directly on world-space we explicitly set the Uv method (In the regular workflow, the material of the primitive being painted on drives the Uv method)
	stroke.ExplicitUvMethod = DonMeshPaint::UvMethodFromWorldUvAxes(WorldAxes);
	
	return PaintStroke_Internal(requestAsHit, stroke, bAllowDuplicateStrokes, bAllowContextFreePainting, bPerformNetworkReplication);
}

bool UDonMeshPaintingHelper::QueryPaintCollision(UPrimitiveComponent* Primitive, FName CollisionTag, const FVector WorldLocation, float MinimumPaintBlobSize /*= 25.f*/, float CollisionInflationScale /*= 1.f*/, float CollisionInflationInAbsoluteUnits /*= 0.f*/, int32 Layer /*= 0*/)
{
	FDonPaintCollisionQuery outQuery;
	TArray<FDonPaintCollisionQuery> queries;
	queries.Add(FDonPaintCollisionQuery(Layer, CollisionTag, MinimumPaintBlobSize, CollisionInflationScale, CollisionInflationInAbsoluteUnits));

	if (!Primitive)
		return false;

	return QueryPaintCollisionMulti(Primitive, queries, outQuery, WorldLocation);
}

bool UDonMeshPaintingHelper::QueryPaintCollisionMulti(class UPrimitiveComponent* Primitive, const TArray<FDonPaintCollisionQuery>& CollisionTags, FDonPaintCollisionQuery& OutCollisionTag, const FVector WorldLocation)
{
	if (!Primitive)
		return false;

	auto globalActor = UDonMeshPaintingHelper::GetGlobalManager(Primitive->GetOwner());
	if (!globalActor)
		return false;

	UDonMeshPainterComponent* meshPainter = globalActor->GetMeshPainterComponent(Primitive->GetOwner());
	if (!meshPainter)
		return false;

	return meshPainter->QueryPaintCollisionMulti(Primitive, CollisionTags, OutCollisionTag, WorldLocation);
}

//

void UDonMeshPaintingHelper::FlushAllPaint(UObject* WorldContextObject)
{
	auto globalActor = UDonMeshPaintingHelper::GetGlobalManager(WorldContextObject);
	if (!ensure(globalActor))
		return;	

	globalActor->FlushAllPaint();
}

void UDonMeshPaintingHelper::FlushPaintForActor(AActor* Actor, TArray<UMaterialInterface*> MaterialsFilter)
{
	if (!Actor)
		return;

	auto globalActor = UDonMeshPaintingHelper::GetGlobalManager(Actor);
	if (!ensure(globalActor))
		return;

	auto meshPainter = globalActor->GetMeshPainterComponent(Actor);
	if (!ensure(meshPainter))
		return;

	if (MaterialsFilter.Num())
		meshPainter->FlushMaterials(MaterialsFilter);
	else
		meshPainter->Flush();
}

void UDonMeshPaintingHelper::DonExportPaintMasks(UObject* PaintableEntity, const FString AbsoluteDirectoryPath)
{
	auto owner = Cast<AActor>(PaintableEntity);
	if (!owner)
	{
		auto component = Cast<UActorComponent>(PaintableEntity);
		owner = component ? component->GetOwner() : nullptr;
	}

	if (!owner)
		return;

	auto globalActor = UDonMeshPaintingHelper::GetGlobalManager(owner);
	if (!globalActor)
		return;

	UDonMeshPainterComponent* meshPainter = globalActor->GetMeshPainterComponent(owner);
	if (meshPainter)
		meshPainter->DonExportPaintMasks(PaintableEntity, AbsoluteDirectoryPath);
}

FBoxSphereBounds UDonMeshPaintingHelper::GetLocalBounds(class UPrimitiveComponent* PrimitiveComponent)
{
	if (PrimitiveComponent)
	{
		// ~ Bugfix for Decal Size Consistency ~
		// No longer using cached bounds here as it causes decal brush size to vary with mesh rotation as the world space bounds change. 
		// Retaining previous code below for future reference:
		/*
		// Skeletal meshes can leverage cached bounds, so use that to avoid calculating from pose:
		if (PrimitiveComponent->IsA(USkeletalMeshComponent::StaticClass()))
		{
			const FTransform componentToWorld = PrimitiveComponent->GetComponentTransform();

			const FVector originWorldSpace = PrimitiveComponent->Bounds.Origin;
			const FVector extentsWorldSpace = PrimitiveComponent->Bounds.BoxExtent;

			const FVector boundsMinLocalSpace = componentToWorld.InverseTransformPosition(originWorldSpace - extentsWorldSpace);
			const FVector boundsMaxLocalSpace = componentToWorld.InverseTransformPosition(originWorldSpace + extentsWorldSpace);

			return FBoxSphereBounds(FBox(boundsMinLocalSpace, boundsMaxLocalSpace));
		}
		else // For other primitives, calc bounds is necessary:
		*/
		{
			auto bounds = PrimitiveComponent->CalcBounds(FTransform::Identity);
			bounds.BoxExtent *= DonMeshPaint::LocalBoundsSafetyNet;
			return bounds;
		}
	}
	else
		return FBoxSphereBounds();
}

// DEPRECATED - remove?
FBoxSphereBounds UDonMeshPaintingHelper::GetWorldBounds(class UPrimitiveComponent* PrimitiveComponent)
{
	return PrimitiveComponent->Bounds; // below code no longer appears necessary; also this value corresponds directly to Material Expression ObjectPosiitionWS and so is preferred

	/*if (PrimitiveComponent)
	{
		// Skeletal meshes can leverage cached bounds, so use that to avoid calculating from pose:
		if (PrimitiveComponent->IsA(USkeletalMeshComponent::StaticClass()))
			return PrimitiveComponent->Bounds;		
		else // For other primitives, calc bounds is necessary:
			return PrimitiveComponent->CalcBounds(PrimitiveComponent->GetComponentTransform());
	}
	else
		return FBoxSphereBounds();*/
}

class ADonMeshPainterGlobalActor* UDonMeshPaintingHelper::GetGlobalManager(UObject* Context)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(Context, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return nullptr;

	for (TActorIterator<ADonMeshPainterGlobalActor> It(World); It; ++It)
	{
		return (*It);
	}

	// We need to spawn the manager:
	FActorSpawnParameters managerSpawnParams = FActorSpawnParameters();
	managerSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto globalManager = World->SpawnActor<ADonMeshPainterGlobalActor>(ADonMeshPainterGlobalActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, managerSpawnParams);

return globalManager;
}

// UV Acquisition Helpers:
bool UDonMeshPaintingHelper::GetCollisionUV(UObject* PaintableComponent, const FVector WorldLocation, const int32 FaceIndex, const int32 UvChannel, FVector2D& OutUV)
{
	auto paintableAsPrimitive = Cast<UPrimitiveComponent>(PaintableComponent);
	if (!paintableAsPrimitive)
		return false;
	
	FHitResult paintableAsHit;
	paintableAsHit.Component = paintableAsPrimitive;
	paintableAsHit.Actor = paintableAsPrimitive->GetOwner();
	paintableAsHit.Location = paintableAsHit.ImpactPoint = WorldLocation;
	paintableAsHit.FaceIndex = FaceIndex;

	bool bFoundUV = UGameplayStatics::FindCollisionUV(paintableAsHit, UvChannel, OutUV);
	if (!bFoundUV)
		return false;

	SanitizeCollisionUV(OutUV);

	return true;
}

void UDonMeshPaintingHelper::SanitizeCollisionUV(FVector2D &HitUV)
{
	if (HitUV.X > 1.f)
		HitUV.X = FMath::Fractional(HitUV.X);
	else if (HitUV.X < 0.f)
		HitUV.X = 1 + FMath::Fractional(HitUV.X);

	if (HitUV.Y > 1.f)
		HitUV.Y = FMath::Fractional(HitUV.Y);
	else	if (HitUV.Y < 0.f)
		HitUV.Y = 1 + FMath::Fractional(HitUV.Y);
}

bool UDonMeshPaintingHelper::GetPaintableWorldBounds(UObject* WorldContextObject, FVector& OutWorldMin, FVector& OutWorldMax, const EDonUvAxes UvAxes)
{
	// Step 1: Obtain the Don Painter MPC:
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return false;

	auto globalPainter = GetGlobalManager(WorldContextObject);
	if (!globalPainter)
		return false;

	auto donPainterMPC = globalPainter->GlobalMeshPaintingParameters;
	auto donPainterMPCInstance = donPainterMPC ? World->GetParameterCollectionInstance(donPainterMPC) : nullptr;
	if (!donPainterMPCInstance)
		return false;	

	// Step 2. Extract World Min and World Max coordinates:
	FLinearColor worldMin, worldMax;
	const bool bMinParamFound = donPainterMPCInstance->GetVectorParameterValue(DonMeshPaint::P_WorldMin, worldMin);
	if (!bMinParamFound)
		return false;

	const bool bMaxParamFound = donPainterMPCInstance->GetVectorParameterValue(DonMeshPaint::P_WorldMax, worldMax);
	if (!bMaxParamFound)
		return false;

	OutWorldMin = FVector(worldMin);
	OutWorldMax = FVector(worldMax);

	if (OutWorldMin == OutWorldMax)
		return false; // this is invalid for pretty much every usecase / operation

	return true;
}

bool UDonMeshPaintingHelper::GetPaintableLocalBounds(class UPrimitiveComponent* PrimitiveComponent, FVector& OutLocalMin, FVector& OutLocalMax)
{
	const FBoxSphereBounds localBounds = UDonMeshPaintingHelper::GetLocalBounds(PrimitiveComponent);
	OutLocalMin = localBounds.Origin - localBounds.BoxExtent;
	OutLocalMax = localBounds.Origin + localBounds.BoxExtent;

	if (OutLocalMin == OutLocalMax)
		return false; // this is invalid for pretty much every usecase / operation

	return true;
}

bool UDonMeshPaintingHelper::GetWorldSpacePlanarUV(UObject* WorldContextObject, FVector WorldLocation, const EDonUvAxes UvAxes, FVector2D& OutUV)
{
	// Step 1. Acquire the global bounds of the paintable world space (stored in a MPC):
	FVector worldMin, worldMax;
	bool bAcquiredWorldBounds = GetPaintableWorldBounds(WorldContextObject, worldMin, worldMax, UvAxes);
	if (!bAcquiredWorldBounds)
		return false;

	// ~~~

	// Step 2. Compute UVs:
	const bool bNeedsUVWRangeValidation = true;
	return GetPlanarUV(OutUV, WorldLocation, worldMin, worldMax, UvAxes, bNeedsUVWRangeValidation);
}

bool UDonMeshPaintingHelper::GetRelativeSpacePlanarUV(class UPrimitiveComponent* PrimitiveComponent, FVector WorldLocation, const EDonUvAxes UvAxes, FVector2D& OutUV)
{
	if (!PrimitiveComponent)
		return false;

	FVector localMin, localMax;
	bool bLocalBoundsAcquired = UDonMeshPaintingHelper::GetPaintableLocalBounds(PrimitiveComponent, localMin, localMax);
	if (!bLocalBoundsAcquired)
		return false;

	const FVector relativeLocation = PrimitiveComponent->GetComponentTransform().InverseTransformPosition(WorldLocation);

	return GetPlanarUV(OutUV, relativeLocation, localMin, localMax, UvAxes);
}

bool UDonMeshPaintingHelper::GetPlanarUV(FVector2D& OutUV, FVector PlaneLocation, FVector PlaneMin, FVector PlaneMax, const EDonUvAxes UvAxes, bool bNeedsUVWRangeValidation /*= false*/)
{
	FVector uvw;
	const bool bSuccess = GetPlanarUVW(uvw, PlaneLocation, PlaneMin, PlaneMax);
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetPlanarUVW fail"), *uvw.ToString());
		return false;
	}

	if (bNeedsUVWRangeValidation)
	{
		// UV range validation as a paint stroke can fall outside the plane (unlikely for relative space as it is bounds based, but possible for world-space as it is user-set)
		if (UvAxes == EDonUvAxes::XY)
		{
			if(uvw.X < 0.f || uvw.X > 1.f || uvw.Y < 0.f || uvw.Y > 1.f)
			{
				UE_LOG(LogTemp, Warning, TEXT("GetPlanarUV found invalid uvw: %s"), *uvw.ToString());
				return false; // For XY, we relax the Z validation check (it is very rare on landscapes for this to matter, this allows a smoother worklow for end-users)
			}
		}
		else if (uvw.X < 0.f || uvw.X > 1.f || uvw.Y < 0.f || uvw.Y > 1.f || uvw.Z < 0.f || uvw.Z > 1.f)
		{
			UE_LOG(LogTemp, Warning, TEXT("Uvw validation fail"), *uvw.ToString());
			return false;
		}
	}	

	OutUV = FVector2D(uvw.X, uvw.Y);
	switch (UvAxes)
	{
	case EDonUvAxes::XY:
		OutUV = FVector2D(uvw.X, uvw.Y);
		break;

	case EDonUvAxes::XZ:
		OutUV = FVector2D(uvw.X, uvw.Z);
		break;

	case EDonUvAxes::YZ:
		OutUV = FVector2D(uvw.Y, uvw.Z);
		break;
	}

	return true;
}

bool UDonMeshPaintingHelper::GetPlanarUVW(FVector& OutUVW, FVector PlaneLocation, FVector PlaneMin, FVector PlaneMax)
{
	if (PlaneMax == PlaneMin)
		return false; // prevent division by zero!

	OutUVW = (PlaneLocation - FVector(PlaneMin)) / (PlaneMax - PlaneMin);	

	return true;
}

bool UDonMeshPaintingHelper::GetPositionsTextureUV(class UTextureRenderTarget2D* PositionsTexture, const FBoxSphereBounds ReferenceBounds, FVector RelativePosition, class UPrimitiveComponent* PrimitiveComponent, FVector2D& OutUV, const float DesiredMinimumProximity/* = 3.f*/, const float MaximumPermittedProximity /*= 50.f*/, const int32 TravelStepPer512p/* = 16*/)
{
	if (!PositionsTexture)
		return false;
	
	FRenderTarget *RenderTarget = PositionsTexture->GameThread_GetRenderTargetResource();
	TArray<FFloat16Color> positionsAsPixels;
	RenderTarget->ReadFloat16Pixels(positionsAsPixels);

	return GetPositionsTextureUV(positionsAsPixels, ReferenceBounds, PositionsTexture->SizeX, PositionsTexture->SizeY, RelativePosition, PrimitiveComponent, OutUV, DesiredMinimumProximity, TravelStepPer512p);
}

bool UDonMeshPaintingHelper::GetPositionsTextureUV(const TArray<FFloat16Color>& PositionsAsPixels, const FBoxSphereBounds ReferenceBounds, const int32 TextureSizeX, const int32 TextureSizeY, FVector RelativePosition, class UPrimitiveComponent* PrimitiveComponent, FVector2D& OutUV, const float DesiredMinimumProximity /*= 1.f*/, const float MaximumPermittedProximity /*= 50.f*/, const int32 TravelStepPer512p /*= 16*/)
{
	if (!PrimitiveComponent)
		return false;

	const FVector localMin = ReferenceBounds.Origin - ReferenceBounds.BoxExtent;
	const FVector localMax = ReferenceBounds.Origin + ReferenceBounds.BoxExtent;	
	if (localMin == localMax)
		return false;

	const int32 sizeX = TextureSizeX;
	const int32 sizeY = TextureSizeY;
	const int32 travelStepX = TravelStepPer512p * (sizeX / 512.f);
	const int32 travelStepY = TravelStepPer512p * (sizeY / 512.f);

	float minimumDistanceEncountered = -1.f;
	FVector minimumDistanceUnpackedPosition;

	bool bFoundUV = false;

	for (int32 i = 0; i < sizeX; i += travelStepX)
	{
		for (int32 j = 0; j < sizeY; j += travelStepY)
		{
			const int32 pixelIndex = i * sizeX + j;
			if (!ensure(PositionsAsPixels.IsValidIndex(pixelIndex)))
				continue;

			const FFloat16Color packedPositionAsColor = PositionsAsPixels[pixelIndex];
			const FVector packedPosition = FVector(packedPositionAsColor.R, packedPositionAsColor.G, packedPositionAsColor.B);
			const FVector unpackedPosition = (localMax - localMin) * packedPosition + localMin;

			const float distance = FVector::Dist(RelativePosition, unpackedPosition);

			// Debug pixels:
			//auto worldPosition = PrimitiveComponent->GetComponentTransform().TransformPosition(unpackedPosition);
			//DrawDebugPoint(PrimitiveComponent->GetWorld(), worldPosition, 3.f, FColor::Red, true);
			//UE_LOG(LogDonMeshPainting, Display, TEXT("Sampling location: (Rel) %s (World) %s Distance: %f/%f"), *unpackedPosition.ToString(), *worldPosition.ToString(), distance, DesiredMinimumProximity);

			if (distance < DesiredMinimumProximity)
			{
				OutUV = IndexToUV(i, j, sizeX, sizeY);
				UE_LOG(LogDonMeshPainting, VeryVerbose, TEXT("OutUV: %s Distance: %f"), *OutUV.ToString(), distance);
				bFoundUV = true;
				break;
			}
			else if (minimumDistanceEncountered == -1.f || distance < minimumDistanceEncountered)
			{
				OutUV = IndexToUV(i, j, sizeX, sizeY);
				minimumDistanceEncountered = distance;
				minimumDistanceUnpackedPosition = unpackedPosition;
			}
		}
	}

	if (!bFoundUV)
	{
		UE_LOG(LogDonMeshPainting, Verbose, TEXT("Unable to find UV with sufficient accuracy. Closest position found had a distance of: %f"), minimumDistanceEncountered);

		if (minimumDistanceEncountered > MaximumPermittedProximity)
			return false;
	}

	return true;
}

class UMeshComponent* UDonMeshPaintingHelper::GetMostProbablePaintingCandidate(APawn* Pawn)
{
	TArray<USkeletalMeshComponent*> skeletalMeshComponents;
	Pawn->GetComponents(skeletalMeshComponents);
	if (skeletalMeshComponents.Num() > 0)
		return skeletalMeshComponents[0];

	TArray<UStaticMeshComponent*> staticMeshComponents;
	Pawn->GetComponents(staticMeshComponents);
	if (staticMeshComponents.Num() > 0)
		return staticMeshComponents[0];
	
	return nullptr;
}

FVector2D UDonMeshPaintingHelper::BrushSizeFromLocalBounds(UObject* PaintableComponent, const float BrushSize, const FVector ComponentScaleDivisor, const float DecalProjectionHeuristic, const EDonUvAcquisitionMethod UvAcquisitionMethod, const bool bAlwaysNormalizeBrushSize /*= false*/)
{
	FVector localMin, localMax;
	bool bAcquiredLocalBounds = UDonMeshPaintingHelper::GetPaintableLocalBounds(Cast<UPrimitiveComponent>(PaintableComponent), localMin, localMax);
	if (!ensure(bAcquiredLocalBounds))
		return FVector2D(BrushSize, BrushSize);

	FVector2D outBrushSizeUV;
	const EDonUvAxes uvAxes = DonMeshPaint::LocalUvAxesFromUvMethod(UvAcquisitionMethod);
	UDonMeshPaintingHelper::BrushSizeAsBoundsUvRatio(BrushSize * DecalProjectionHeuristic, outBrushSizeUV, localMin, localMax, uvAxes, ComponentScaleDivisor, bAlwaysNormalizeBrushSize);

	return outBrushSizeUV;
}

bool UDonMeshPaintingHelper::BrushSizeFromWorldBounds(UObject* PaintableComponent, const float BrushSize, FVector2D& OutBrushSizeUV, const FVector ComponentScaleDivisor, const float DecalProjectionHeuristic, const EDonUvAcquisitionMethod UvAcquisitionMethod)
{
	FVector worldMin, worldMax;
	bool bWorldBoundsAcquired = UDonMeshPaintingHelper::GetPaintableWorldBounds(PaintableComponent, worldMin, worldMax, DonMeshPaint::WorldUvAxesFromUvMethod(UvAcquisitionMethod));
	if (!ensure(bWorldBoundsAcquired))
		return false;

	FVector2D outBrushSizeUV;
	const EDonUvAxes uvAxes = DonMeshPaint::WorldUvAxesFromUvMethod(UvAcquisitionMethod);

	return UDonMeshPaintingHelper::BrushSizeAsBoundsUvRatio(BrushSize * DecalProjectionHeuristic, OutBrushSizeUV, worldMin, worldMax, uvAxes);
}

bool UDonMeshPaintingHelper::BrushSizeAsBoundsUvRatio(const float BrushSize, FVector2D& OutBrushSizeUV, const FVector BoundsMin, const FVector BoundsMax, const EDonUvAxes UvAxes, const FVector ComponentScaleDivisor /*= FVector(1, 1, 1)*/, const bool bAlwaysNormalizeBrushSize /*= false*/)
{
	//GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::Blue, FString::Printf(TEXT("BoundsMax: %s, BoundsMin: %s"), *BoundsMax.ToString(), *BoundsMin.ToString()));

	const float xExtent = (BoundsMax.X - BoundsMin.X) * ComponentScaleDivisor.X;
	const float yExtent = (BoundsMax.Y - BoundsMin.Y) * ComponentScaleDivisor.Y;
	const float zExtent = (BoundsMax.Z - BoundsMin.Z) * ComponentScaleDivisor.Z;

	switch (UvAxes)
	{
	case EDonUvAxes::XY:
		if (xExtent == 0 || yExtent == 0)
			return false;

		OutBrushSizeUV = FVector2D(BrushSize / xExtent, BrushSize / yExtent);
		break;

	case EDonUvAxes::XZ:
		if (xExtent == 0 || zExtent == 0)
			return false;

		OutBrushSizeUV = FVector2D(BrushSize / xExtent, BrushSize / zExtent);
		break;

	case EDonUvAxes::YZ:
		if (yExtent == 0 || zExtent == 0)
			return false;

		OutBrushSizeUV = FVector2D(BrushSize / yExtent, BrushSize / zExtent);
		break;

	default: // Fallback: Not accurate, but currently the only reasonable technique for the Mesh UV workflow to project decals with a worldspace brush size (rather than explicit UV brush size)
		const float sumExtents = xExtent + yExtent + zExtent;
		if (sumExtents == 0)
			return false;

		float averageBrushSize = BrushSize * 3.f / sumExtents;
		OutBrushSizeUV = FVector2D(averageBrushSize, averageBrushSize);
	}

	// Prevent negative brush size:
	OutBrushSizeUV = FVector2D(FMath::Abs(OutBrushSizeUV.X), FMath::Abs(OutBrushSizeUV.Y));

	if (bAlwaysNormalizeBrushSize)
	{
		const float averageBrushSize = (OutBrushSizeUV.X + OutBrushSizeUV.Y) / 2.f;
		OutBrushSizeUV = FVector2D(averageBrushSize, averageBrushSize);
	}

	return true;
}

void UDonMeshPaintingHelper::DrawDebugPositionsTexture(class UTextureRenderTarget2D* PositionsTexture, class UPrimitiveComponent* PrimitiveComponent)
{
	UWorld* const World = GEngine->GetWorldFromContextObject(PrimitiveComponent, EGetWorldErrorMode::ReturnNull);
	if (!World)
		return;

	const FTransform componentToWorld = PrimitiveComponent->GetComponentTransform();

	FVector localMin, localMax;
	bool bLocalBoundsAcquired = UDonMeshPaintingHelper::GetPaintableLocalBounds(PrimitiveComponent, localMin, localMax);
	if (!bLocalBoundsAcquired)
		return;
	
	TArray<FFloat16Color> positionsAsPixels;
	FRenderTarget *RenderTarget = PositionsTexture->GameThread_GetRenderTargetResource();	
	RenderTarget->ReadFloat16Pixels(positionsAsPixels);

	const int32 sizeX = PositionsTexture->SizeX;
	const int32 sizeY = PositionsTexture->SizeY;

	for (int32 i = 0; i < sizeX; i ++)
	{
		for (int32 j = 0; j < sizeY; j ++)
		{
			const int32 pixelIndex = i * sizeX + j;
			if (!ensure(positionsAsPixels.IsValidIndex(pixelIndex)))
				continue;

			const FFloat16Color packedPositionAsColor = positionsAsPixels[pixelIndex];
			const FVector packedPosition = FVector(packedPositionAsColor.R, packedPositionAsColor.G, packedPositionAsColor.B);
			const FVector unpackedPosition = (localMax - localMin) * packedPosition + localMin;
			const FVector worldSpacePosition = componentToWorld.TransformPosition(unpackedPosition);

			DrawDebugPoint(World, worldSpacePosition, 6.f, FColor::Red, true);
		}
	}
}

void UDonMeshPaintingHelper::DrawTextureToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UTexture* Texture)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || !TextureRenderTarget || !TextureRenderTarget->Resource || !Texture)
		return;

	UCanvas* Canvas = World->GetCanvasForDrawMaterialToRenderTarget();
	Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr, nullptr);
	Canvas->Update();

	FCanvas RenderCanvas(TextureRenderTarget->GameThread_GetRenderTargetResource(), nullptr, World, World->FeatureLevel);
	Canvas->Canvas = &RenderCanvas;

	FDrawEvent* DrawTextureToTargetEvent = new FDrawEvent();
	FName RTName = TextureRenderTarget->GetFName();

	ENQUEUE_RENDER_COMMAND(BeginDrawEventCommand)(
		[RTName, DrawTextureToTargetEvent](FRHICommandList& RHICmdList)
	{
		BEGIN_DRAW_EVENTF(RHICmdList, DrawCanvasToTarget, (*DrawTextureToTargetEvent), *RTName.ToString());
	});

	Canvas->K2_DrawTexture(Texture, FVector2D(0, 0), FVector2D(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY), FVector2D(0, 0), FVector2D::UnitVector, FLinearColor::White, EBlendMode::BLEND_Additive);
	RenderCanvas.Flush_GameThread();
	Canvas->Canvas = NULL;

	FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(CanvasRenderTargetResolveCommand)(
		[RenderTargetResource, DrawTextureToTargetEvent](FRHICommandList& RHICmdList)
	{
		RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, FResolveParams());
		STOP_DRAW_EVENT((*DrawTextureToTargetEvent));
		delete DrawTextureToTargetEvent;
	}
	);
}

void UDonMeshPaintingHelper::DrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material)
{
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, TextureRenderTarget, Material);
}

void UDonMeshPaintingHelper::DrawTextToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, const FText Text, FVector2D ScreenPosition, const float TextRotation, const FLinearColor BrushColor, const FDonPaintableTextStyle& TextStyle)
{
	ensure(WorldContextObject);
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World || !TextureRenderTarget || !TextureRenderTarget->Resource)
		return;

	UCanvas* Canvas = World->GetCanvasForDrawMaterialToRenderTarget();
	Canvas->Init(TextureRenderTarget->SizeX, TextureRenderTarget->SizeY, nullptr, nullptr);
	Canvas->Update();

	FCanvas RenderCanvas(
		TextureRenderTarget->GameThread_GetRenderTargetResource(),
		nullptr,
		World,
		World->FeatureLevel);

	Canvas->Canvas = &RenderCanvas;

	FDrawEvent* DrawTextureToTargetEvent = new FDrawEvent();

	FName RTName = TextureRenderTarget->GetFName();
	ENQUEUE_RENDER_COMMAND(BeginDrawEventCommand)(
		[&RTName, DrawTextureToTargetEvent](FRHICommandList& RHICmdList)
	{
		BEGIN_DRAW_EVENTF(
			RHICmdList,
			DrawCanvasToTarget,
			(*DrawTextureToTargetEvent),
			*RTName.ToString());
	});

	FCanvasTextItem TextItem(ScreenPosition, Text, GEngine->GetMediumFont(), BrushColor);
	
	TextItem.SlateFontInfo = FSlateFontInfo();
	TextItem.SlateFontInfo->FontObject = TextStyle.FontObject ? TextStyle.FontObject : GEngine->GetMediumFont();
	TextItem.SlateFontInfo->FontMaterial = TextStyle.FontMaterial;
	TextItem.SlateFontInfo->TypefaceFontName = TextStyle.TypefaceFontName;
	TextItem.SlateFontInfo->Size = TextStyle.FontSize;	

	TextItem.Scale = FVector2D(1, 1);
	TextItem.bCentreX = true;
	TextItem.bCentreY = true;
	TextItem.HorizSpacingAdjust = TextStyle.Kerning;	

	
	// Perform Text rotation:
	FVector AnchorPos(0, 0, 0);
	FRotationMatrix RotMatrix(FRotator(0.f, TextRotation, 0.f));
	FMatrix TransformMatrix;
	TransformMatrix = FTranslationMatrix(-AnchorPos) * RotMatrix * FTranslationMatrix(AnchorPos);

	FVector TestPos(ScreenPosition.X, ScreenPosition.Y, 0.0f);	
	FMatrix FinalTransform = FTranslationMatrix(-TestPos) * TransformMatrix * FTranslationMatrix(TestPos);
	Canvas->Canvas->PushRelativeTransform(FinalTransform);
	
	Canvas->DrawItem(TextItem);
	

	RenderCanvas.Flush_GameThread();
	Canvas->Canvas = NULL;
	
	FTextureRenderTargetResource* RenderTargetResource = TextureRenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(CanvasRenderTargetResolveCommand)(
		[RenderTargetResource, DrawTextureToTargetEvent](FRHICommandList& RHICmdList)
	{
		RHICmdList.CopyToResolveTarget(RenderTargetResource->GetRenderTargetTexture(), RenderTargetResource->TextureRHI, FResolveParams());
		STOP_DRAW_EVENT((*DrawTextureToTargetEvent));
		delete DrawTextureToTargetEvent;
	}
	);
}

UTextureRenderTarget2D* UDonMeshPaintingHelper::CloneRenderTarget(UTextureRenderTarget2D* SourceRenderTarget)
{
	auto renderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), UTextureRenderTarget2D::StaticClass());
	renderTarget->InitAutoFormat(SourceRenderTarget->SizeX, SourceRenderTarget->SizeY);
	renderTarget->RenderTargetFormat = SourceRenderTarget->RenderTargetFormat;

	renderTarget->UpdateResourceImmediate();

	return renderTarget;
}

TArray<uint8> UDonMeshPaintingHelper::ExtractRenderTargetAsPNG(UObject* WorldContextObject, UTextureRenderTarget2D* RenderTarget, const bool bFillAlpha /*=false*/)
{
	TArray<uint8> imageBytes;

	if (!RenderTarget)
		return imageBytes;

	const float width = RenderTarget->GetSurfaceWidth();
	const float height = RenderTarget->GetSurfaceHeight();
	FTextureRenderTargetResource *rtResource = RenderTarget->GameThread_GetRenderTargetResource();
	FReadSurfaceDataFlags readPixelFlags(RCM_UNorm);

	TArray<FColor> outBMP;
	outBMP.AddUninitialized(width * height);
	rtResource->ReadPixels(outBMP, readPixelFlags);	

	if(bFillAlpha)
	{
		for (auto& pixel : outBMP)
			pixel = FColor(pixel.R, pixel.G, pixel.B, 255);
	}

	FIntPoint destSize(width, height);
	FDonBakedPaintPerMaterial bakedPaint;
	FImageUtils::CompressImageArray(destSize.X, destSize.Y, outBMP, imageBytes);

	return imageBytes;
}