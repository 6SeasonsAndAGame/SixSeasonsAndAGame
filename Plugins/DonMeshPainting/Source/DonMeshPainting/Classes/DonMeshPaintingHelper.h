// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "DonMeshPaintingDefinitions.h"
#include "Runtime/CoreUObject/Public/UObject/TextProperty.h"

#include "DonMeshPaintingHelper.generated.h"

/**
 * 
 */
UCLASS()
class DONMESHPAINTING_API UDonMeshPaintingHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// Public Blueprint/API Functions:

	//  ~~~ Paint via Hit Functions ~~~

	/** [Paint Stroke]
	*    Paints a stroke onto the specified paint layer for a given Hit result. Use this when you have a Hit Result handy, eg: from collision impacts, projectiles, mouse cursor hits, etc.
	*
	* @param Hit                            -  This is the Hit Result from your collision impacts, mouse/cursor hits (projected to world) or any other kind of HitResult carrying a valid primitive, impact location, etc.
	* @param BrushSize                 -  The radius of the paint stroke measured in World Space units. If you're using Mesh UVs, Effective brush size for decal/text projection is UV Island dependent
	* @param BrushDecalTexture  -  Optional Decal. The decal texture is expected to have the decal in RGB and the decal mask in its Alpha channel. Both can be used inside your material for specific effects.
	*
	* @param PaintLayer   -   Use this to drive multiple effects that are independent of each other. Each paint layer is a unique texture onto which a single effect may be painted as RGB with an Alpha mask 
	*                                      (or even multiple effects with individual R, G, B channel masks). A primitive can accommodate multiple paint layers for each material.
	*
	* @param CollisionTag  -  Use this to encode your paint stroke with a special collision tag that you can read back later to drive gameplay decisions, custom collisions, etc
	*                                      You will need to pass the same value back to the QueryPaintCollision node to check whether a particular location has been painted

	* @param BrushOpacity              -  The opacity of your paint stroke
	* @param BrushHardness           -  The hardness of your paint stroke, 0 = Soft, 1 = Hard
	* @param BrushDecalRotation    -  The rotation of the decal projected onto your mesh. For Mesh UV workflows rotation of your UV Island may influence actual rotation.
	*
	* @param BrushRenderMaterial      -  Defaults to "M_PaintBrush_Regular" which is an AlphaComposite brush meaning the RGB contains your paint/decal while the Alpha accumulates your paint mask & opacity.
	*                                                         If you're looking to pack R, G, B channel mask effects, just set this to the "M_PaintBrush_Regular_Additive" material provided in this plugin's Content folder.
	*                                                         Most users will not need to create their own render brush, but you can do so if desired.
	*
	* @param bAllowDuplicateStrokes  -  Important for Decals, Gameplay effects, etc! By default the plugin allows you to keep accumulating or overwriting paint strokes on a single region.
	*                                                         Sometimes, however it is desirable to prevent players from being able to repeat a stroke on a location that has already been painted. Set this to false to achieve that.
	*                                                         Eg: Painted Gameplay Items (like traps), Portal effects (eg: Exploding floors that leave holes behind them), Character Decal Tattoos that should only appear once, etc.
	*
	* @param CollisionInflationScale    -  Scales the paint collision associated with this stroke. Useful for Masked materials where your material's "Opacity Mask Clip" affects perceived collision.
	*                                                          As many collidable effects like Portals tend to use soft brushes or low opacity clip masks (to allow for crater WPO effects, etc to be visible...),
	*                                                          the size of the observed hole/collision perceived by the player is potentially _smaller_ the actual brush size (collision size) at which you painted the portal!
	*/	 
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (AdvancedDisplay = "7"))
	static bool PaintStroke(const FHitResult& Hit, float BrushSize = 10.f, FLinearColor BrushColor = FLinearColor::White, UTexture2D* BrushDecalTexture = nullptr, int32 PaintLayer = 0, const bool bEraseMode = false, const FName CollisionTag = NAME_None, const float BrushOpacity = 1.f, float BrushHardness = 0.1f, const int32 BrushDecalRotation = 0, UMaterialInterface* BrushRenderMaterial = nullptr, const bool bAllowDuplicateStrokes = true, const float CollisionInflationScale = 1.f);


	/** [Paint Text]  
	*    Paints text from UMG/Slate onto the specified paint layer, for a given Hit result. Use this to stamp text guided by collision impacts, player interaction, etc.
	*    If you already know exactly where to stamp the text beforehand (Eg: Shirt names/numbers) just use PaintTextAtComponent instead.
	*
	* @param Hit              -  This is the Hit Result from your collision impacts, mouse/cursor hits (projected to world) or any other kind of HitResult carrying a valid primitive, impact location, etc.
	* @param Text           -   The text to paint. Can be obtained from UMG, Slate or even constructed inline in a Blueprint (LiteralText) or C++ (NSLOCTEXT),
	* @param Style          -   Use this to set your Font, Font Size and other text properties.
	* @param Color          -   The color of your text. Typically you will just want to use this as a _mask_ and drive your actual text color inside the material. Note:- Alpha channel does not contain any information/mask for text because of the way it is rendered.
	* @param Rotation     -   The rotation of your text projection. If you're using a Mesh UV workflow (characters/etc), then the UV Islands of your UV map will influence the actual rotation.
	* @param PaintLayer  -   The paint layer onto which text is to be projected. Each paint layer is a separate texture. Typically each text needs a dedicated layer as text-material-render doesn't support alphas.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (AutoCreateRefTerm = "TextStyle"))
	static bool PaintText(const FHitResult& Hit, const FText Text, const FDonPaintableTextStyle& Style, const FLinearColor Color = FLinearColor::White, const int32 Rotation = 0, int32 PaintLayer = 0);


	//  ~~~ Paint via Component Functions ~~~

	/** [Paint Stroke At Component]
	*    Paints a stroke directly on a primitive for the specified paint layer. Use this when you know beforehand exactly which component/socket you're targeting to paint.
	*    This is great for orchestrating effects; Eg: when a character walks over a Lava trap you can splash the character's legs with Lava (Sample project has this very example in it!)
	*
	* @param PrimitiveComponent - The primitive onto which you're painting. Typically a skeletal mesh or a static mesh. For direct landscape painting the "PaintWorldDirect" function is recommended.
	* @param RelativeLocation - The _relative_ location on your mesh for this paint stroke. Ignored if SocketName is also set. Hint:- This is a relative space, not world space location.
	* @param SocketName - Specify a socket instead of relative location. Convenient for targeting specific limbs or body parts directly. Note:- Socket must be accurately placed at the desired part of your mesh for the effect to be rendered precisely.
	*
	* @param BrushSize                -  The radius of the paint stroke measured in World Space units. If you're using Mesh UVs, Effective brush size for decal/text projection is UV Island dependent
	* @param BrushDecalTexture  -  Optional Decal. The decal texture is expected to have the decal in RGB and the decal mask in its Alpha channel. Both can be used inside your material for specific effects.
	*
	* @param PaintLayer   -   Use this to drive multiple effects that are independent of each other. Each paint layer is a unique texture onto which a single effect may be painted as RGB with an Alpha mask
	*                                      (or even multiple effects with individual R, G, B channel masks). A primitive can accommodate multiple paint layers for each material.
	* @param CollisionTag  -  Use this to encode your paint stroke with a special collision tag that you can read back later to drive gameplay decisions, custom collisions, etc
	*                                      You will need to pass the same value back to the QueryPaintCollision node to check whether a particular location has been painted
	*
	* @param BrushOpacity              -  The opacity of your paint stroke
	* @param BrushHardness           -  The hardness of your paint stroke, 0 = Soft, 1 = Hard
	* @param BrushDecalRotation    -  The rotation of the decal projected onto your mesh. For Mesh UV workflows rotation of your UV Island may influence actual rotation.
	*
	* @param BrushRenderMaterial      -  Defaults to "M_PaintBrush_Regular" which is an AlphaComposite brush meaning the RGB contains your paint/decal while the Alpha accumulates your paint mask & opacity.
	*                                                         If you're looking to pack R, G, B channel mask effects, just set this to the "M_PaintBrush_Regular_Additive" material provided in this plugin's Content folder.
	*                                                         Most users will not need to create their own render brush, but you can do so if desired.
	* @param bAllowDuplicateStrokes  -  Important for Decals, Gameplay effects, etc! By default the plugin allows you to keep accumulating or overwriting paint strokes on a single region.
	*                                                         Sometimes, however it is desirable to prevent players from being able to repeat a stroke on a location that has already been painted. Set this to false to achieve that.
	*                                                         Eg: Painted Gameplay Items (like traps), Portal effects (eg: Exploding floors that leave holes behind them), Character Decal Tattoos that should only appear once, etc.
	* @param CollisionInflationScale    -  Scales the paint collision associated with this stroke. Useful for Masked materials where your material's "Opacity Mask Clip" affects perceived collision.
	*                                                          As many collidable effects like Portals tend to use soft brushes or low opacity clip masks (to allow for crater WPO effects, etc to be visible...),
	*                                                          the size of the observed hole/collision perceived by the player is potentially _smaller_ the actual brush size (collision size) at which you painted the portal!
	*
	* @param bDrawDebugLocation - Useful for visualizing exactly where the system is attempting to add paint. Use this for debugging your socket / relative location accuracy.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (AdvancedDisplay = "9"))
	static bool PaintStrokeAtComponent(class UPrimitiveComponent* PrimitiveComponent, FVector RelativeLocation, const FName SocketName = NAME_None, float BrushSize = 10.f, FLinearColor BrushColor = FLinearColor::White, UTexture2D* BrushDecalTexture = nullptr, int32 PaintLayer = 0, const bool bEraseMode = false, const FName CollisionTag = NAME_None, const float BrushOpacity = 1.f, float BrushHardness = 0.1f, const int32 BrushDecalRotation = 0, UMaterialInterface* BrushRenderMaterial = nullptr, const bool bAllowDuplicateStrokes = true, const float CollisionInflationScale = 1.f, bool bDrawDebugLocation = false);

	/** [Paint Text At Component]
	*    Paints text directly on a primitive for the specified paint layer. Use this when you know beforehand exactly which component/socket you're targeting to stamp text onto.
	*    Typical example is stamping shirt names & numbers for characters; because the location of the shirt name/number is (typically) fixed, you can use this function and pass in a target socket directly.
	*
	* @param PrimitiveComponent  -  The primitive onto which you're stamping text. Typically a skeletal mesh or a static mesh.
	* @param RelativeLocation       -  The _relative_ location on your mesh for this paint stroke. Ignored if SocketName is also set. Hint:- This is a relative space, not world space location.
	* @param SocketName             -  Specify a socket instead of relative location. Convenient for targeting specific limbs or body parts directly. Note:- Socket must be accurately placed along the collision body!
	*
	* @param Text           -   The text to paint. Can be obtained from UMG, Slate or even constructed inline in a Blueprint (LiteralText) or C++ (NSLOCTEXT),
	* @param Style          -   Use this to set your Font, Font Size and other text properties.
	* @param Color          -   The color of your text. Typically you will just want to use this as a _mask_ to drive your actual text color inside the material. Note:- Alpha channel does not contain any mask for text.
	* @param Rotation     -   The rotation of your text projection. If you're using a Mesh UV workflow (characters/etc), then the UV Islands of your UV map will influence the actual rotation.
	* @param PaintLayer  -   The paint layer onto which text is to be projected. Each paint layer is a separate texture. Typically each text needs a dedicated layer as text-material-render doesn't support alphas.
	*
	* @param bDrawDebugLocation - Useful for visualizing exactly where the system is attempting to draw text onto. Use this for debugging your socket / relative location accuracy.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting")
	static bool PaintTextAtComponent(class UPrimitiveComponent* PrimitiveComponent, FVector RelativeLocation, const FText Text, const FDonPaintableTextStyle& Style, const FName SocketName = NAME_None, const FLinearColor Color = FLinearColor::White, const int32 Rotation = 0, int32 PaintLayer = 0, bool bDrawDebugLocation = false);


	//  ~~~ Context-Free Painting Functions ~~~

	/** [Paint World Direct] - Context Free World Space painting along any two axes.
	*    Paints a shared world-space texture along XY or XZ or YZ directly, without the need to specify an actor or primitive as a context.
	*   Typically used for driving Global Effects like FoW systems. Can also be used for direct landscape painting if you're trying to orchestrate a predetermined effect,
	*
	* @param WorldLocation - The world location onto which paint is to be accumulated (and potentially used by several interested actors/effect-systems/materials).
	* @param WorldAxes - Choose from XY or XZ or YZ axes for accumulating paint. Because this is planar mapping you only control two axes; Eg: Painting on XY you lose control over Z axis, i.e. world height.
	*
	* @param BrushSize                -  The radius of the paint stroke measured in World Space units. If you're using Mesh UVs, Effective brush size for decal/text projection is UV Island dependent
	* @param BrushDecalTexture  -  Your Decal. The decal texture is expected to have the decal in RGB and the decal mask in its Alpha channel. Both can be used inside your material for specific effects.
	*
	* @param PaintLayer   -   Use this to drive multiple effects that are independent of each other. Each paint layer is a unique texture onto which a single effect may be painted as RGB with an Alpha mask
	*                                      (or even multiple effects with individual R, G, B channel masks). A primitive can accommodate multiple paint layers for each material.
	* @param CollisionTag  -  Use this to encode your paint stroke with a special collision tag that you can read back later to drive gameplay decisions, custom collisions, etc
	*                                      You will need to pass the same value back to the QueryPaintCollision node to check whether a particular location has been painted
	*
	* @param BrushOpacity              -  The opacity of your paint stroke
	* @param BrushHardness           -  The hardness of your paint stroke, 0 = Soft, 1 = Hard
	* @param BrushDecalRotation    -  The rotation of the decal projected onto your mesh. For Mesh UV workflows rotation of your UV Island may influence actual rotation.
	*
	* @param BrushRenderMaterial      -  Defaults to "M_PaintBrush_Regular" which is an AlphaComposite brush meaning the RGB contains your paint/decal while the Alpha accumulates your paint mask & opacity.
	*                                                         If you're looking to pack R, G, B channel mask effects, just set this to the "M_PaintBrush_Regular_Additive" material provided in this plugin's Content folder.
	*                                                         Most users will not need to create their own render brush, but you can do so if desired.
	* @param bAllowDuplicateStrokes  -  Important for Decals, Gameplay effects, etc! By default the plugin allows you to keep accumulating or overwriting paint strokes on a single region.
	*                                                         Sometimes, however it is desirable to prevent players from being able to repeat a stroke on a location that has already been painted. Set this to false to achieve that.
	*                                                         Eg: Painted Gameplay Items (like traps), Portal effects (eg: Exploding floors that leave holes behind them), Character Decal Tattoos that should only appear once, etc.
	* @param CollisionInflationScale    -  Scales the paint collision associated with this stroke. Useful for Masked materials where your material's "Opacity Mask Clip" affects perceived collision.
	*                                                          As many collidable effects like Portals tend to use soft brushes or low opacity clip masks (to allow for crater WPO effects, etc to be visible...),
	*                                                          the size of the observed hole/collision perceived by the player is potentially _smaller_ the actual brush size (collision size) at which you painted the portal!
	*
	* @param bPerformNetworkReplication - Special option to support things like FoW systems where each _local_ player needs a different/unique perspective and therefore, a non-replicating paint FX.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (AdvancedDisplay = "8", WorldContext = "WorldContextObject"))
	static bool PaintWorldDirect(UObject* WorldContextObject, const FVector WorldLocation, const EDonUvAxes WorldAxes, float BrushSize = 10.f, FLinearColor BrushColor = FLinearColor::White, UTexture2D* BrushDecalTexture = nullptr, int32 PaintLayer = 0, const bool bEraseMode = false, const FName CollisionTag = NAME_None, const float BrushOpacity = 1.f, float BrushHardness = 0.1f, const int32 BrushDecalRotation = 0, UMaterialInterface* BrushRenderMaterial = nullptr, const bool bAllowDuplicateStrokes = true, const float CollisionInflationScale = 1.f, const bool bPerformNetworkReplication = true);


	// ~~~ Paint Blob Collision API ~~~
	
	/** [Query Paint Collision]
	*    Quickly find out whether a collision tag has been "painted" at a particular world location on a given primitive (static mesh/landscape component/etc). Returns true if matching collision was found.
	*    Use this to build gameplay systems that react to painted areas. Eg: Painted A.I. traps or behavior cues, Portals, etc.
	*    For complex queries or for evaluating multiple collision tags you should use the QueryPaintCollisionMulti function instead.
	*
	* @param Primitive - Any primitive (mesh/landscape-component) for which paint collision is to be tested. Typically you will supply this via generic collision events without knowing the exact primitive yourself.
	* @param CollisionTag - The collision tag to look for. Remember to use exactly the same collision tag you passed into the system earlier in the Paint Stroke node!
	* @param WorldLocation - The world location on this primitive where collision is to be tested.
	* @param MinimumPaintBlobSize - This is the minimum blob/collision size this query needs to succeed. Eg: An A.I. may query for a Portal of a certain minimum size to ensure it can believably pass through.
	*
	* @param CollisionInflationScale - Scales the painted collision used for evaluating this query. 
	                                                       Typically the correct place to do this is while painting the stroke itself as you have more context of the overall effect there.
	*														
	* @param CollisionInflationInAbsoluteUnits - Collision Safety Net. Inflates the painted collision in world-space units. 
	*                                                                       Very useful for projectiles, etc that need additional margin for believable collision effects.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (AdvancedDisplay = "5"))
	static bool QueryPaintCollision(UPrimitiveComponent* Primitive, FName CollisionTag, const FVector WorldLocation, float MinimumPaintBlobSize = 25.f, float CollisionInflationScale = 1.f, float CollisionInflationInAbsoluteUnits = 0.f, int32 Layer = 0);

	/** [Query Paint Collision Multi]
	*    Execute complex collision queries across multiple collision tags with support for filters, bucketing of duplicate collision tags by distance brackets and more. Returns true if any matching collision was found.
	*    For a simple paint collision query you should just use the QueryPaintCollision function.	
	*
	* @param Primitive - Any primitive (mesh/landscape-component) for which paint collision is to be tested. Typically you will supply this via generic collision events without knowing the exact primitive yourself.
	* @param CollisionTags - List of Don Paint Collision Queries.  Each query has configurable collision filters in it.
	*                                     You can create multiple queries for the same collision tag! The sample project uses this technique to make A.I bots jump across small-sized holes and fall into large-sized ones.
	*                                     [Blueprint tip] drag a wire from this parameter, type "Make Array", and then "Make DonPaintCollisionQuery" to rapidly setup your queries.
	*
	* @param OutCollisionTag  - The first matching collision query that succeeded. Use this to drive gameplay decisions by testing which collision tag succeeded, and also which specific filters it passed with.
	*
	* @param WorldLocation - The world location on this primitive where collision is to be tested.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting")
	static bool QueryPaintCollisionMulti(class UPrimitiveComponent* Primitive, const TArray<FDonPaintCollisionQuery>& CollisionTags, FDonPaintCollisionQuery& OutCollisionTag, const FVector WorldLocation);

	 // ~ Purging functions ~

	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (WorldContext = "WorldContextObject"))
	static void FlushAllPaint(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "MaterialsFilter"))
	static void FlushPaintForActor(AActor* Actor, TArray<UMaterialInterface*> MaterialsFilter);


	/** [Don Export Paint Masks]
	*    Exports the paint masks textures (render targets) for all the materials of a given primitive to a specified path
	*    Note:- The exported texture is a UV mask denoting painted areas, it does not bake the actual color or effects visible on your material.
	*
	* @param Primitive - Any primitive (mesh/landscape-component) for which paint collision is to be tested. Typically you will supply this via generic collision events without knowing the exact primitive yourself.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Misc")
	static void DonExportPaintMasks(UObject* PaintableEntity, const FString AbsoluteDirectoryPath);

	// This is mostly for the sample project's convenience
	UFUNCTION(BlueprintPure, Category = "Don Mesh Painting")
	static FString GameSaveDirectory() { return FPaths::ProjectSavedDir(); }

	//

private:
	static bool PaintStroke_Internal(const FHitResult& Hit, FDonPaintStroke& Stroke, const bool bAllowDuplicateStrokes, bool bAllowContextFreePainting, const bool bPerformNetworkReplication = true);
	
public:

	// Public utility functions for internal/C++ use

	static void DrawTextureToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UTexture* Texture);
	static void DrawMaterialToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, UMaterialInterface* Material);
	static void DrawTextToRenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* TextureRenderTarget, const FText Text, FVector2D ScreenPosition, const float TextRotation, const FLinearColor BrushColor, const FDonPaintableTextStyle& TextStyle);	
	static UTextureRenderTarget2D* CloneRenderTarget(UTextureRenderTarget2D* SourceRenderTarget);

	static TArray<uint8> ExtractRenderTargetAsPNG(UObject* WorldContextObject, UTextureRenderTarget2D* RenderTarget, const bool bFillAlpha =false);
	static TArray<uint8> ExtractRenderTargetAsJPG(UObject* WorldContextObject, UTextureRenderTarget2D* RenderTarget, const bool bFillAlpha = false);
	
	static FBoxSphereBounds GetLocalBounds(class UPrimitiveComponent* PrimitiveComponent);
	static FBoxSphereBounds GetWorldBounds(class UPrimitiveComponent* PrimitiveComponent);
	static bool GetPaintableWorldBounds(UObject* WorldContextObject, FVector& OutWorldMin, FVector& OutWorldMax, const EDonUvAxes UvAxes);
	static bool GetPaintableLocalBounds(class UPrimitiveComponent* PrimitiveComponent, FVector& OutLocalMin, FVector& OutLocalMax);	

	static class ADonMeshPainterGlobalActor* GetGlobalManager(UObject* Context);

	// UV Acquisition Helpers:
	static bool GetCollisionUV(UObject* PaintableComponent, const FVector WorldLocation, const int32 FaceIndex, const int32 UvChannel, FVector2D& OutUV);
	static bool GetWorldSpacePlanarUV(UObject* WorldContextObject, FVector WorldLocation, const EDonUvAxes UvAxes, FVector2D& OutUV);
	static bool GetRelativeSpacePlanarUV(class UPrimitiveComponent* PrimitiveComponent, FVector WorldLocation, const EDonUvAxes UvAxes, FVector2D& OutUV);
	static bool GetPlanarUV(FVector2D& OutUV, FVector PlaneLocation, FVector PlaneMin, FVector PlaneMax, const EDonUvAxes UvAxes, bool bNeedsUVWRangeValidation = false);
	static bool GetPlanarUVW(FVector& OutUVW, FVector PlaneLocation, FVector PlaneMin, FVector PlaneMax);
	static bool GetPositionsTextureUV(class UTextureRenderTarget2D* PositionsTexture, const FBoxSphereBounds ReferenceBounds, FVector RelativePosition, class UPrimitiveComponent* PrimitiveComponent, FVector2D& OutUV, const float DesiredMinimumProximity = 3.f, const float MaximumPermittedProximity = 50.f, const int32 TravelStepPer512p = 16 );
	static bool GetPositionsTextureUV(const TArray<FFloat16Color>& PositionsAsPixels, const FBoxSphereBounds ReferenceBounds, int32 TextureSizeX, int32 TextureSizeY, FVector RelativePosition, class UPrimitiveComponent* PrimitiveComponent, FVector2D& OutUV, const float DesiredMinimumProximity = 1.f, const float MaximumPermittedProximity = 50.f, const int32 TravelStepPer512p = 16);

	static class UMeshComponent* GetMostProbablePaintingCandidate(APawn* Pawn);

	// Brush Size normalization helpers:
	static FVector2D BrushSizeFromLocalBounds(UObject* PaintableComponent, const float BrushSize, const FVector ComponentScaleDivisor, const float DecalProjectionHeuristic, const EDonUvAcquisitionMethod UvAcquisitionMethod, const bool bAlwaysNormalizeBrushSize = false);
	static bool BrushSizeFromWorldBounds(UObject* PaintableComponent, const float BrushSize, FVector2D& OutBrushSizeUV, const FVector ComponentScaleDivisor, const float DecalProjectionHeuristic, const EDonUvAcquisitionMethod UvAcquisitionMethod);
	static bool BrushSizeAsBoundsUvRatio(const float BrushSize, FVector2D& OutBrushSizeUV, const FVector BoundsMin, const FVector BoundsMax, const EDonUvAxes UvAxes, const FVector ComponentScaleDivisor = FVector(1, 1, 1), const bool bAlwaysNormalizeBrushSize = false);

	// Debug Visualization:
	static void DrawDebugPositionsTexture(class UTextureRenderTarget2D* PositionsTexture, class UPrimitiveComponent* PrimitiveComponent);

private:
	static void SanitizeCollisionUV(FVector2D &HitUV);
	FORCEINLINE static FVector2D IndexToUV(const int32 i, const int32 j, const int32 SizeX, const int32 SizeY)	{ return FVector2D((float)j / SizeX, (float)i / SizeY); }
};
