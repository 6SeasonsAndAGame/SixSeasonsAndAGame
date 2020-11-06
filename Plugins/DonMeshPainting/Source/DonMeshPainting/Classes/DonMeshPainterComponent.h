// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "DonMeshPaintingHelper.h"
#include "DonMeshPaintingDefinitions.h"
#include "DonMeshPainterGlobalActor.h"
//#include "Runtime/CoreUObject/Public/Serialization/ObjectAndNameAsStringProxyArchive.h"

#include "DonMeshPainterComponent.generated.h"

// Type used for rapid paint collision lookups:
typedef FIntVector DonMeshCellUnit;  // Note:- UHT doesn't recognize typedefs

// Type used to represent all the paint layers (render targets, position textures, etc) associated with a single material
typedef TMap<int32, struct FDonPaintLayer> DonMaterialPaintLayers;

/** Replication data structure used to keep track of paint strokes and the primitives associated with those strokes*/
USTRUCT()
struct FDonPaintableMeshData
{
	GENERATED_USTRUCT_BODY()

	/** The paintable entity (primitive/landscape/globalpainter) associated with a set of strokes*/
	UPROPERTY()
	TWeakObjectPtr<class UObject> Paintable;

	/** Collection of paint strokes with various metadata that can be use to reproduce a stroke exactly the same way on server and client*/
	UPROPERTY()
	TArray<FDonPaintStroke> PaintStrokes;	

	UPROPERTY(NotReplicated)
	int32 NumPaintStrokes = 0; // used to track new strokes that arrived from the server (this variable is the number of local strokes rendered).

	FORCEINLINE void AddStroke(const FDonPaintStroke& Stroke)
	{
		PaintStrokes.Add(Stroke);
		RefreshNumStrokesCount();
	}

	FORCEINLINE void RefreshNumStrokesCount()
	{
		NumPaintStrokes = PaintStrokes.Num();
	}

	// Support for creating effect templates from source actors (eg: a character) and loading the effects/designs onto multiple actors
	//
	// 1. Paintable name is explicitly serialized because the source actorfrom which saved effects are loaded may not be present in the destination map.
	// Eg: Think of a character customization map with a template character (which the player customizes and saves) which then needs to be loaded in a gameplay map onto hordes of A.I. (where the original character won't be present)
	UPROPERTY()
	FName PaintableName;

	// 2. While loading an effect template onto multiple actors we use the reference transform to position the target mesh exactly like the source mesh.	
	// Plugin-users currently need to take care of ensuring that the same animation pose is active on the target mesh(es)
	UPROPERTY()
	FTransform ReferenceTransform;
	//

	FDonPaintableMeshData(){}
	FDonPaintableMeshData(UObject* InMesh, const FTransform& InReferenceTransform)
		: Paintable(InMesh), ReferenceTransform(InReferenceTransform)
	{
		if (Paintable.IsValid())
			PaintableName = Paintable->GetFName();
	}
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonPaintableMeshData& DonPaintableMeshData)
{
	Ar << DonPaintableMeshData.Paintable;
	Ar << DonPaintableMeshData.PaintStrokes;
	Ar << DonPaintableMeshData.PaintableName;
	Ar << DonPaintableMeshData.ReferenceTransform;

	return Ar;
}

/* Contains the actual render target onto which paint is accumulated and its various metadata including Uv Method, Uv channel (for Mesh UV workflow only), position textures, baking timestamps, etc*/
struct FDonPaintLayer
{
	// The actual texture on which we paint:
	class UTextureRenderTarget2D* RenderTarget = nullptr;	

	EDonUvAcquisitionMethod UvMethod;

	// A positions lookup texture generated for some primitives (eg: skeletal meshes)
	class UTextureRenderTarget2D* PositionsTexture = nullptr;
	float PositionsTextureLastUpdate = 0.f;// store as World real time seconds	
	FBoxSphereBounds PositionsTextureReferenceBounds; // the reference bounds using which the latest positions texture was created
	
	int32 UvChannelInferred = 0;

	FDonPaintLayer(){}
	FDonPaintLayer(class UTextureRenderTarget2D* InRenderTarget, EDonUvAcquisitionMethod InUvMethod, int32 InUvChannelInferred = 0, class UTextureRenderTarget2D* InPositionsTexture = nullptr, float InPositionsTextureLastUpdate = 0.f)
		: 
		RenderTarget(InRenderTarget), 
		UvMethod(InUvMethod),		
		PositionsTexture(InPositionsTexture), 
		PositionsTextureLastUpdate(InPositionsTextureLastUpdate),
		UvChannelInferred(InUvChannelInferred){}
};

/**
*  [UDonMeshPainterComponent]
 *  This Component is the "Paint Engine" that orchestrates all painting activity from end-to-end.
 *
 *  For best results in networked games and for workflow convenience, it has been designed such that each actor has its own paintable component automatically assigned to it at run-time.
 *  This allows use of network-relevancy along with other benefits.
 *
 *   The Global Painter actor also has a special painter component of its own that is used for Landscape paint replication, for context-free World Space painting, etc.
 */
UCLASS()
class DONMESHPAINTING_API UDonMeshPainterComponent : public UActorComponent
{
	GENERATED_BODY()

	public:

	UDonMeshPainterComponent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY()
	ADonMeshPainterGlobalActor* GlobalMeshPainterActor;
	
	/** Facilitates rapid queries by dividing world space into queriable buckets. Constant in the current design.*/
	static const int32 MeshCellSize = 1000;
	
	bool PaintStroke(const FHitResult& Hit, const FDonPaintStroke& Stroke, const bool bAllowDuplicateStrokes, bool bAllowContextFreePainting, bool bPerformNetworkReplication = true);
	
	bool QueryPaintCollisionMulti(UPrimitiveComponent* Primitive, const TArray<FDonPaintCollisionQuery>& CollisionTags, FDonPaintCollisionQuery& OutCollisionTag, const FVector WorldLocation);	

	EDonUvAcquisitionMethod GetUvMethodForDistanceCheck(const FDonPaintStroke& Stroke);
	float GetDistanceToPaintedCollision(FTransform Transform, const FDonPaintStroke& Stroke, FVector WorldLocationToSample);
	float GetDistanceToPaintedCollision(UPrimitiveComponent* Primitive, const FDonPaintStroke& Stroke, FVector WorldLocationToSample);

	void LogWarning_ClientAttemptingPaintCollisionQuery();

	UObject* ResolvePaintableEntity(class UPrimitiveComponent* PrimitiveComponent, bool bAllowContextFreePainting, bool bWarnIfShapeCapsuleEncountered = true);
	EDonUvAcquisitionMethod ResolveUvAcquisitionMethod(const FName DefaultLayerName, UObject* PaintableComponent, const int32 MaterialIndex, const int32 FaceIndex, const bool bNeedsUVProjection, const int32 UvChannel);
	bool ResolveUVs(const EDonUvAcquisitionMethod UvAcquisitionMethod, UObject* PaintableComponent, const FDonPaintStroke& Stroke, const int32 UvChannel, FVector2D& OutUV);
	
	FVector2D GetEffectiveBrushSizeWS(UObject* PaintableComponent, const float BrushSize, const EDonUvAcquisitionMethod UvAcquisitionMethod, bool bNeedsDecalTexture);

private:
	bool IsMeshPaintedAt(UObject* PaintableComponent, FVector WorldLocation, float DistanceFactorRequired, int32 Layer);
	
public:
	// Networking:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;
	virtual void DestroyComponent(bool bPromoteChildren = false) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ClearRenderTarget(class UTextureRenderTarget2D* RenderTarget);
	void ClearAllRenderTargets();
	

private:

	UPROPERTY(ReplicatedUsing = OnRep_MeshStrokes)
	TArray<FDonPaintableMeshData> Rep_MeshPaintStrokes;

	UFUNCTION()
	void OnRep_MeshStrokes();	

public:
	FORCEINLINE TArray<FDonPaintableMeshData> GetPaintStrokes() { return Rep_MeshPaintStrokes; }
	FORCEINLINE void LoadPaintStrokes(const TArray<FDonPaintableMeshData>& PaintStrokes) { Rep_MeshPaintStrokes = PaintStrokes; }
	void ProcessPaintStrokes(const bool bIsSaveLoadCycle = false, const bool bIsUsingBakedTextures = false);
	bool bHasPendingUpdates = false;

	FORCEINLINE void Flush()
	{
		if(!GetOwner()->HasAuthority())
			Rep_MeshPaintStrokes.Empty();
		ClearAllRenderTargets();

		// Paint Blob Collision:
		MeshPaintStrokesMap.Empty();
	}

	void FlushMaterials(TArray<UMaterialInterface*> MaterialsToFlush);

private:
	FDonPaintableMeshData& GetPaintableMeshData(UObject* Paintable);
	FTransform GetPaintableComponentTransform(UObject* PaintableComponent);
	void SetPaintableComponentTransform(UObject* PaintableComponent, const FTransform& Transform);
	FVector GetPaintableComponentScaleDivisor(UObject* PaintableComponent);

	DonMeshCellUnit GetMeshCell(UObject* PaintableComponent, FVector WorldLocation);
	
	bool RenderPaintStroke(UObject* PaintableComponent, const FDonPaintStroke& Stroke, bool bSetupDataAndSkipRendering = false);
	class UMaterialInstanceDynamic* GetCachedRenderMaterial(class UMaterialInterface* RenderMaterial);

	void SyncRenderTargets(UTextureRenderTarget2D* Source, UTextureRenderTarget2D* Destination);

	FORCEINLINE bool IsPositionsCapturePending(const FDonPaintLayer& PaintLayer)
	{
		return PaintLayer.PositionsTextureLastUpdate == 0.f; // intentionally not using a simple-minded "texture != null". That kind of check can hide all kinds of nasty bugs under the hood!
	}

	bool IsPositionsTextureStale(const FDonPaintLayer& PaintLayer);
	bool IsPositionsTextureStale(UObject* PaintableComponent, const FDonPaintLayer& PaintLayer);

	// Paint Data per-layer, per-texture-per-mesh
	TMap<UObject*, TMap<int32, TMap<DonMeshCellUnit, TArray<FDonPaintStroke>>>> MeshPaintStrokesMap;

	// Paint Data per-mesh-per-material: (regular workflow)
	TArray<TMap<UObject*, DonMaterialPaintLayers>> PaintLayersByPaintableMesh;

	// Paint Data per explicit Uv: (context-free painting workflow)
	TMap<EDonUvAcquisitionMethod, DonMaterialPaintLayers> PaintLayersByExplicitUvMethod;

public:
	// Used for serializing skinned mesh paint as baked textures
	TArray<TArray<FDonBakedPaintMaterials>> ExtractBakedPaintData(const bool bIsSaveLoadCycle = false, const bool bExportToFile = false, FString ExportDirectory = FString());
	void LoadBakedPaintData(const TArray<TArray<FDonBakedPaintMaterials>>& Data);
	void DonExportPaintMasks(UObject* PaintableEntity, const FString AbsoluteDirectoryPath);

private:

	UPROPERTY()
	TArray<class UTextureRenderTarget2D*> ActiveRenderTargets;

	int32 DefaultTextureSizeX = 1024; // uint32 not supported by blueprints	
	int32 DefaultTextureSizeY = 1024;
	//		
	
	void LoadPositionsTexture(FDonPaintLayer& PaintTexture, UPrimitiveComponent* PrimitiveComponent, const int32 MaterialIndex, const int32 UvChannelToUse, const bool bPopulateCache);

	// Ensures that the requested number of paint layers are made available in the data structures used to store paint
	void EnsurePaintLayersBySize(const int32 RequiredSize);

	DonMaterialPaintLayers* SetupPaintingPrerequisites(UObject* PaintableComponent, const FDonPaintStroke& Stroke);
	DonMaterialPaintLayers* SetupPaintingPrerequisites_AutoUv(UObject* PaintableComponent, const FDonPaintStroke& Stroke);
	DonMaterialPaintLayers* SetupPaintingPrerequisites_ExplicitUv(UObject* PaintableComponent, const FDonPaintStroke& Stroke);

	class UTextureRenderTarget2D* SetupRenderTarget(UObject* PaintableComponent, const FDonPaintStroke& Stroke, UMaterialInterface* Material, const int32 MaterialIndex, bool bExistingRenderTargetIsCompulsory, EDonUvAcquisitionMethod& OutUvMethod, int32& OutUvChannel);

	void RegisterRenderTarget(UTextureRenderTarget2D* renderTargetToUse);


	class UTextureRenderTarget2D* ResolvePaintLayer(UObject* PaintableComponent, const FDonPaintStroke& Stroke, UMaterialInterface* Material, const int32 MaterialIndex, FName& OutPaintLayer, EDonUvAcquisitionMethod& OutUvMethod, int32& OutUvChannel);
	bool QueryMaterialPaintLayerByName(UMaterialInterface* Material, const FName LayerNamePrefix, const int32 LayerIndex, const int32 UvChannel, UTexture*& OutTexture, FName& OutLayerParameterName);

	UMaterialInstanceDynamic* GetActiveDynamicMaterial(UPrimitiveComponent* MeshComponent, const int32 MaterialIndex, const FName TextureName);	

	FORCEINLINE class ADonMeshPainterGlobalActor* GlobalPainter() { return GlobalMeshPainterActor; }

	static const int32 MaxUvChannelsSupported = 6;

	void ErasePaintCollision(UObject* PaintableComponent, const FDonPaintStroke& Stroke);

	// Debug
	void DebugMeshHole(bool bWithinRadius, const float distance, const float radius, const FDonPaintStroke &meshHole);
	void DebugMeshCell(UObject* PaintableComponent, const DonMeshCellUnit &Cell);

	// Logging
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Mesh Painter")
	bool bDisplayLogWarningsOnScreen = true;

private:
	void LogOrDisplay(FString Message);

	void LogError_HitUVMissing(UObject* PaintableComponent);
	void LogError_PlanarWorldSpaceUVMissing(UObject* PaintableComponent);	
	void LogError_HitContainsNoPaintableObject(UObject* PaintableComponent = nullptr);	
	void LogError_MeshNeededForPositionBasedPainting(UObject* PaintableComponent);
	void LogError_UnwrappedPositionsTextureNotFound(UObject* PaintableComponent);
	void LogError_PreExistingRenderTargetNotFound(UMaterialInterface* Material, const int32 LayerIndex);
	void LogError_PaintStrokeOutsideGlobalBounds();
	void LogError_GlobalBoundsNotSetup();
	void LogError_CollisionUVSupportNotAvailable(UObject* PaintableComponent);

	void LogWarning_CollisionUVSupportNotAvailable(UObject* PaintableComponent);
	void LogWarning_TryingToPaintOnACollisionCapsule(APawn* Pawn, UObject* PaintableComponent);
	void LogWarning_MaterialNotSetupForPainting(UMaterialInterface* Material, const int32 LayerIndex);
	void LogWarning_MaterialExcludedFromPainting(UMaterialInterface* Material, const int32 LayerIndex);
	void LogWarning_LandscapesMustUsePlanarWsUvs(const EDonUvAcquisitionMethod RequestedUvMethod);

#if !UE_BUILD_SHIPPING
	// Simple wrappers to access log memorization functionality in the Global Painter actor:
	FORCEINLINE void LogMemorize_MaterialWithSetupIssue(UMaterialInterface* Material)  { if (GlobalPainter()) GlobalPainter()->LogMemorize_MaterialWithSetupIssue(Material);	}
	FORCEINLINE void LogMemorize_PawnWithSetupIssue(class APawn* Pawn)	{  if (GlobalPainter()) GlobalPainter()->LogMemorize_PawnWithSetupIssue(Pawn);	}
	FORCEINLINE bool IsMaterialWithKnownSetupIssues(class UMaterialInterface* Material) 	{ return GlobalPainter() && GlobalPainter()->IsMaterialWithKnownSetupIssues(Material); }
	FORCEINLINE bool IsPawnWithKnownSetupIssues(class APawn* Pawn) { return GlobalPainter() && GlobalPainter()->IsPawnWithKnownSetupIssues(Pawn); }
	FORCEINLINE bool IsPawnWithKnownSetupIssues(class UObject* PaintableObject) { return GlobalPainter() && GlobalPainter()->IsPawnWithKnownSetupIssues(PaintableObject); }
	void DrawDebugPaintableBounds();
#endif // UE_BUILD_SHIPPING

};
