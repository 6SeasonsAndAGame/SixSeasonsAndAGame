// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "DonMeshPainterGlobalActor.h"
#include "DonMeshPaintingPrivatePCH.h"
#include "DonMeshPainterComponent.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Runtime/Engine/Classes/Components/SceneCaptureComponent2D.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Runtime/Engine/Classes/Kismet/KismetRenderingLibrary.h"

#define DON_BUGFIX_FOR_MORPH_TARGETS_POSCAPTURE_ENABLED 0

ADonMeshPainterGlobalActor::ADonMeshPainterGlobalActor(const FObjectInitializer& ObjectInitializer)
{
	SetReplicates(true);

	bAlwaysRelevant = true; // the global actor manages replication on behalf of other actors (currently - Landscape) and therefore has no generic way of managing relevancy

	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> UnwrapperMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> DilationMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> HiddenMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> SaveTextureToRTMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> DefaultBrushRenderMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> EraserBrushRenderMaterial;		
		ConstructorHelpers::FObjectFinderOptional<UMaterialParameterCollection> GlobalMeshPaintingParameters;

		FConstructorStatics()
			: UnwrapperMaterial(TEXT("/DonMeshPainting/Materials/PositionsTextureBaking/M_BakeMeshPositions.M_BakeMeshPositions")),
			DilationMaterial(TEXT("/DonMeshPainting/Materials/PositionsTextureBaking/M_DilatePositionsTexture.M_DilatePositionsTexture")),
			HiddenMaterial(TEXT("/DonMeshPainting/Materials/PositionsTextureBaking/M_BakingMask.M_BakingMask")),
			SaveTextureToRTMaterial(TEXT("/DonMeshPainting/Materials/Serialization/M_SaveTextureToRenderTarget.M_SaveTextureToRenderTarget")),
			DefaultBrushRenderMaterial(TEXT("/DonMeshPainting/Materials/RenderBrushes/M_PaintBrush_Regular.M_PaintBrush_Regular")),
			EraserBrushRenderMaterial(TEXT("/DonMeshPainting/Materials/RenderBrushes/M_PaintBrush_Eraser_Inst.M_PaintBrush_Eraser_Inst")),			
			GlobalMeshPaintingParameters(TEXT("/DonMeshPainting/Materials/MPC_GlobalMeshPainting.MPC_GlobalMeshPainting"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	UnwrapperMaterial = ConstructorStatics.UnwrapperMaterial.Get();
	DilationMaterial = ConstructorStatics.DilationMaterial.Get();
	HiddenMaterial = ConstructorStatics.HiddenMaterial.Get();
	SaveTextureToRTMaterial = ConstructorStatics.SaveTextureToRTMaterial.Get();
	DefaultBrushRenderMaterial = ConstructorStatics.DefaultBrushRenderMaterial.Get();
	EraserBrushRenderMaterial = ConstructorStatics.EraserBrushRenderMaterial.Get();
	GlobalMeshPaintingParameters = ConstructorStatics.GlobalMeshPaintingParameters.Get();

	SceneRoot = CreateDefaultSubobject<USceneComponent>(FName("SceneRoot"));
	RootComponent = SceneRoot;

	UVPositionsCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(FName("UVPositionsCaptureComponent"));
	UVPositionsCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	UVPositionsCaptureComponent->OrthoWidth = 1000.f;
	UVPositionsCaptureComponent->SetWorldRotation(FRotator(-90.f, 0.f, -90.f));
	UVPositionsCaptureComponent->SetRelativeLocation(FVector(0, 0, 500.f));
	UVPositionsCaptureComponent->bCaptureEveryFrame = false;	
	UVPositionsCaptureComponent->ShowFlags.SetFog(false);
	UVPositionsCaptureComponent->ShowFlags.SetVolumetricFog(false);
	UVPositionsCaptureComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);	

	 // For Debug Visualization
	DebugCaptureMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DebugCaptureMesh"));
}

class UTextureRenderTarget2D* ADonMeshPainterGlobalActor::GetPositionsTextureForMesh(class UPrimitiveComponent* Mesh, const int32 MaterialIndex, const int32 UvChannel, const FBoxSphereBounds ReferenceBounds)
{
	UE_LOG(LogDonMeshPainting, Verbose, TEXT("Generating positions texture for %s"), Mesh ? *Mesh->GetName() : TEXT(""));

	if (!ensure(UnwrapperMaterialInstance && DilationMaterialInstance))
		return nullptr;

	UTextureRenderTarget2D* positionsTexture = nullptr;
	UTextureRenderTarget2D* dilatedPositionsTexture = nullptr;

	auto asStaticMesh = Cast< UStaticMeshComponent>(Mesh);
	auto asSkeletalMesh = Cast< USkeletalMeshComponent>(Mesh);

	auto cachedPositions = PositionsTextureCache.Find(Mesh);
	if (cachedPositions && cachedPositions->IsValidIndex(MaterialIndex))
	{
		positionsTexture = (*cachedPositions)[MaterialIndex].PositionsTexture;
		dilatedPositionsTexture = (*cachedPositions)[MaterialIndex].DilatedPositionsTexture;

		// For static meshes we can return this straight away. Skeletal meshes need to recalculate their positions (by pose) in each request.
		if(asStaticMesh && dilatedPositionsTexture) // latter check is necessary because we seed the cache early via "SetNumZeroed" below
			return dilatedPositionsTexture;
	}
	
	if(!positionsTexture || !dilatedPositionsTexture)
	{
		// Create:
		positionsTexture = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), UTextureRenderTarget2D::StaticClass());
		positionsTexture->InitAutoFormat(DefaultPositionsTextureSizeX, DefaultPositionsTextureSizeY);
		dilatedPositionsTexture = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), UTextureRenderTarget2D::StaticClass());
		dilatedPositionsTexture->InitAutoFormat(DefaultPositionsTextureSizeX, DefaultPositionsTextureSizeY);

		// and Cache:
		auto cachedPositions = PositionsTextureCache.FindOrAdd(Mesh);
		cachedPositions.SetNumZeroed(Mesh->GetNumMaterials());
		cachedPositions[MaterialIndex] = FDonMeshPositionsPerMaterial(positionsTexture, dilatedPositionsTexture);
		PositionsTextureCache.Add(Mesh, cachedPositions);

		ActiveRenderTargets.Add(positionsTexture);
		ActiveRenderTargets.Add(dilatedPositionsTexture);
	}
	
	CurrentUnwrap = dilatedPositionsTexture; // useful debug aid

	// Added in 4.22: Bugfix for long-standing "Morph Targets Flicker" issue, The previous technique is preserved in the else branch for reference.
#if DON_BUGFIX_FOR_MORPH_TARGETS_POSCAPTURE_ENABLED
		UObject* meshKey = nullptr;
		if (asStaticMesh)
			meshKey = asStaticMesh->GetStaticMesh();
		else if (asSkeletalMesh)
			meshKey = asSkeletalMesh->SkeletalMesh;
		else
		{
			meshKey = Mesh;
			ensureMsgf(false, TEXT("%s is neither a static mesh or nor a skeletal mesh! Unable to cache a common mesh object for generating positions texture."));
		}

		auto captureMesh = PositionsTextureMeshes.FindOrAdd(meshKey);
		//UPrimitiveComponent* captureMesh = DebugCaptureMesh; // swap with above for debug visualization
		if (!captureMesh)
		{
			captureMesh = NewObject<UPrimitiveComponent>(this, Mesh->GetClass(), NAME_None, RF_NoFlags, Mesh);
			//captureMesh->SetMobility(EComponentMobility::Movable);
			captureMesh->RegisterComponent();
			captureMesh->SetCullDistance(0);
			captureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
				captureMesh->SetMaterial(i, HiddenMaterial); // Comment this out for debug visualizing the last capture mesh...

			PositionsTextureMeshes.Add(meshKey, captureMesh);
			CommonPositionCaptureMeshes.Add(captureMesh);
		}

		captureMesh->SetWorldTransform(Mesh->GetComponentTransform());
		captureMesh->SetWorldLocation(FVector(0, 0, -1000)); // As these are hidden and non-collidable, location should be irrelevant

		 //if (asSkeletalMesh) DebugCaptureMesh->SetSkeletalMesh(asSkeletalMesh->SkeletalMesh); // for debug alone		
		// ...		

		const FVector objectPosition = UDonMeshPaintingHelper::GetWorldBounds(captureMesh).Origin;
		SetActorLocation(objectPosition);
		UVPositionsCaptureComponent->SetRelativeLocation(FVector(0, 0, 500.f));		

		// Apply our unwrapper:	
		TArray<UMaterialInterface*> originalMaterials;
		for (int32 i = 0; i < captureMesh->GetNumMaterials(); i++)
		{
			 originalMaterials.Add(captureMesh->GetMaterial(i)); // used only for debug visualization in this new workflow
			if (i == MaterialIndex)
			{
				UnwrapperMaterialInstance->SetScalarParameterValue(DonMeshPaint::P_DonUvChannel, UvChannel); // route our UV Channel.
				UnwrapperMaterialInstance->SetVectorParameterValue(DonMeshPaint::P_BoundsMin, ReferenceBounds.Origin - ReferenceBounds.BoxExtent); // route our bounds
				UnwrapperMaterialInstance->SetVectorParameterValue(DonMeshPaint::P_BoundsMax, ReferenceBounds.Origin + ReferenceBounds.BoxExtent);
				captureMesh->SetMaterial(MaterialIndex, UnwrapperMaterialInstance); // Apply UV extraction material:				
			}
			else
			{
				captureMesh->SetMaterial(i, HiddenMaterial); // Hide this material so it doesn't interfere with our positions rendering!
			}
		}

		if (asSkeletalMesh)
		{
			auto captureMeshAsSkeletalMesh = Cast<USkeletalMeshComponent>(captureMesh);
			captureMeshAsSkeletalMesh->SetMasterPoseComponent(asSkeletalMesh);

			captureMeshAsSkeletalMesh->ClearMorphTargets();
			for (const auto& morphKvPair : asSkeletalMesh->GetMorphTargetCurves())
				captureMeshAsSkeletalMesh->SetMorphTarget(morphKvPair.Key, morphKvPair.Value);
		}

		// Step 1. Extract positions into UVs:
		UVPositionsCaptureComponent->TextureTarget = positionsTexture;
		UVPositionsCaptureComponent->ClearShowOnlyComponents();
		UVPositionsCaptureComponent->ShowOnlyComponent(captureMesh);
		UVPositionsCaptureComponent->CaptureScene();

		// Step 2. Dilate the resultant texture (this helps reduce seams between UV islands while painting)
		DilationMaterialInstance->SetTextureParameterValue(DonMeshPaint::P_LocalPositionsTexture, positionsTexture);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, dilatedPositionsTexture, DilationMaterialInstance);

		// Hide again
		for (int32 i = 0; i < captureMesh->GetNumMaterials(); i++)
		{
			captureMesh->SetMaterial(i, HiddenMaterial);
			//captureMesh->SetMaterial(i, originalMaterials[i]); // For debug visualization: Swap this line with above 
		}		
			
#else
		const FVector objectPosition = UDonMeshPaintingHelper::GetWorldBounds(Mesh).Origin;
		SetActorLocation(objectPosition);

		UVPositionsCaptureComponent->SetRelativeLocation(FVector(0, 0, 500.f));

		// Ensure that we are able to capture the mesh for the brief duration of our capture:
		const float originalMaxDrawDistance = Mesh->LDMaxDrawDistance;
		Mesh->SetCullDistance(0);

		// Memorize original materials and apply our unwrapper:	
		TArray<UMaterialInterface*> originalMaterials;
		for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
		{
			originalMaterials.Add(Mesh->GetMaterial(i));

			if (i == MaterialIndex)
			{
				if (!ensure(UnwrapperMaterialInstance))
					return nullptr;

				UnwrapperMaterialInstance->SetScalarParameterValue(DonMeshPaint::P_DonUvChannel, UvChannel); // route our UV Channel.
				UnwrapperMaterialInstance->SetVectorParameterValue(DonMeshPaint::P_BoundsMin, ReferenceBounds.Origin - ReferenceBounds.BoxExtent); // route our bounds
				UnwrapperMaterialInstance->SetVectorParameterValue(DonMeshPaint::P_BoundsMax, ReferenceBounds.Origin + ReferenceBounds.BoxExtent);
				Mesh->SetMaterial(MaterialIndex, UnwrapperMaterialInstance); // Apply UV extraction material:
							}
			else
				Mesh->SetMaterial(i, HiddenMaterial); // Hide this material so it doesn't interfere with our positions rendering!				
		}

		// Step 1. Extract positions into UVs:
		UVPositionsCaptureComponent->TextureTarget = positionsTexture;
		UVPositionsCaptureComponent->ClearShowOnlyComponents();
		UVPositionsCaptureComponent->ShowOnlyComponent(Mesh);
		UVPositionsCaptureComponent->CaptureScene();

		// Step 2. Dilate the resultant texture (this helps reduce seams between UV islands while painting)
		if (!ensure(DilationMaterialInstance))
			return nullptr;

		DilationMaterialInstance->SetTextureParameterValue(DonMeshPaint::P_LocalPositionsTexture, positionsTexture);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, dilatedPositionsTexture, DilationMaterialInstance);

		// Reinstate original materials:
		for (int32 i = 0; i < Mesh->GetNumMaterials(); i++)
			Mesh->SetMaterial(i, originalMaterials[i]);

		Mesh->SetCullDistance(originalMaxDrawDistance);
		
#endif // DON_BUGFIX_FOR_MORPH_TARGETS_POSCAPTURE_ENABLED

	return dilatedPositionsTexture;
}

void ADonMeshPainterGlobalActor::CachePositionsTexture(class UTextureRenderTarget2D* PositionsTexture, class UPrimitiveComponent* PrimitiveComponent, const int32 MaterialIndex)
{
	if (!PositionsTexture)
		return;

	TArray<DonPositionPixels>& cachedPositionsPerMaterial = PositionsAsPixelsCache.FindOrAdd(PrimitiveComponent);
	cachedPositionsPerMaterial.SetNumZeroed(PrimitiveComponent->GetNumMaterials());

	// ~ Load positions afresh  ~
	DonPositionPixels positionsAsPixels;
	FRenderTarget *RenderTarget = PositionsTexture->GameThread_GetRenderTargetResource();	
	RenderTarget->ReadFloat16Pixels(positionsAsPixels); // (Possible optimization: Use FRenderCommandFence and listen via Tick. However this will complicate flow of control and brush stroke processing.

	cachedPositionsPerMaterial[MaterialIndex] = positionsAsPixels;
}

bool ADonMeshPainterGlobalActor::UvFromCachedPositionsTexture(FVector2D& OutUV, class UTextureRenderTarget2D* PositionsTexture, const FBoxSphereBounds ReferenceBounds, FVector RelativePosition, class UPrimitiveComponent* PrimitiveComponent, const int32 MaterialIndex)
{
	const TArray<DonPositionPixels>& cachedPositionsPerMaterial = PositionsAsPixelsCache.FindOrAdd(PrimitiveComponent);
	if (!cachedPositionsPerMaterial.IsValidIndex(MaterialIndex))
		return false;

	const DonPositionPixels& positionsAsPixels = cachedPositionsPerMaterial[MaterialIndex];	

	// @todo #DoNConfig - Expose these to end-users so they can decide the trade-off b/w accuracy & performance as suited for their projects.
	const float desiredMinimumProximity = 3.f;
	const float maximumPermittedProximity = 50.f;
	const int32 travelStepPer512p = 8;

	return UDonMeshPaintingHelper::GetPositionsTextureUV(positionsAsPixels, ReferenceBounds, PositionsTexture->SizeX, PositionsTexture->SizeY, RelativePosition, PrimitiveComponent, OutUV, desiredMinimumProximity, maximumPermittedProximity, travelStepPer512p);
}

void ADonMeshPainterGlobalActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	for (auto painterComponent : ActivePainterComponents)
	{
		if (!painterComponent)
			continue;

		painterComponent->ClearAllRenderTargets();
	}
}

void ADonMeshPainterGlobalActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADonMeshPainterGlobalActor, ActivePainterComponents);

}

void ADonMeshPainterGlobalActor::OnRep_ActivePainterComponents()
{
	for (auto newPainterComponent : ActivePainterComponents)
	{
		if (!newPainterComponent)
			continue;

		newPainterComponent->GlobalMeshPainterActor = this; // signals to the client that painting can begin.
		if(newPainterComponent->bHasPendingUpdates)
		{
			newPainterComponent->ProcessPaintStrokes(); // helps the first OnRep (which was potentially skipped earlier) to proceed
			newPainterComponent->bHasPendingUpdates = false;
		}
	}
}

void ADonMeshPainterGlobalActor::FlushAllPaint()
{
	for (auto painterComponent : ActivePainterComponents)
	{
		if (!painterComponent)
			continue;

		painterComponent->Flush();
	}
}

void ADonMeshPainterGlobalActor::MultiCast_FlushAllPaint_Implementation()
{
	FlushAllPaint();
}

class UMaterialInstanceDynamic* ADonMeshPainterGlobalActor::GetCachedRenderMaterial(class UMaterialInterface* RenderMaterial)
{
	if (!RenderMaterial)
		return nullptr;

	auto** pRenderMID = RenderMaterialsCache.Find(RenderMaterial);
	if (pRenderMID)
		return *pRenderMID;
	else
	{
		auto renderMID = UMaterialInstanceDynamic::Create(RenderMaterial, this);		
		RenderMaterialsCache.Add(RenderMaterial, renderMID);
		ActiveRenderMaterials.Add(renderMID);
		
		return renderMID;
	}
}

class AActor* ADonMeshPainterGlobalActor::GetMeshPaintingOwnerActor(AActor* PaintableActor)
{
	if (!PaintableActor)
		return nullptr;

	if (PaintableActor->IsA(ALandscape::StaticClass()))
	{
		// Landscapes do not replicate, so we route paint strokes to the global manager for managing landscaper replication:
		return this;
	}

	return PaintableActor;
}

class UDonMeshPainterComponent* ADonMeshPainterGlobalActor::GetMeshPainterComponent(AActor* PaintableActor)
{
	auto owner = GetMeshPaintingOwnerActor(PaintableActor);
	if (!owner)
		return nullptr;

	// Lookup:
	TArray<UDonMeshPainterComponent*> meshPainterComponents;
	owner->GetComponents(meshPainterComponents);
	if (meshPainterComponents.Num() > 0)
		return meshPainterComponents[0];

	// Or Spawn:
	UDonMeshPainterComponent* meshPainter = NewObject<UDonMeshPainterComponent>(owner, UDonMeshPainterComponent::StaticClass(), NAME_None);
	meshPainter->RegisterComponent();
	meshPainter->GlobalMeshPainterActor = this;

	ActivePainterComponents.Add(meshPainter);

	return meshPainter;
}

class UTextureRenderTarget2D* ADonMeshPainterGlobalActor::LoadWorldSpacePaintingMetadata(const EDonUvAxes WorldAxes, const int32 LayerIndex)
{
	FString axesString = DonMeshPaint::AxesAsString(WorldAxes);
	FString assetBasePath("/DonMeshPainting/Textures/RenderTargets/WorldSpace/");
	FString layerPath = LayerIndex == 0 ? FString() : FString::Printf(TEXT("Layer%d/"), LayerIndex);
	FString assetName = FString::Printf(TEXT("RT_Don_WorldSpace%s_Layer%d"), *axesString, LayerIndex);	
	FString assetFullPath = FString::Printf(TEXT("%s%s%s.%s"), *assetBasePath, *layerPath, *assetName, *assetName);

	// LoadObject Cooking notes:-
	// Any assets loaded here are guaranteed to be included by the end-user in their materials (if not, the absence of the asset will be a No-Op anyway).
	// Therefore these texture assets _will_ be cooked in packaged games via direct asset reference in a material.
	// The purpose of this load operation is to facilitate an easy, convenient, single-node workflow for end-users.
	
	// One-time load (all loaded assets are cached downstream)
	auto renderTarget = LoadObject<UTextureRenderTarget2D>(nullptr, *assetFullPath);

	if (!renderTarget)
		LogWorldSpaceAssetMissingError(assetFullPath);

	return renderTarget;
}

void ADonMeshPainterGlobalActor::LogWorldSpaceAssetMissingError(const FString AssetPath)
{
#if !UE_BUILD_SHIPPING	
	if (LogCache_WorldSpaceAssetsWithKnownPathIssues.Contains(AssetPath))
		return; // we've already warned the user! Return to avoid spamming their logs/display.

	// Warn the user:
	FString message("\n[Mesh Painting Alert!]\n");
	message += "-----------------------------------\n";
	message += FString::Printf(TEXT("Render Target %s is missing!\n\n"), *AssetPath);
	message += "If you created an extra layer yourself, just setup your new RT in the specified path.\n\n";

	UE_LOG(LogDonMeshPainting, Error, TEXT("%s"), *message);
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, message);

	LogCache_WorldSpaceAssetsWithKnownPathIssues.Add(AssetPath); // smart log memorization

#endif // UE_BUILD_SHIPPING
}

void ADonMeshPainterGlobalActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureInitialSetup();

}

void ADonMeshPainterGlobalActor::EnsureInitialSetup()
{
	if (!DilationMaterialInstance)
		DilationMaterialInstance = UMaterialInstanceDynamic::Create(DilationMaterial, this);

	if (!UnwrapperMaterialInstance)
		UnwrapperMaterialInstance = UMaterialInstanceDynamic::Create(UnwrapperMaterial, this);

	if (!SaveTextureToRTMaterialInstance)
		SaveTextureToRTMaterialInstance = UMaterialInstanceDynamic::Create(SaveTextureToRTMaterial, this);
}

#if !UE_BUILD_SHIPPING
bool ADonMeshPainterGlobalActor::IsPawnWithKnownSetupIssues(UObject* PaintableComponent)
{
	auto actorComponent = Cast<UActorComponent>(PaintableComponent);
	auto pawn = actorComponent ? Cast<APawn>(actorComponent->GetOwner()) : nullptr;

	// If this pawn already has known setup issues, user has already been alerted via a previous log. This flag is used to avoid log spamming
	return LogCache_PawnsWithKnownSetupIssues.Contains(pawn);
}
#endif // UE_BUILD_SHIPPING

class UTextureRenderTarget2D* ADonMeshPainterGlobalActor::GetRenderTargetFromPool(class UTextureRenderTarget2D* Template)
{
	for (auto renderTarget : RenderTargetPool)
		if (renderTarget->SizeX == Template->SizeX && renderTarget->SizeY == Template->SizeY)
			return renderTarget;

	auto renderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), UTextureRenderTarget2D::StaticClass());
	renderTarget->InitAutoFormat(Template->SizeX, Template->SizeY);
	renderTarget->RenderTargetFormat = Template->RenderTargetFormat;

	renderTarget->UpdateResourceImmediate();
	RenderTargetPool.Add(renderTarget);

	return renderTarget;
}