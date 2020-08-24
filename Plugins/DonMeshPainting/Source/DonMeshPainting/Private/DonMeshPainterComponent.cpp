// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "DonMeshPainterComponent.h"
#include "DonMeshPaintingPrivatePCH.h"

#include "Runtime/RHI/Public/RHI.h"

#include "Runtime/Landscape/Classes/Landscape.h"
#include "Runtime/Landscape/Classes/LandscapeProxy.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Classes/Kismet/KismetRenderingLibrary.h"
#include "PhysicsEngine/PhysicsSettings.h"

#include "Runtime/ImageWrapper/Public/IImageWrapper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"

// Networking:
#include "Net/UnrealNetwork.h"

#define DON_MESH_CELL_DEBUGGING_ENABLED 0

UDonMeshPainterComponent::UDonMeshPainterComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UDonMeshPainterComponent::OnRep_MeshStrokes()
{
	if (!GlobalPainter())
	{
		bHasPendingUpdates = true;
		return; // ensures that clients can begin painting only after the server has replicated the global painter
	}

	ProcessPaintStrokes();
}

void UDonMeshPainterComponent::ProcessPaintStrokes(const bool bIsSaveLoadCycle /*= false*/, const bool bIsUsingBakedTextures /*= false*/)
{
	for (FDonPaintableMeshData& data : Rep_MeshPaintStrokes)
	{
		auto paintable = data.Paintable.Get();
		if (!paintable)
			continue;

		const auto originalTransform = GetPaintableComponentTransform(paintable);
		if(bIsSaveLoadCycle)
			SetPaintableComponentTransform(paintable, data.ReferenceTransform); // usecase: loading effect templates onto multiple actors

		bool bSetupDataAndSkipRendering = bIsSaveLoadCycle && bIsUsingBakedTextures && DonMeshPaint::NeedsBakedTextures(paintable);

		for (int32 i = data.NumPaintStrokes; i < data.PaintStrokes.Num(); i++)
		{
			RenderPaintStroke(paintable, data.PaintStrokes[i], bSetupDataAndSkipRendering);
		}

		data.RefreshNumStrokesCount();

		if(bIsSaveLoadCycle)
			SetPaintableComponentTransform(paintable, originalTransform); // restore
	}
}

DonMeshCellUnit UDonMeshPainterComponent::GetMeshCell(UObject* PaintableComponent, FVector WorldLocation)
{
	if (!PaintableComponent)
		return DonMeshCellUnit();

	const int32 cellSize = MeshCellSize;

	FVector localPosition = GetPaintableComponentTransform(PaintableComponent).InverseTransformPosition(WorldLocation);
	DonMeshCellUnit cellLocation = DonMeshCellUnit(localPosition.X / cellSize, localPosition.Y / cellSize, localPosition.Z / cellSize);

	return cellLocation;
}

EDonUvAcquisitionMethod UDonMeshPainterComponent::ResolveUvAcquisitionMethod(const FName DefaultLayerName, UObject* PaintableComponent, const int32 MaterialIndex, const int32 FaceIndex, const bool bNeedsUVProjection, const int32 UvChannel)
{
	EDonUvAcquisitionMethod result = EDonUvAcquisitionMethod::UnwrappedPositionsTexture; // placate compiler

	if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_WorldSpaceXY))
	{
		result = EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY;
	}
	else if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_LocalSpaceXY))
	{
		result = EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXY;
	}
	else if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_MeshGeneric))
	{
		// Skeletal Mesh / Et Al:
		result = EDonUvAcquisitionMethod::UnwrappedPositionsTexture;

		if (PaintableComponent->IsA(UStaticMeshComponent::StaticClass()) && MaterialIndex == 0)
		{
			if (bNeedsUVProjection || UvChannel == 0) // If the UV channel used is 0, then we probably don't have a lightmap UV anyway, so only option is to use collision UVs
			{
				// Static Meshes have the unique ability to support Collision UVs via face index. For projecting decals this offers a simpler workflow:
				// Note: Only one material can be supported via this workflow, hence the index check. In a multi-material setup it is not possible to (easily) determine which material the collision UVs correlate to.
				// @toinvestigate: 4.16 now supports hit-based material lookup so the zero material limitation can be removed; will need some refactoring :)

				if (UPhysicsSettings::Get()->bSupportUVFromHitResults && FaceIndex != -1)
					result = EDonUvAcquisitionMethod::ByCollisionUVs;
				else if(UvChannel > 0)
					LogWarning_CollisionUVSupportNotAvailable(PaintableComponent); // We're still fine, UV projection will occur via a positions texture lookup
				else
					LogError_CollisionUVSupportNotAvailable(PaintableComponent); // We have neither collision UV support, nor a lightmap UV. This is an error condition!
			}
		}		
	}
	else if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_WorldSpaceXZ))
	{
		result = EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ;
	}
	else if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_WorldSpaceYZ))
	{
		result = EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ;
	}
	else if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_LocalSpaceXZ))
	{
		result = EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXZ;
	}
	else if (DefaultLayerName.IsEqual(DonMeshPaint::DonPaintLayer_LocalSpaceYZ))
	{
		result = EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceYZ;
	}

	return result;
}

bool UDonMeshPainterComponent::ResolveUVs(const EDonUvAcquisitionMethod UvAcquisitionMethod, UObject* PaintableComponent, const FDonPaintStroke& Stroke, const int32 UvChannel, FVector2D& OutUV)
{
	// Step 1. UV Resolution by acquisition method:
	switch (UvAcquisitionMethod)
	{
	case EDonUvAcquisitionMethod::ByCollisionUVs:
	{
		const bool bFoundUV = UDonMeshPaintingHelper::GetCollisionUV(PaintableComponent, Stroke.WorldLocation, Stroke.FaceIndex, UvChannel, OutUV);
		if (!bFoundUV)
		{
			LogError_HitUVMissing(PaintableComponent);
			return false;
		}

		break;
	}

	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY:
	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ:
	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ:
	{
		const bool bFoundUV = UDonMeshPaintingHelper::GetWorldSpacePlanarUV(PaintableComponent, Stroke.WorldLocation, DonMeshPaint::WorldUvAxesFromUvMethod(UvAcquisitionMethod), OutUV);
		if (!bFoundUV)
		{
			LogError_GlobalBoundsNotSetup();
			return false;
		}

		break;
	}

	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXY:
	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXZ:
	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceYZ:
	{
		const bool bFoundUV = UDonMeshPaintingHelper::GetRelativeSpacePlanarUV(Cast<UPrimitiveComponent>(PaintableComponent), Stroke.WorldLocation, DonMeshPaint::LocalUvAxesFromUvMethod(UvAcquisitionMethod), OutUV);
		if (!bFoundUV)
			return false;

		break;
	}

	case EDonUvAcquisitionMethod::UnwrappedPositionsTexture:
	{
		// For positions texture, UVs are only calculated if a decal texture is used, in which case it needs to be calculated per material.
		// That work is performed further down the chain inside RenderPaintStroke (mainly for convenience) via a per-material positions texture lookup
		OutUV = FVector2D::ZeroVector;
		break;
	}	
	}

	// Step 2. Validate UVs:

	// This is mainly necessary for planar UV mapping where a paint stroke can fall outside the plane (unlikely for relative space as it is bounds based, but possible for world-space as it is user-set)
	if (OutUV.X < 0.f || OutUV.X > 1.f || OutUV.Y < 0.f || OutUV.Y > 1.f)
		return false;

	return true;
}

FVector2D UDonMeshPainterComponent::GetEffectiveBrushSizeWS(UObject* PaintableComponent, const float BrushSize, const EDonUvAcquisitionMethod UvAcquisitionMethod, bool bNeedsDecalTexture)
{
	FVector componentScale = GetPaintableComponentScaleDivisor(PaintableComponent);
	float decalProjectionHeuristic = bNeedsDecalTexture ? 1.33f : 1.f;

	switch (UvAcquisitionMethod)
	{

	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY:
	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ:
	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ:
	{
		FVector2D outBrushSizeUV;
		bool bBrushSizeValid = UDonMeshPaintingHelper::BrushSizeFromWorldBounds(PaintableComponent, BrushSize, outBrushSizeUV, componentScale, decalProjectionHeuristic, UvAcquisitionMethod);
		if (bBrushSizeValid)
			return outBrushSizeUV;
		else
		{
			LogError_GlobalBoundsNotSetup();
			break;
		}
	}

	case EDonUvAcquisitionMethod::UnwrappedPositionsTexture:
		if (!bNeedsDecalTexture)
			return FVector2D(BrushSize, BrushSize) / componentScale.X;
		// else, flow over to the next case:

	case EDonUvAcquisitionMethod::ByCollisionUVs:
	{
		decalProjectionHeuristic = 2.f; // Rough heuristic to accommodate for bounds being generally larger than the actual mesh

		const bool bAlwaysNormalizeBrushSize = true;
		return UDonMeshPaintingHelper::BrushSizeFromLocalBounds(PaintableComponent, BrushSize, componentScale, decalProjectionHeuristic, UvAcquisitionMethod, bAlwaysNormalizeBrushSize);
	}

	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXY:
	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXZ:
	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceYZ:
	{
		const bool bAlwaysNormalizeBrushSize = false;
		return UDonMeshPaintingHelper::BrushSizeFromLocalBounds(PaintableComponent, BrushSize, componentScale, decalProjectionHeuristic, UvAcquisitionMethod, bAlwaysNormalizeBrushSize);
	}	
	}
	
	return FVector2D(BrushSize, BrushSize);
}

UObject* UDonMeshPainterComponent::ResolvePaintableEntity(class UPrimitiveComponent* PrimitiveComponent, bool bAllowContextFreePainting, bool bWarnIfShapeCapsuleEncountered /*= true*/)
{
	if (bAllowContextFreePainting && PrimitiveComponent == nullptr)
		return GlobalPainter(); // this is the "Paint World Direct" workflow, where users are not required to provide any primitive or actor context for painting!

	if (!PrimitiveComponent)
		return nullptr;

	// Handle primitive components:
	UObject* paintableComponent = PrimitiveComponent;
	AActor* paintableOwner = PrimitiveComponent->GetOwner();	

	if (!paintableComponent)
	{
		LogError_HitContainsNoPaintableObject();
		return nullptr;
	}

	// Advanced edgecase: Is the user trying to paint on a collision capsule instead of an actual mesh?
	if (paintableComponent->IsA(UShapeComponent::StaticClass()))
	{
		paintableComponent = nullptr; // start over

		// If it's a pawn, we can rescue and alert them:
		if (paintableOwner && paintableOwner->IsA(APawn::StaticClass()))
		{
			auto pawn = Cast<APawn>(paintableOwner);
			paintableComponent = UDonMeshPaintingHelper::GetMostProbablePaintingCandidate(pawn);

			if(bWarnIfShapeCapsuleEncountered)
				LogWarning_TryingToPaintOnACollisionCapsule(pawn, PrimitiveComponent);

			return paintableComponent; // done
		}

		if(!paintableComponent) // If we haven't found a suitable substitute, we must return:
		{
			if(bWarnIfShapeCapsuleEncountered)
				LogError_HitContainsNoPaintableObject(paintableComponent);
			return nullptr;
		}
	}

	// Handle landscapes:
	auto landscapeActor = Cast<ALandscape>(paintableOwner);
	if (landscapeActor)
		paintableComponent = landscapeActor; // For landscapes we extract the actor itself instead of the hit component (which is a ULandscapeHeightfieldCollisionComponent)

	return paintableComponent;
}

bool UDonMeshPainterComponent::PaintStroke(const FHitResult& Hit, const FDonPaintStroke& Stroke, const bool bAllowDuplicateStrokes, bool bAllowContextFreePainting, bool bPerformNetworkReplication /*= true*/)
{
	// 1) Validate and resolve the Paintable entity:
	UObject* paintableComponent = ResolvePaintableEntity(Hit.GetComponent(), bAllowContextFreePainting);
	if (!paintableComponent)
		return false;

	const float minDistanceFactor = 0.25f; // 0.01f;
	if (!bAllowDuplicateStrokes && !Stroke.bEraseMode && IsMeshPaintedAt(paintableComponent, Hit.ImpactPoint, minDistanceFactor, Stroke.StrokeParams.PaintLayerIndex))
	{
		UE_LOG(LogDonMeshPainting, Verbose, TEXT("Repeat stroke detected. Ignoring this stroke..."));
		return false;
	}

	bool bStatus;

	// 2a) If this is a dedicated server, just setup the painting prerequisites and validate:
	if (IsRunningDedicatedServer())
	{
		UE_LOG(LogDonMeshPainting, Verbose, TEXT("[Dedicated Server] Received paint request"));
		bStatus = SetupPaintingPrerequisites(paintableComponent, Stroke) != nullptr;
		if(!bStatus)
			UE_LOG(LogDonMeshPainting, Error, TEXT("[Dedicated Server] Unable to setup prerequisites. Cannot process this paint request."));
	}
	else
		// 2b) If this is a listen-server or client, render the stroke immediately:
		bStatus = RenderPaintStroke(paintableComponent, Stroke);

	// 3) Finally, register the stroke for use in replication, etc
	if (bStatus && GetOwner()->HasAuthority() && bPerformNetworkReplication)
		GetPaintableMeshData(paintableComponent).AddStroke(Stroke);

	return bStatus;
}

bool UDonMeshPainterComponent::RenderPaintStroke(UObject* PaintableComponent, const FDonPaintStroke& Stroke, bool bSetupDataAndSkipRendering /*= false*/)
{
	if (!PaintableComponent)
		return false;	

	// Setup prerequisites:		
	DonMaterialPaintLayers* paintingSetup = SetupPaintingPrerequisites(PaintableComponent, Stroke);
	if(!paintingSetup || paintingSetup->Num() == 0)
		return false;

	if (bSetupDataAndSkipRendering)
		return true;

	// Render the stroke:
	UMaterialInstanceDynamic* drawMaterial = nullptr;
	if (!Stroke.HasText())
	{
		drawMaterial = GetCachedRenderMaterial(Stroke.StrokeParams.BrushRenderMaterial);
		if (!drawMaterial)
			return false;
	}	
	
	for (auto& kvPair : *paintingSetup)
	{
		const auto materialIndex = kvPair.Key;
		auto& paintLayer = kvPair.Value;

		UE_LOG(LogDonMeshPainting, VeryVerbose, TEXT("Painting material index %d"), materialIndex);

		FVector2D uvCoords;
		bool bUvsResolved = ResolveUVs(paintLayer.UvMethod, PaintableComponent, Stroke, paintLayer.UvChannelInferred, uvCoords);
		if (!bUvsResolved)
			return false;

		const FVector2D effectiveBrushSizeUV = GetEffectiveBrushSizeWS(PaintableComponent, Stroke.BrushSizeWS, paintLayer.UvMethod, Stroke.StrokeParams.BrushDecalTexture != nullptr);
		const float effectiveBrushSizeAverage = (effectiveBrushSizeUV.X + effectiveBrushSizeUV.Y) / 2.f; // Eventually, non-uniform textures (eg; 512x1024 will need to be supported per-mesh-per-material)
		
		bool bPositionsTextureAvailable = false;
		bool bBrushDecalTextureAvailable = Stroke.NeedsDecalTexture();

		// Positions Texture based UV painting requires additional parameters to be setup:
		if (paintLayer.UvMethod == EDonUvAcquisitionMethod::UnwrappedPositionsTexture)
		{
			auto primitiveComponent = Cast<UPrimitiveComponent>(PaintableComponent);
			if (!primitiveComponent)
			{
				LogError_MeshNeededForPositionBasedPainting(PaintableComponent);
				return false;
			}

			// Generate/Refresh positions texture if necessary:
			const bool bNeedsToRefreshPositions = IsPositionsCapturePending(paintLayer) || IsPositionsTextureStale(PaintableComponent, paintLayer);
			if (bNeedsToRefreshPositions)
			{
				const bool bPopulateCache = Stroke.NeedsDecalOrTextProjection();
				LoadPositionsTexture(paintLayer, Cast<UPrimitiveComponent>(PaintableComponent), materialIndex, paintLayer.UvChannelInferred, bPopulateCache);
			}

			// Validate positions texture:
			if (!paintLayer.PositionsTexture)
			{
				LogError_UnwrappedPositionsTextureNotFound(primitiveComponent);
				return false;
			}

			bPositionsTextureAvailable = true;

			// For stamping decal textures we need knowledge of the exact U, V coordinates:
			const FVector localPosition = primitiveComponent->GetComponentTransform().InverseTransformPosition(Stroke.WorldLocation);
			if (Stroke.NeedsDecalOrTextProjection())
			{
				bool bFoundUVs = GlobalPainter()->UvFromCachedPositionsTexture(uvCoords, paintLayer.PositionsTexture, paintLayer.PositionsTextureReferenceBounds, localPosition, primitiveComponent, materialIndex);
				if (!bFoundUVs)
					continue;
			}

			if (drawMaterial)
			{
				drawMaterial->SetTextureParameterValue(DonMeshPaint::P_LocalPositionsTexture, paintLayer.PositionsTexture);
				drawMaterial->SetVectorParameterValue(DonMeshPaint::P_LocalPosition, localPosition);

				const FBoxSphereBounds& bounds = paintLayer.PositionsTextureReferenceBounds;
				drawMaterial->SetVectorParameterValue(DonMeshPaint::P_BoundsMin, bounds.Origin - bounds.BoxExtent);
				drawMaterial->SetVectorParameterValue(DonMeshPaint::P_BoundsMax, bounds.Origin + bounds.BoxExtent);
			}
		}

		if (Stroke.HasText())
		{
			const FVector2D screenPosition = FVector2D(paintLayer.RenderTarget->SizeX * uvCoords.X, paintLayer.RenderTarget->SizeY * uvCoords.Y);
			UDonMeshPaintingHelper::DrawTextToRenderTarget(this, paintLayer.RenderTarget, Stroke.Text, screenPosition, Stroke.BrushDecalRotation, Stroke.BrushColor, Stroke.TextStyle);
		}
		else
		{
			// Common parameters:
			drawMaterial->SetTextureParameterValue(DonMeshPaint::P_BrushDecalTexture, Stroke.StrokeParams.BrushDecalTexture);
			drawMaterial->SetVectorParameterValue(DonMeshPaint::P_BrushColor, Stroke.BrushColor);

			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_EffectiveBrushSizeAveraged, effectiveBrushSizeAverage);
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_EffectiveBrushSizeU, effectiveBrushSizeUV.X);
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_EffectiveBrushSizeV, effectiveBrushSizeUV.Y);
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_IsNonUniformBrush, effectiveBrushSizeUV.X != effectiveBrushSizeUV.Y);

			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_BrushRotation, FMath::Fractional(Stroke.BrushDecalRotation / 360.f));
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_BrushOpacity, Stroke.StrokeParams.BrushOpacity);
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_BrushHardness, Stroke.StrokeParams.BrushHardness);

			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_IsPositionsTextureAvailable, bPositionsTextureAvailable ? 1 : 0);
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_IsBrushDecalTextureAvailable, bBrushDecalTextureAvailable ? 1 : 0);

			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_UVx, uvCoords.X);
			drawMaterial->SetScalarParameterValue(DonMeshPaint::P_UVy, uvCoords.Y); // Note: these coords are not available for positions texture workflow that is not using a decal texture

			if(Stroke.bEraseMode)
			{
				auto renderTargetPrevious = GlobalPainter()->GetRenderTargetFromPool(paintLayer.RenderTarget);
				SyncRenderTargets(paintLayer.RenderTarget, renderTargetPrevious);
				drawMaterial->SetTextureParameterValue(DonMeshPaint::P_RenderTargetPrevious, renderTargetPrevious);				
				UKismetRenderingLibrary::ClearRenderTarget2D(this, paintLayer.RenderTarget, FLinearColor(0, 0, 0, 1));

				ErasePaintCollision(PaintableComponent, Stroke);
			}
			
			UDonMeshPaintingHelper::DrawMaterialToRenderTarget(this, paintLayer.RenderTarget, drawMaterial);			
			
			//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString("Rendered paint stroke!"));
		}

#if WITH_EDITOR
		// Debug aid: Useful for inspecting the brush render material
		GlobalPainter()->CurrentAdditiveBrush = drawMaterial;

		auto paintableAsPrimitive = Cast<UPrimitiveComponent>(PaintableComponent);
		if (paintableAsPrimitive)
			GlobalPainter()->LastPaintedMaterial = paintableAsPrimitive->GetMaterial(materialIndex);
#endif // WITH_EDITOR

	}

	return true;	
}

void UDonMeshPainterComponent::ErasePaintCollision(UObject* PaintableComponent, const FDonPaintStroke& Stroke)
{
	const auto worldLocation = Stroke.WorldLocation;
	DonMeshCellUnit cell = GetMeshCell(PaintableComponent, worldLocation);

	auto& strokesMap = MeshPaintStrokesMap.FindOrAdd(PaintableComponent).FindOrAdd(Stroke.StrokeParams.PaintLayerIndex);
	if (!strokesMap.Contains(cell))
		return; // no strokes in this cell region (for this query layer

	// #DonPixelCollision_CellAggregation 
	// @todo: We should aggregate all neighboring cells as a square grid to prevent misses for very large brush strokes (larger than cell, etc)

	auto& strokes = *strokesMap.Find(cell);

	for(int32 i = strokes.Num() - 1; i >= 0; i-- )
	{
		auto& stroke = strokes[i];
		const float radius = Stroke.BrushSizeWS;
		const float distance = GetDistanceToPaintedCollision(GetPaintableComponentTransform(PaintableComponent), stroke, worldLocation);
		UE_LOG(LogDonMeshPainting, VeryVerbose, TEXT("[Erase] Paint collision distance measured: %f"), distance);

		const bool bWithinRadius = radius > distance;
		if (bWithinRadius)
		{
			strokes.RemoveAt(i);
		}
	}
}

class UMaterialInstanceDynamic* UDonMeshPainterComponent::GetCachedRenderMaterial(class UMaterialInterface* RenderMaterial)
{
	auto globalPainter = GlobalPainter();
	if (!globalPainter)
	{
		ensure(false);		
		LogOrDisplay(FString("Painter component without global actor linkage found!\n"));
		LogOrDisplay(FString("Is your Save System trying to save DoN's Mesh Painting components? If so, please exclude UDonMeshPainterComponent and ADonMeshPaintingGlobalActor from your Save System"));
		return nullptr;
	}

	return globalPainter->GetCachedRenderMaterial(RenderMaterial);
}

void UDonMeshPainterComponent::SyncRenderTargets(UTextureRenderTarget2D* Source, UTextureRenderTarget2D* Destination)
{
	auto saveTextureToRTMaterial = GlobalPainter()->SaveTextureToRTMaterialInstance;
	if (!ensure(saveTextureToRTMaterial))
		return;

	saveTextureToRTMaterial->SetTextureParameterValue(DonMeshPaint::P_TextureToRender, Source);
	UKismetRenderingLibrary::ClearRenderTarget2D(this, Destination, FLinearColor(0, 0, 0, 1)); // vital step!
	UDonMeshPaintingHelper::DrawMaterialToRenderTarget(this, Destination, saveTextureToRTMaterial);
}

bool UDonMeshPainterComponent::IsPositionsTextureStale(const FDonPaintLayer& PaintLayer)
{
	const float timeElapsedSinceLastPositionRefresh = GetWorld()->GetRealTimeSeconds() - PaintLayer.PositionsTextureLastUpdate;

	return timeElapsedSinceLastPositionRefresh > 0.1f; // @todo: parametrize
}

bool UDonMeshPainterComponent::IsPositionsTextureStale(UObject* PaintableComponent, const FDonPaintLayer& PaintLayer)
{
	if (!PaintableComponent->IsA(USkeletalMeshComponent::StaticClass()))
		return false; // Typically, only skeletal meshes animate their positions and need to periodically refresh their positions data

	return IsPositionsTextureStale(PaintLayer);
}

TArray<TArray<FDonBakedPaintMaterials>> UDonMeshPainterComponent::ExtractBakedPaintData(const bool bIsSaveLoadCycle /*= false*/, const bool bExportToFile /*= false*/, FString ExportDirectory /*= FString()*/)
{
	TArray<TArray<FDonBakedPaintMaterials>> bakedPaintPerPrimitive;

	// Outer loop: collection of primitives with paint strokes that we are interested in
	for (auto paintStrokesPerPrimitive : Rep_MeshPaintStrokes)
	{
		auto activePrimitive = paintStrokesPerPrimitive.Paintable.Get();
		if (!ensure(activePrimitive))
			continue;

		if (bIsSaveLoadCycle && !DonMeshPaint::NeedsBakedTextures(activePrimitive))
		{
			bakedPaintPerPrimitive.AddZeroed(); // For save games we don't need baked textures for all entities. Only pose-dependent ones like skinned meshes do...
			continue;
		}
		
		int32 layerIndex = 0;
		TArray<FDonBakedPaintMaterials> bakedPaintLayers;		

		// Loop 1: Iterates on Layers
		for (const auto& layerAggregate : PaintLayersByPaintableMesh)
		{
			FDonBakedPaintMaterials materialsContainer;

			// Loop 2: Iterates on Primitives (for the current layer)
			for (const auto& kvPair : layerAggregate)
			{
				if (kvPair.Key != activePrimitive)  // Skip if the current primitive is not our active primitive (outer loop)
					continue;

				const DonMaterialPaintLayers& paintedMaterials = kvPair.Value;				

				// Loop 3: Iterates on Materials (for the current primitive)
				for (const auto& mKvPair : paintedMaterials)
				{
					const int32 materialIndex = mKvPair.Key;
					const auto& paintLayer = mKvPair.Value;
					auto renderTarget = paintLayer.RenderTarget;					

					const bool bFillAlpha = bExportToFile; // if we're exporting to file it's mostly for visualization, so we fill the alpha up

					FDonBakedPaintPerMaterial bakedPaint;
					bakedPaint.CompressedPaint = UDonMeshPaintingHelper::ExtractRenderTargetAsPNG(this, renderTarget, bExportToFile/*bFillAlpha*/);
					bakedPaint.MaterialIndex = materialIndex;
					materialsContainer.BakedMaterials.Add(bakedPaint);

					// Export if necessary (niche usecase)
					if (bExportToFile && !ExportDirectory.IsEmpty())
					{
						FString filepath = FString::Printf(TEXT("%s/BakedPaintTextures/%s/Material_%d_Layer%d.png"), *ExportDirectory, *activePrimitive->GetName(), materialIndex, layerIndex);
						bool bSuccess = FFileHelper::SaveArrayToFile(bakedPaint.CompressedPaint, *filepath);
					}				

				} // material loop				

				break; // we have located our active primitive and can skip the reset

			} // primitive loop

			bakedPaintLayers.Add(materialsContainer);			
			layerIndex++;

		} // layer loop

		bakedPaintPerPrimitive.Add(bakedPaintLayers); // Adds all the material layers for the active primitive (outer loop)
	} // outer primitive loop
	
	return bakedPaintPerPrimitive;
}

void UDonMeshPainterComponent::LoadBakedPaintData(const TArray<TArray<FDonBakedPaintMaterials>>& Data)
{
	// Note:- It is assumed that Rep_MeshPaintStrokes is loaded/up-to-date at this point
	 int32 primitiveIndex = 0;
	for (auto paintStrokesPerPrimitive : Rep_MeshPaintStrokes)
	{
		auto activePrimitive = paintStrokesPerPrimitive.Paintable.Get();
		if (!activePrimitive)
			continue;		

		// Loop 1: Iterates on Primitives
		//for (const auto& bakedPaintLayers : Data)
		const auto& bakedPaintLayers = Data[primitiveIndex];
		{
			EnsurePaintLayersBySize(bakedPaintLayers.Num());
			int32 layerIndex = 0;
			
			// Loop 2: Iterates on Layers			
			for (const auto& bakedLayer : bakedPaintLayers)
			{
				// Loop 3: Iterates on Materials 
				//int32 materialIndex = 0;
				for (const auto& bakedMaterial : bakedLayer.BakedMaterials)
				{
					const auto& paintLayer = PaintLayersByPaintableMesh[layerIndex].FindOrAdd(activePrimitive).FindOrAdd(bakedMaterial.MaterialIndex);
					auto renderTarget = paintLayer.RenderTarget;
					if (!renderTarget)
						continue;

					IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
					TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

					if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(bakedMaterial.CompressedPaint.GetData(), bakedMaterial.CompressedPaint.Num()))
					{
						TArray<uint8, FDefaultAllocator64> uncompressed;
						if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, uncompressed))
						{
							UTexture2D* texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
							if (!texture)
								continue;

							void* textureData = texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
							FMemory::Memcpy(textureData, uncompressed.GetData(), uncompressed.Num());
							texture->PlatformData->Mips[0].BulkData.Unlock();

							texture->UpdateResource();

							auto saveTextureToRTMaterial = GlobalPainter()->SaveTextureToRTMaterialInstance;
							if (!ensure(saveTextureToRTMaterial))
								continue;

							saveTextureToRTMaterial->SetTextureParameterValue(DonMeshPaint::P_TextureToRender, texture);
							UDonMeshPaintingHelper::DrawMaterialToRenderTarget(this, renderTarget, saveTextureToRTMaterial);
						}
					}

					//materialIndex++;
				}

				layerIndex++;
			}
		}
		primitiveIndex++;
	}
}

void UDonMeshPainterComponent::DonExportPaintMasks(UObject* PaintableEntity, const FString AbsoluteDirectoryPath)
{
	UDonMeshPainterComponent::ExtractBakedPaintData(false/*bIsSaveLoadCycle*/, true /*bExportToFile*/, AbsoluteDirectoryPath);
}

void UDonMeshPainterComponent::LoadPositionsTexture(FDonPaintLayer& PaintTexture, UPrimitiveComponent* PrimitiveComponent, const int32 MaterialIndex, const int32 UvChannelToUse, const bool bPopulateCache)
{
	PaintTexture.PositionsTextureReferenceBounds = UDonMeshPaintingHelper::GetLocalBounds(PrimitiveComponent);
	PaintTexture.PositionsTexture = GlobalPainter()->GetPositionsTextureForMesh(PrimitiveComponent, MaterialIndex, UvChannelToUse, PaintTexture.PositionsTextureReferenceBounds);
	PaintTexture.PositionsTextureLastUpdate = GetWorld()->GetRealTimeSeconds();	

	// For stamping decal textures in a positions texture workflow we need CPU access to the positions; cached for efficiency:
	if(bPopulateCache)
		GlobalPainter()->CachePositionsTexture(PaintTexture.PositionsTexture, PrimitiveComponent, MaterialIndex);
}

void UDonMeshPainterComponent::EnsurePaintLayersBySize(const int32 RequiredSize)
{
	if (PaintLayersByPaintableMesh.Num() < RequiredSize)
		PaintLayersByPaintableMesh.SetNumZeroed(RequiredSize); // guarantee index lookup	
}

DonMaterialPaintLayers* UDonMeshPainterComponent::SetupPaintingPrerequisites(UObject* PaintableComponent, const FDonPaintStroke& Stroke)
{
	// Register the stroke into a cell for rapid queries:
	DonMeshCellUnit cell = GetMeshCell(PaintableComponent, Stroke.WorldLocation); // @todo handle case when stroke radius is greater than world cell size (inject into neighbor cells?)
	MeshPaintStrokesMap.FindOrAdd(PaintableComponent).FindOrAdd(Stroke.StrokeParams.PaintLayerIndex).FindOrAdd(cell).Add(Stroke);

	if (Stroke.ExplicitUvMethod == EDonUvAcquisitionMethod::Unspecified)
	{
		// This is the regular workflow where users pass in primitives and the Uv Method is auto-inferred per material:
		return SetupPaintingPrerequisites_AutoUv(PaintableComponent, Stroke);
	}
	else
	{
		// Special case for context-free World Space painting. Great for driving global effect systems that are not tied to a particular primitive:
		return SetupPaintingPrerequisites_ExplicitUv(PaintableComponent, Stroke);
	}
}

DonMaterialPaintLayers* UDonMeshPainterComponent::SetupPaintingPrerequisites_AutoUv(UObject* PaintableComponent, const FDonPaintStroke& Stroke)
{
	const int32 layerIndex = Stroke.StrokeParams.PaintLayerIndex;
	EnsurePaintLayersBySize(layerIndex + 1); // guarantee index lookup

	// Load from cache:	
	auto setupCache = PaintLayersByPaintableMesh[layerIndex].Find(PaintableComponent);
	if (setupCache)
		return setupCache;

	// Perform one-time setup:
	DonMaterialPaintLayers newSetup;

	auto primitiveComponent = Cast<UPrimitiveComponent>(PaintableComponent);
	if (primitiveComponent)
	{
		const bool bExistingRenderTargetIsCompulsory = false;

		for (int32 i = 0; i < primitiveComponent->GetNumMaterials(); i++)
		{
			EDonUvAcquisitionMethod uvMethod;
			int32 uvChannel = 0;
			auto renderTarget = SetupRenderTarget(PaintableComponent, Stroke, primitiveComponent->GetMaterial(i), i, bExistingRenderTargetIsCompulsory, uvMethod, uvChannel);			
			if (!renderTarget && !IsRunningDedicatedServer())
				continue; // this material probably isn't configured for mesh painting.

			newSetup.Add(i, FDonPaintLayer(renderTarget, uvMethod, uvChannel));
		}
	}
	else // Landscape
	{
		const bool bExistingRenderTargetIsCompulsory = true;

		auto landscape = Cast<ALandscape>(PaintableComponent);
		if (!landscape)
		{
			LogError_HitContainsNoPaintableObject(PaintableComponent);
			return nullptr;
		}

		EDonUvAcquisitionMethod uvMethod;
		int32 uvChannel = 0;
		auto renderTarget = SetupRenderTarget(landscape, Stroke, landscape->LandscapeMaterial, 0, bExistingRenderTargetIsCompulsory, uvMethod, uvChannel);
		if (!renderTarget && !IsRunningDedicatedServer())
			return nullptr;

		newSetup.Add(0, FDonPaintLayer(renderTarget, uvMethod, 0));
	}

	if (newSetup.Num() == 0)
		return nullptr; // Found no paintable materials in this primitive! Instead of polluting our cache and index lookup, just return at this point.

	// Populate Cache and return a pointer. We won't be reallocating the map for the duration of this call, so it is safe to use downstream.
	return &(PaintLayersByPaintableMesh[layerIndex].Add(PaintableComponent, newSetup));
}

DonMaterialPaintLayers* UDonMeshPainterComponent::SetupPaintingPrerequisites_ExplicitUv(UObject* PaintableComponent, const FDonPaintStroke& Stroke)
{
	// Right now, context-free world space painting is the only usecase for explicit UVs. So if we don't find a valid world space axes, just return...
	const EDonUvAxes worldAxes = DonMeshPaint::WorldUvAxesFromUvMethod(Stroke.ExplicitUvMethod);
	if (worldAxes == EDonUvAxes::Invalid)
		return nullptr;

	const int32 layerIndex = Stroke.StrokeParams.PaintLayerIndex;

	auto setupCache = PaintLayersByExplicitUvMethod.Find(Stroke.ExplicitUvMethod);
	if (setupCache)
		return setupCache;

	// One-time load:
	auto renderTarget = GlobalPainter()->LoadWorldSpacePaintingMetadata(worldAxes, layerIndex);
	if (!renderTarget)
		return nullptr;

	RegisterRenderTarget(renderTarget);

	DonMaterialPaintLayers explicitUvSetup;
	explicitUvSetup.Add(0, FDonPaintLayer(renderTarget, Stroke.ExplicitUvMethod, layerIndex));

	return &PaintLayersByExplicitUvMethod.Add(Stroke.ExplicitUvMethod, explicitUvSetup);
}

class UTextureRenderTarget2D* UDonMeshPainterComponent::SetupRenderTarget(UObject* PaintableComponent, const FDonPaintStroke& Stroke, UMaterialInterface* Material, const int32 MaterialIndex,  bool bExistingRenderTargetIsCompulsory, EDonUvAcquisitionMethod& OutUvMethod, int32& OutUvChannel)
{
	if (!Material)
		return nullptr;

	// 1. First acquire the texture parameter corresponding to our paint layer. Also infer UV method, UV channel, etc.
	FName paintLayer;
	auto existingRenderTarget = ResolvePaintLayer(PaintableComponent, Stroke, Material, MaterialIndex, paintLayer, OutUvMethod, OutUvChannel);
	if (!existingRenderTarget)
		return nullptr;

	if (IsRunningDedicatedServer())
		return nullptr;
	
	// 2. Now we can start setting up the Render Target itself
	// Read existing Render Target (if available) to infer user-set metadata
	int32 TextureSizeX = DefaultTextureSizeX, TextureSizeY = DefaultTextureSizeY;	
	if (existingRenderTarget)
	{
		TextureSizeX = existingRenderTarget->SizeX;
		TextureSizeY = existingRenderTarget->SizeY;
	}

	UTextureRenderTarget2D* renderTargetToUse = nullptr;
	bExistingRenderTargetIsCompulsory = bExistingRenderTargetIsCompulsory || DonMeshPaint::UvMethodIsPlanarWorldSapce(OutUvMethod);

	if(bExistingRenderTargetIsCompulsory)
	{
		if (existingRenderTarget != nullptr)
			renderTargetToUse = existingRenderTarget;
		else
		{
			LogError_PreExistingRenderTargetNotFound(Material, Stroke.StrokeParams.PaintLayerIndex);
			return nullptr;
		}			
	}
	else
	{
		if (!existingRenderTarget)
		{
			UE_LOG(LogDonMeshPainting, Warning, TEXT("Unable to find default texture for \"%s\" to infer desired texture size. Using default size %dx%d for painting"), *paintLayer.ToString(), TextureSizeX, TextureSizeY);
		}

		// Create new render target:
		renderTargetToUse = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), UTextureRenderTarget2D::StaticClass());
		renderTargetToUse->InitAutoFormat(TextureSizeX, TextureSizeY);
		if (existingRenderTarget)
			renderTargetToUse->RenderTargetFormat = existingRenderTarget->RenderTargetFormat;

		renderTargetToUse->UpdateResourceImmediate(); // without this, the first stroke is never rendered. Took a lot of debugging to figure this out :)

		auto primitiveComponent = Cast<UPrimitiveComponent>(PaintableComponent);
		ensure(primitiveComponent);
		if (primitiveComponent)
		{
			auto dynamicMaterial = GetActiveDynamicMaterial(primitiveComponent, MaterialIndex, paintLayer);
			ensure(dynamicMaterial);
			if (dynamicMaterial)
			{
				const auto& bounds = UDonMeshPaintingHelper::GetLocalBounds(primitiveComponent);

				dynamicMaterial->SetTextureParameterValue(paintLayer, renderTargetToUse);
				dynamicMaterial->SetVectorParameterValue(DonMeshPaint::P_DonLocalBoundsMin, bounds.Origin - bounds.BoxExtent);
				dynamicMaterial->SetVectorParameterValue(DonMeshPaint::P_DonLocalBoundsMax, bounds.Origin + bounds.BoxExtent);
				
			}
		}		
	}

	RegisterRenderTarget(renderTargetToUse);

	return renderTargetToUse;
}

void UDonMeshPainterComponent::RegisterRenderTarget(UTextureRenderTarget2D* RenderTargetToUse)
{
	ActiveRenderTargets.Add(RenderTargetToUse);
	ClearRenderTarget(RenderTargetToUse);
}

class UTextureRenderTarget2D* UDonMeshPainterComponent::ResolvePaintLayer(UObject* PaintableComponent, const FDonPaintStroke& Stroke, UMaterialInterface* Material, const int32 MaterialIndex, FName& OutPaintLayer, EDonUvAcquisitionMethod& OutUvMethod, int32& OutUvChannel)
{
	bool bRenderTargetParameterFound = false;
	UTexture* existingTexture = nullptr;

	const int32 layerIndex = Stroke.StrokeParams.PaintLayerIndex;
	const bool bNeedsUVProjection = Stroke.NeedsDecalOrTextProjection();

	// Analyze the user's material setup to auto-detect the desired UV method
	for (const FName layerNameByUvMethod : DonMeshPaint::PaintLayersByUvMethod)
	{
		FName layerParameterName;

		// A) Mesh UV Painting  (UV Channel dependent):
		//
		if (layerNameByUvMethod.IsEqual(DonMeshPaint::DonPaintLayer_MeshGeneric))
		{
			for (OutUvChannel = 0; OutUvChannel < MaxUvChannelsSupported; OutUvChannel++)
			{
				bRenderTargetParameterFound = QueryMaterialPaintLayerByName(Material, layerNameByUvMethod, layerIndex, OutUvChannel, existingTexture, layerParameterName);
				if (bRenderTargetParameterFound)
					break; // layer found!
			}
		}
		// B) World/Local Space painting (UV Channel independent)
		else 
		{
			const int32 uvChannelIndex = -1;
			bRenderTargetParameterFound = QueryMaterialPaintLayerByName(Material, layerNameByUvMethod, layerIndex, uvChannelIndex, existingTexture, layerParameterName);
		}

		// If we found the layer, resolve its UV acquisition method:
		if (bRenderTargetParameterFound)
		{
			OutPaintLayer = layerParameterName; // resolved!
			OutUvMethod = ResolveUvAcquisitionMethod(layerNameByUvMethod, PaintableComponent, MaterialIndex, Stroke.FaceIndex, bNeedsUVProjection, OutUvChannel);
			break;
		}
	}

	if (!bRenderTargetParameterFound)
	{
		LogWarning_MaterialNotSetupForPainting(Material, layerIndex);
		return nullptr; // this material is not configured for painting!
	}

	auto renderTargetTemplate = Cast<UTextureRenderTarget2D>(existingTexture);
	if (!renderTargetTemplate)
	{
		LogWarning_MaterialExcludedFromPainting(Material, layerIndex);
		return nullptr; // this material has been intentionally excluded from painting (eg: A master material may have a paint layer that a child material simply isn't interested in using, so optimize out)
	}

	// Validate Uv Method result:
	if (PaintableComponent->IsA(ALandscape::StaticClass()) && !DonMeshPaint::UvMethodIsPlanarWorldSapce(OutUvMethod))
	{
		LogWarning_LandscapesMustUsePlanarWsUvs(OutUvMethod);
		OutUvMethod = EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY; // Force it to use planr WS and warn the user:
	}

	return renderTargetTemplate;
}

bool UDonMeshPainterComponent::QueryMaterialPaintLayerByName(UMaterialInterface* Material, const FName LayerNamePrefix, const int32 LayerIndex, const int32 UvChannel, UTexture*& OutTexture, FName& OutLayerParameterName)
{
	const FString uvChannelToken = UvChannel == -1 ? FString("_") : FString::Printf(TEXT("_UV%d_"), UvChannel);
	const FString layerAsString = FString::Printf(TEXT("%s%s%d"), *LayerNamePrefix.ToString(), *uvChannelToken, LayerIndex);
	OutLayerParameterName = FName(*layerAsString);

	return Material->GetTextureParameterValue(OutLayerParameterName, OutTexture);
}

void UDonMeshPainterComponent::ClearRenderTarget(class UTextureRenderTarget2D* RenderTarget)
{
	if(RenderTarget)
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, FLinearColor(0, 0, 0, 1));
}

void UDonMeshPainterComponent::ClearAllRenderTargets()
{
	for (auto renderTarget : ActiveRenderTargets)
		ClearRenderTarget(renderTarget);
}

void UDonMeshPainterComponent::FlushMaterials(TArray<UMaterialInterface*> MaterialsToFlush)
{
	for (const auto& layer : PaintLayersByPaintableMesh) // loop 1: Layer aggregates
	{
		for (const auto& kvPair : layer) // loop 2: primitives per layer
		{
			UPrimitiveComponent* primitiveComponent = Cast<UPrimitiveComponent>(kvPair.Key);
			if (!primitiveComponent)
				continue;

			for (int32 i = 0; i < primitiveComponent->GetNumMaterials(); i++)
				if (MaterialsToFlush.Contains(primitiveComponent->GetMaterial(i)))
				{
					if (!kvPair.Value.Contains(i))
						continue;

					ClearRenderTarget(kvPair.Value.Find(i)->RenderTarget);
				}
		}
	}
}


UMaterialInstanceDynamic* UDonMeshPainterComponent::GetActiveDynamicMaterial(UPrimitiveComponent* MeshComponent, const int32 MaterialIndex, const FName TextureName)
{
	UTexture* texture;
	UMaterialInterface* material = MeshComponent->GetMaterial(MaterialIndex);
	bool bParamFound = material->GetTextureParameterValue(TextureName, texture);
	if (!bParamFound)
		return nullptr;

	UMaterialInstanceDynamic* dynamicMaterial = nullptr;
	if (material->IsA(UMaterialInstanceDynamic::StaticClass()))
		return Cast<UMaterialInstanceDynamic>(material);
	else
		return MeshComponent->CreateDynamicMaterialInstance(MaterialIndex);
}

bool UDonMeshPainterComponent::QueryPaintCollisionMulti(UPrimitiveComponent* Primitive, const TArray<FDonPaintCollisionQuery>& CollisionTags, FDonPaintCollisionQuery& OutCollisionTag, const FVector WorldLocation)
{
	const bool bWarnIfShapeCapsuleEncountered = false;
	const bool bAllowContextFreeCollisionGathering = true;
	UObject* paintableComponent = ResolvePaintableEntity(Primitive, bAllowContextFreeCollisionGathering, bWarnIfShapeCapsuleEncountered);
	if (!paintableComponent)
		return false;

	DonMeshCellUnit cell = GetMeshCell(paintableComponent, WorldLocation);
	float closestDistanceFound = -1;

	for (const FDonPaintCollisionQuery& query : CollisionTags)
	{
		const auto& strokesMap = MeshPaintStrokesMap.FindOrAdd(paintableComponent).FindOrAdd(query.Layer);
		if (!strokesMap.Contains(cell))
			continue; // no strokes in this cell region (for this query layer)

		for (const auto& stroke : *strokesMap.Find(cell))
		{
			// Filter by collision tag allowing users to perform complex gameplay decisions based on what was painted!
			const FName currentCollisionTag = stroke.StrokeParams.CollisionTag;
			if (currentCollisionTag.IsNone())
				continue;

			// Check if the current stroke has a matching collision tag:
			if (query.CollisionTag.IsEqual(currentCollisionTag))
			{
				const float radius = stroke.BrushSizeWS;
				if (radius < query.MinimumPaintBlobSize)
					continue; // not of interest to us

							  //@todo - Paint Blob aggregation feature: Aggregate collision of neighboring paint strokes allowing players to accummulate tiny paint blobs into huge ones :D

				const float distance = GetDistanceToPaintedCollision(Primitive, stroke, WorldLocation);
				UE_LOG(LogDonMeshPainting, VeryVerbose, TEXT("Paint collision distance measured: %f"), distance);

				// There is an edge-case here where the querier is on a surface whose axis is very different (eg: perpendicular) to the paint stroke.
				// In this case the distance calculated (along the UV axis) or the brush radius doesn't represent the actual proximity. Currently this is considered OK because the two main usecases involve:
				// a) querier lies on the same surface as paint (eg: lava mines on landscape) b) query is invoked only upon hit (eg: projectile hitting a wall with a portal).
				//
				const float effectiveRadius = radius * query.CollisionInflationScale * stroke.StrokeParams.CollisionInflationScale + query.CollisionInflationInAbsoluteUnits;
				const bool bWithinRadius = effectiveRadius > distance;

				if (bWithinRadius)
				{
					OutCollisionTag = query;
					return true;
				}
				else
					closestDistanceFound = closestDistanceFound == -1 ? distance : FMath::Min(closestDistanceFound, distance);

				// continue on to the next stroke..
			}

		} // stroke iteration loop

	} // query iteration loop

	// (Debug log) // if (closestDistanceFound != -1) GEngine->AddOnScreenDebugMessage(-1, 100.f, FColor::White, FString::Printf(TEXT("Closest distance found: %f"), closestDistanceFound));

	return false;
}

float UDonMeshPainterComponent::GetDistanceToPaintedCollision(UPrimitiveComponent* Primitive, const FDonPaintStroke& Stroke, FVector WorldLocationToSample)
{
	return GetDistanceToPaintedCollision(Primitive->GetComponentTransform(), Stroke, WorldLocationToSample);

}

float UDonMeshPainterComponent::GetDistanceToPaintedCollision(FTransform Transform, const FDonPaintStroke& Stroke, FVector WorldLocationToSample)
{
	EDonUvAcquisitionMethod uvMethod = GetUvMethodForDistanceCheck(Stroke);

	// For local-space paint blob collisions:
	const FTransform componentToWorld = Transform;
	const FVector collisionLocationLS = componentToWorld.InverseTransformPositionNoScale(WorldLocationToSample);
	const FVector strokeLocationLS = componentToWorld.InverseTransformPositionNoScale(Stroke.WorldLocation);

	float distance;

	switch (uvMethod)
	{

	// Mesh Space:
	case EDonUvAcquisitionMethod::UnwrappedPositionsTexture:
	case EDonUvAcquisitionMethod::ByCollisionUVs:
		distance = FVector::Distance(WorldLocationToSample, Stroke.WorldLocation);
		break;

	// World Space:
	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY:
		distance = FVector2D::Distance(FVector2D(WorldLocationToSample.X, WorldLocationToSample.Y), FVector2D(Stroke.WorldLocation.X, Stroke.WorldLocation.Y));
		break;

	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ:
		distance = FVector2D::Distance(FVector2D(WorldLocationToSample.X, WorldLocationToSample.Z), FVector2D(Stroke.WorldLocation.X, Stroke.WorldLocation.Z));
		break;

	case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ:
		distance = FVector2D::Distance(FVector2D(WorldLocationToSample.Y, WorldLocationToSample.Z), FVector2D(Stroke.WorldLocation.Y, Stroke.WorldLocation.Z));
		break;

	// Local Space:
	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXY:
		distance = FVector2D::Distance(FVector2D(collisionLocationLS.X, collisionLocationLS.Y), FVector2D(strokeLocationLS.X, strokeLocationLS.Y));
		break;	

	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXZ:
		distance = FVector2D::Distance(FVector2D(collisionLocationLS.X, collisionLocationLS.Z), FVector2D(strokeLocationLS.X, strokeLocationLS.Z));
		break;

	case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceYZ:
		distance = FVector2D::Distance(FVector2D(collisionLocationLS.Y, collisionLocationLS.Z), FVector2D(strokeLocationLS.Y, strokeLocationLS.Z));
		break;	

	default:
		distance = FVector::Distance(WorldLocationToSample, Stroke.WorldLocation);		
	}

	return distance;
}

EDonUvAcquisitionMethod UDonMeshPainterComponent::GetUvMethodForDistanceCheck(const FDonPaintStroke& Stroke)
{
	if (Stroke.ExplicitUvMethod == EDonUvAcquisitionMethod::Unspecified)
	{
		// Note:- For implicit UVs, paint collision only supports a single Uv method across an entire actor. This vastly simplifies the setup and improves performance.
		const int32 layerIndex = Stroke.StrokeParams.PaintLayerIndex;
		for (const auto& paintableKvPair : PaintLayersByPaintableMesh[layerIndex]) // guaranteed indices
			for (const auto& kvPair : paintableKvPair.Value)
				return kvPair.Value.UvMethod;

		return EDonUvAcquisitionMethod::UnwrappedPositionsTexture; // default
	}
	else
		return Stroke.ExplicitUvMethod;
}

bool UDonMeshPainterComponent::IsMeshPaintedAt(UObject* PaintableComponent, FVector WorldLocation, float DistanceFactorRequired, int32 Layer)
{
	DonMeshCellUnit cell = GetMeshCell(PaintableComponent, WorldLocation);

#if DON_MESH_CELL_DEBUGGING_ENABLED
	DebugMeshCell(cell);
#endif // DON_MESH_CELL_DEBUGGING_ENABLED

	const auto& strokesMap = MeshPaintStrokesMap.FindOrAdd(PaintableComponent).FindOrAdd(Layer);

	if (!strokesMap.Contains(cell))
		return false; // no strokes in this cell region

	for (const auto& meshHole : *strokesMap.Find(cell))
	{
		const float distance = FVector::Distance(WorldLocation, meshHole.WorldLocation);
		const float radius = meshHole.BrushSizeWS;
		const bool bWithinRadius = (distance / radius) < DistanceFactorRequired;

#if DON_MESH_CELL_DEBUGGING_ENABLED
		DebugMeshHole(bWithinRadius, distance, radius, meshHole);
#endif // DON_MESH_CELL_DEBUGGING_ENABLED

		if (bWithinRadius)
			return true;
	}

	return false;
}

void UDonMeshPainterComponent::DebugMeshHole(bool bWithinRadius, const float distance, const float radius, const FDonPaintStroke &meshHole)
{
	UE_LOG(LogTemp, Warning, TEXT("Punctured: %d, Distance (%f), Radius (%f) "), bWithinRadius, distance, radius);
	DrawDebugSphere(GetWorld(), meshHole.WorldLocation, meshHole.BrushSizeWS, 16, bWithinRadius ? FColor::Yellow : FColor::White, true);
}

void UDonMeshPainterComponent::DebugMeshCell(UObject* PaintableComponent, const DonMeshCellUnit &Cell)
{
	if (!PaintableComponent)
		return;

	const FTransform transform = GetPaintableComponentTransform(PaintableComponent);
	FVector cellCentreWS = transform.TransformPosition(MeshCellSize * FVector(Cell.X, Cell.Y, Cell.Z));
	float extent = MeshCellSize * GetPaintableComponentScaleDivisor(PaintableComponent).X;
	DrawDebugBox(GetWorld(), cellCentreWS, FVector(extent, extent, 1.f), FColor::Red, true);
}

// Networking:
void UDonMeshPainterComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDonMeshPainterComponent, Rep_MeshPaintStrokes);

}

void UDonMeshPainterComponent::DestroyComponent(bool bPromoteChildren /*= false*/)
{
	Super::DestroyComponent(bPromoteChildren);	
}

void UDonMeshPainterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

FDonPaintableMeshData& UDonMeshPainterComponent::GetPaintableMeshData(UObject* Paintable)
{
	for (auto& meshData : Rep_MeshPaintStrokes)
	{
		if (Paintable == meshData.Paintable.Get())
			return meshData;
	}

	// Create new:
	Rep_MeshPaintStrokes.Add(FDonPaintableMeshData(Paintable, GetPaintableComponentTransform(Paintable)) );

	return Rep_MeshPaintStrokes.Last();
}

FTransform UDonMeshPainterComponent::GetPaintableComponentTransform(UObject* PaintableComponent)
{
	auto primitiveComponent = Cast<UPrimitiveComponent>(PaintableComponent);
	if (primitiveComponent)
		return primitiveComponent->GetComponentTransform();
	else
	{
		auto paintableActor = Cast<AActor>(PaintableComponent); // this is either the Landscape Actor (Landscape painting workflow) or the Global Painter Actor (Paint World Direct workflow)
		if (paintableActor)
			return paintableActor->GetActorTransform();
	}

	ensure(false); // No known usecases have the ability to enter this branch.

	return FTransform::Identity;
}

void UDonMeshPainterComponent::SetPaintableComponentTransform(UObject* PaintableComponent, const FTransform& Transform)
{
	auto primitiveComponent = Cast<UPrimitiveComponent>(PaintableComponent);
	if (primitiveComponent)
		primitiveComponent->SetWorldTransform(Transform);
	else
	{
		auto paintableActor = Cast<AActor>(PaintableComponent);
		if (paintableActor)
			paintableActor->SetActorTransform(Transform);
	}
}

FVector UDonMeshPainterComponent::GetPaintableComponentScaleDivisor(UObject* PaintableComponent)
{
	auto primitiveComponent = Cast<UPrimitiveComponent>(PaintableComponent);
	if (primitiveComponent)
		return primitiveComponent->GetComponentScale();
	else
		return FVector(1, 1, 1); // Rationale: Landscapes use position based UVs so we don't need to derive the divisor from its actual world transform
}

//
//
// ~~~ Logging code follows: ~~~
//
// All code below is used purely for logging purposes and has no functional impact.
// Some logs perform memorization behavior that greatly improve the user experience, while adding some complexity to the code. 
// These have been explicitly surrounded with #if !UE_BUILD_SHIPPING to be extra-sure that they're optimized out by the compiler.
//

void UDonMeshPainterComponent::LogOrDisplay(FString Message)
{
	if (bDisplayLogWarningsOnScreen)
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Magenta, Message);
	else
		UE_LOG(LogDonMeshPainting, Error, TEXT("%s"), *Message);
}

void UDonMeshPainterComponent::LogError_HitUVMissing(UObject* PaintableComponent)
{
#if !UE_BUILD_SHIPPING
	if (IsPawnWithKnownSetupIssues(PaintableComponent))
		return;
#endif // UE_BUILD_SHIPPING

	FString message = FString("\nUV data missing in Hitresult, Unable to paint Mesh. Please check the following:\n");
	message += "Ensure that \"TraceComplex\" is enabled for your Hitresult query\n";
	message += "Ensure that \"ReturnFaceIndex\" is set to true in the collision query params in your Hit trace/sweep query (this is automatically set for Blueprint users)\n";

	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogWarning_CollisionUVSupportNotAvailable(UObject* PaintableComponent)
{
#if !UE_BUILD_SHIPPING
	if (IsPawnWithKnownSetupIssues(PaintableComponent))
		return;
#endif // UE_BUILD_SHIPPING

	// The code below should be optimized out of shipping builds in any case, as they're purely log/GEngine debug statements
	auto primitive = Cast<UPrimitiveComponent>(PaintableComponent);
	FString message = FString::Printf(TEXT("\nCollision UV support is not enabled! Generating runtime positions texture for %s to obtain UV data...\n"), primitive ? *primitive->GetName() : *FString());
	message += FString("Tip: For static meshes that need decal projection, it is recommended to 1. Turn Collision UVs on under Project->Physics Settings-> Support UV From Hit Results");
	message += FString("and 2. Turn \"Trace Complex\" on.\n");
	message += FString("This enables a slightly more efficient workflow for static meshes.\n");

	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogWarning_TryingToPaintOnACollisionCapsule(APawn* Pawn, UObject* PaintableComponent)
{
#if !UE_BUILD_SHIPPING
	if (IsPawnWithKnownSetupIssues(Pawn))
		return;

	FString message = FString::Printf(TEXT("\n[Alert] %s \n"), Pawn ? *Pawn->GetName() : *FString());
	message += FString::Printf(TEXT("You're trying to paint on a Pawn's Collision capsule (%s) instead of its mesh!\n"), PaintableComponent ? *PaintableComponent->GetName() : *FString());
	message += "This will yield very low accuracy results unless your collision capsule perfectly encapsulates your mesh.\n";
	message += "For best results, run trace queries against your _mesh_ rather than the collision component.\n";
	LogOrDisplay(message);

	LogMemorize_PawnWithSetupIssue(Pawn);	
#endif // UE_BUILD_SHIPPING
}

void UDonMeshPainterComponent::LogWarning_MaterialNotSetupForPainting(UMaterialInterface* Material, const int32 LayerIndex)
{
#if !UE_BUILD_SHIPPING
	if (IsMaterialWithKnownSetupIssues(Material))
		return; // We've already warned the user about this Material's setup.

	FString message = FString::Printf(TEXT("\n[Alert] %s - Layer %d missing\n"), Material ? *Material->GetName() : TEXT(""), LayerIndex);
	message += FString::Printf(TEXT("This material is not configured for painting on layer %d.\nIf you need to paint on this mateiral then add the correct Don paint layer material function inside your material\nOtherwise, ignore this message"), LayerIndex);
	LogOrDisplay(message);

	LogMemorize_MaterialWithSetupIssue(Material);	
#endif // UE_BUILD_SHIPPING
}

void UDonMeshPainterComponent::LogWarning_MaterialExcludedFromPainting(UMaterialInterface* Material, const int32 LayerIndex)
{
#if !UE_BUILD_SHIPPING
	if (IsMaterialWithKnownSetupIssues(Material))
		return; // We've already warned the user about this Material's setup.

	FString message = FString::Printf(TEXT("\n[Info] %s - Material excluded for paint layer %d\n"), Material ? *Material->GetName() : TEXT(""), LayerIndex);
	message += FString::Printf(TEXT("This material has been intentionally excluded from painting as the default paramter for paint layer %d is empty.\n"), LayerIndex);
	message += "Empty defaults are used as a way of excluding certain materials; eg: a Master material may have paint layers that a child material isn't interested in using.";
	LogOrDisplay(message);

	LogMemorize_MaterialWithSetupIssue(Material);
#endif // UE_BUILD_SHIPPING
}
void UDonMeshPainterComponent::LogWarning_LandscapesMustUsePlanarWsUvs(const EDonUvAcquisitionMethod RequestedUvMethod)
{
#if !UE_BUILD_SHIPPING
	if (GlobalPainter() && GlobalPainter()->bLandscapeUvMethodWarningShown)
		return; // We have already alerted the user about this!

	FString requestedUVMethodString;
	UEnum::GetValueAsString(TEXT("/Script/DonMeshPainting.EDonUvAcquisitionMethod"), RequestedUvMethod, requestedUVMethodString);

	FString message = FString("\n[Alert] Landscapes must use world-space planar mapping for painting.\n");
	message += FString::Printf(TEXT("Your requested UV method of \"%s\" is incompatible has been automatically switched to WS planar to facilitate painting.\n"), *requestedUVMethodString);
	LogOrDisplay(message);

	if (GlobalPainter())
		GlobalPainter()->bLandscapeUvMethodWarningShown = true;
#endif // UE_BUILD_SHIPPING
}

void UDonMeshPainterComponent::LogError_PlanarWorldSpaceUVMissing(UObject* PaintableComponent)
{
	auto primitive = Cast<UPrimitiveComponent>(PaintableComponent);
	FString message = FString::Printf(TEXT("\nPlanar UV acquisition failed for %s\n"), primitive ? *primitive->GetName() : *FString());
	message += "Ensure that you have setup \"World Min\" and \"World Max\" values in the Material Parameter Collection \"MPC_GlobalMeshPainting\" to fully encapsulate your paintable world\n";

	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogError_HitContainsNoPaintableObject(UObject* PaintableComponent)
{
	FString message;
	
	if(PaintableComponent == nullptr)
		message = FString("\nMesh Painter found no paintable mesh, landscape or primitive in this Hit. Unable to paint.\n");
	else
		message = FString::Printf(TEXT("Mesh Painter found %s in this Hit instead of a paintable mesh, landscape or primitive. Unable to paint.\n"), *PaintableComponent->GetName());

	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogError_MeshNeededForPositionBasedPainting(UObject* PaintableComponent)
{
	auto primitive = Cast<UPrimitiveComponent>(PaintableComponent);
	FString message = FString::Printf(TEXT("\nPainting via positions texture (i.e. without using collision UV) needs a skeletal or static mesh component.\nInstead found: %s. Unable to paint.\n"), primitive ? *primitive->GetName() : *FString());
	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogError_UnwrappedPositionsTextureNotFound(UObject* PaintableComponent)
{
	auto primitive = Cast<UPrimitiveComponent>(PaintableComponent);
	FString message = FString::Printf(TEXT("\nUnwrapped positions texture not found for %s. Unable to paint with the chosen UV acquisition method.\n"), primitive ? *primitive->GetName() : *FString());
	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogError_PreExistingRenderTargetNotFound(UMaterialInterface* Material, const int32 LayerIndex)
{
	FString message = FString::Printf(TEXT("\n[Alert] %s Setup incomplete!\n"), *Material->GetName());
	message += FString("For world-space painting it is compulsory to supply a pre-existing Render Target as the Landscape actor's material cannot be modified at run-time.");
	message += FString::Printf(TEXT("\nPlease create a new Render Target asset and assign it to the texture object used in paint layer \"%d\".\n"), LayerIndex);

	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogError_PaintStrokeOutsideGlobalBounds()
{
	FString message = "Attempting to paint outside global bounds! If you need to paint here you will need to open MPC_GlobalMeshPainting and increase the bounds to fully encapsulate your paintable world\n";

	LogOrDisplay(message);
}

void UDonMeshPainterComponent::LogError_GlobalBoundsNotSetup()
{
#if !UE_BUILD_SHIPPING
	FString message = "\nWorld bounds setup missing or incomplete!\nPlease open the material parameter collection asset \"MPC_GlobalMeshPainting\" and set your \"World Min\" and \"World Max\" bounds to fully encapsulate your paintable world.\n";
	message += "For painting in world-space this setup is needed as your UVs for painting are generated using these bounds.\n";

	DrawDebugPaintableBounds();

	LogOrDisplay(message);
#endif // UE_BUILD_SHIPPING
}

void UDonMeshPainterComponent::LogError_CollisionUVSupportNotAvailable(UObject* PaintableComponent)
{
#if !UE_BUILD_SHIPPING
	if (IsPawnWithKnownSetupIssues(PaintableComponent))
		return;
#endif // UE_BUILD_SHIPPING

	// The code below should be optimized out of shipping builds in any case, as they're purely log/GEngine debug statements
	auto primitive = Cast<UPrimitiveComponent>(PaintableComponent);
	FString message = FString::Printf(TEXT("\nFound neither a Lightmap UV nor Collision UV support. Unable to paint texture for %s...\n"), primitive ? *primitive->GetName() : *FString());
	message += FString("Tip: Either enable collision UV support (under Project->Physics Settings-> Support UV From Hit Results)\n");
	message += FString("or add a lightmap UV and use the Don_Mesh_Paint_UV1 node.\n");

	LogOrDisplay(message);
}

#if !UE_BUILD_SHIPPING
void UDonMeshPainterComponent::DrawDebugPaintableBounds()
{
	FVector worldMin, worldMax;
	bool bAcquiredWorldBounds = UDonMeshPaintingHelper::GetPaintableWorldBounds(this, worldMin, worldMax, EDonUvAxes::XY);
	if (!bAcquiredWorldBounds)
		return;

	if (GlobalPainter() && !GlobalPainter()->bWorldBoundsDebugBoxShown)
	{
		FVector origin = (worldMax + worldMin) / 2;
		DrawDebugBox(GetWorld(), origin, worldMax - origin, FColor::Red, true, -1, 0, 100.f);
		GlobalPainter()->bWorldBoundsDebugBoxShown = true;
	}
}
#endif // UE_BUILD_SHIPPING
