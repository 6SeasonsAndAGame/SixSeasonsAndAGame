// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#pragma once

#include "Runtime/SlateCore/Public/Fonts/SlateFontInfo.h"
#include "Runtime/Engine/Classes/Materials/MaterialInterface.h"

#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "Runtime/Engine/Classes/Components/SkinnedMeshComponent.h"

#include "DonMeshPaintingDefinitions.generated.h"

// Used for serializing effects that cannot be saved via pure logical representation (i.e. as paint strokes)
// Currently used for baking textures from skinned meshes as their paint strokes are pose dependent
struct FDonBakedPaintPerMaterial
{
	// Index of the material for which paint is baked. (Could easily use uint8 here, but for consistency with all of Unreal's mat APIs using int32 instead)
	int32 MaterialIndex; 

	// Baked paint texture as a byte array
	TArray<uint8> CompressedPaint;	
};

struct FDonBakedPaintMaterials
{
	TArray<FDonBakedPaintPerMaterial> BakedMaterials;
};

// Archive functions for baked paint:

FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonBakedPaintPerMaterial& BakedPaintMaterial)
{
	Ar << BakedPaintMaterial.MaterialIndex;
	Ar << BakedPaintMaterial.CompressedPaint;

	return Ar;
}


FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonBakedPaintMaterials& BakedPaintMaterials)
{
	Ar << BakedPaintMaterials.BakedMaterials;

	return Ar;
}

/** [EDonUvAcquisitionMethod] Contains various UV methods that can be use for painting a primitive. 
*     Typically end-users select their UV method in the material and this is automatically picked up by the system.
*     In advanced usercases (Global FX systems) we also allow end users to explicitly specify the UV method in the PaintWorldDirect Blueprint node (context-free painting)
*/
UENUM()
enum class EDonUvAcquisitionMethod : uint8
{
	/**  [By Unwrapped Positions Texture] This mode generates a positions texture by unwrapping and packing bounds-based relative positions of a given mesh into a texture.
	*
	*     [+] Seamless painting is easier to achieve in this mode because UV islands that are located far away from each other can still be painted in one single stroke
	*     [+] No need to enable "Support UV From Hit Results" for painting thus allowing face index to be optimized out of cooked builds.
	*     [+] Brush size can be accurately expressed in World Space units by use of a distance based sphere mask on the brush material.
	*     [-] Decal textures are only supported with an expensive CPU lookup. As exact UV location is not known beforehand it is much faster to use a procedural brush in the Material Editor as that runs on GPU
	*/
	UnwrappedPositionsTexture,

	/**  [By Collision UVs] This mode requires "Support UV From Hit Results" to be enabled under project settings (Physics category). Only works for static meshes.
	*
	*     [+] Allows precise projection of custom textures because the exact UV coordinates are known.
	*     [-] Requires multiple texture parameters per material because a Hit result doesn't directly map to a render material
	*     [-] Seamless painting is hard to achieve when UV islands of neighboring mesh faces are located far away from each other in the UV map.
	*     [-] Brush Size cannot be accurately expressed in World Space units as UV mapping of a given texture to a given mesh in the world space cannot be generically derived.
	*/
	ByCollisionUVs,

	/**   [By Planar Mapping In WorldSpace] Ideal for planar surfaces like landscapes, for tagging actors in World-Space, or even to drive global effect systems like FoW!
	*      Remember to open "MPC_GlobalMeshPainting" asset and setup your World Min/Max bounds for this to work correctly.
	*
	*     [+] Single texture can be shared across the entire world.
	*     [+] Rapid UV coordinate lookup with simple math
	*     [+] Seamless painting is possible because UVs are based in world space.
	*     [+] Allows precise projection of custom textures because the exact UV coordinates are known.
	*     [-] You only control two axes at a time. Eg: If you choose XY mapping you lose control of Z axis, i.e. height information, as mapping is purely planar.
	*/
	PlanarMappingInWorldSpaceXY,
	PlanarMappingInWorldSpaceXZ,
	PlanarMappingInWorldSpaceYZ,

	/**   [By Planar Mapping In LocalSpace] Recommended for floors, walls or other planar surfaces that need specific effects to driven in Local Space.
	*       Examples include creation of holes/pits on a floor, creation of bullet holes in walls, Portals that projectiles or players can pass through, etc.
	*
	*     [+] Advantages are same as World Space
	*     [-] You need a unique texture for every asset using local space painting
	*/
	PlanarMappingInLocalSpaceXY,
	PlanarMappingInLocalSpaceXZ,
	PlanarMappingInLocalSpaceYZ,

	Unspecified
};

/** [EDonUvAxes] Represents world axes along two directions. Typically used for WorldSpace or LocalSpace painting*/
UENUM()
enum class EDonUvAxes : uint8
{
	XY,
	XZ,
	YZ,
	Invalid
};

/** [FDonPaintableTextStyle]  Text style used while stamping UMG tattoos, etc */
USTRUCT(BlueprintType)
struct FDonPaintableTextStyle
{
	GENERATED_USTRUCT_BODY()	

	/** Use this to set the Font for your text*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Text", meta = (AllowedClasses = "Font", DisplayName = "Font Family"))
	UObject* FontObject;

	/** (From Slate) "The material to use when rendering this font" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Text", meta = (AllowedClasses = "MaterialInterface"))
	UObject* FontMaterial;

	/** (From Slate) "The name of the font to use from the default typeface (None will use the first entry)" */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Text", meta = (DisplayName = "Font"))
	FName TypefaceFontName;

	/** The size of the font */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Text", meta = (UIMin = 1, UIMax = 1000, ClampMin = 1, ClampMax = 1000))
	int32 FontSize = 108;

	/** Fancy term for "horizontal letter spacing"*/
	UPROPERTY(BlueprintReadWrite, Category = "Don Paintable Text")
	float Kerning = 0.0f;	
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonPaintableTextStyle& TextStyle)
{
	Ar << TextStyle.FontObject;
	Ar << TextStyle.FontMaterial;
	Ar << TextStyle.TypefaceFontName;
	Ar << TextStyle.FontSize;
	Ar << TextStyle.Kerning;

	return Ar;
}

/** Metadata associated with a single paint stroke*/
USTRUCT()
struct FDonPaintStrokeParameters
{
	GENERATED_USTRUCT_BODY()

	/** Use this to drive multiple effects that are independent of each other.
	*    Each paint layer is a unique texture onto which a single effect may be painted as RGB with an Alpha mask (or even multiple effects with R, G, B channel masks).
	*    A primitive can accommodate multiple paint layers for each material.
	*/
	UPROPERTY()
	int32 PaintLayerIndex = 0;	

	/** Used for projecting decals as textures. A decal texture is expected to have the decal in RGB and the decal mask in its Alpha channel.
	 *   You can choose to use either the decal's RGB or it's alpha mask or both in your materials.
	 */
	UPROPERTY()
	class UTexture2D* BrushDecalTexture;

	/** Defaults to M_PaintBrush_Regular which is an AlphaComposite brush. This is the material with which actual texture painting is performed.
	*  You can also use M_PaintBrush_Regular_Additive for packing R, G, B channel mask effects. Most users will not need to create their own render brush, but you can do so if desired. 
	*/
	UPROPERTY()
	UMaterialInterface* BrushRenderMaterial;

	// Technical Note:- Weak pointers are not used here for Brush textures, Materials, etc as it is expected that we can hold a stable reference to these assets.
	// This is of interest from an efficiency standpoint, both for replication and for lookup.

	/** Opacity of your brush stroke*/
	UPROPERTY()
	float BrushOpacity = 1.f;

	/** Hardness of your brush stroke. 0 is smooth, 1 is hard*/
	UPROPERTY()
	float BrushHardness = 0.1f;

	/** Use this to encode your paint stroke with a special collision tag that you can read back later to drive gameplay decisions, custom collisions, etc
	*    You will need to pass the same value back to the QueryPaintCollision node to check whether a particular location has been painted*/
	UPROPERTY()
	FName CollisionTag = NAME_None;

	/** [Collision Inflation Scale]
	*    Important! Tweak this for best collision results with masked materials where your material's Opacity Mask Clip affects perceived collision.
	*   As many collidable effects like Portals tend to use soft brushes or low opacity clip masks (to allow for crater WPO effects, etc to be visible...),
	*   the size of the observed hole/collision perceived by the player is potentially _smaller_ the actual brush size (collision size) at which you painted the portal!
	* 	 Use this to compensate for such discrepancies between visuals and collision.
	*/
	UPROPERTY()
	float CollisionInflationScale = 1.f;

	FDonPaintStrokeParameters(){}
	FDonPaintStrokeParameters(const int32 InPaintLayerIndex, UTexture2D* InBrushDecalTexture, UMaterialInterface* InBrushRenderMaterial, const float InBrushOpacity, const float InBrushHardness,
		const FName InCollisionTag, const float InCollisionInflationScale) :
		PaintLayerIndex(InPaintLayerIndex),
		BrushDecalTexture(InBrushDecalTexture),
		BrushRenderMaterial(InBrushRenderMaterial), 
		BrushOpacity(InBrushOpacity),
		BrushHardness(InBrushHardness),
		CollisionTag(InCollisionTag),
		CollisionInflationScale(InCollisionInflationScale)
		{}	
};

FORCEINLINE_DEBUGGABLE FArchive& operator<<(FArchive &Ar, FDonPaintStrokeParameters& StrokeParams)
{
	Ar << StrokeParams.PaintLayerIndex;
	Ar << StrokeParams.BrushDecalTexture;
	Ar << StrokeParams.BrushRenderMaterial;
	Ar << StrokeParams.BrushOpacity;
	Ar << StrokeParams.BrushHardness;
	Ar << StrokeParams.CollisionTag;
	Ar << StrokeParams.CollisionInflationScale;

	return Ar;
}

// @todo Optimization idea for Paint Stroke: 
// For continuous strokes it will be more efficient to use a TArray<FVector> WorldLocations as only the location varies and all the metadata remains the same; repeating the data is inefficient.
// N.B. This is less relevant for typical gameplay scenarios (such as one-off impact collisions / etc) and more relevant for paint app usecases where continuous storkes are more common.

/*Data associated with a single paint stroke*/
USTRUCT()
struct FDonPaintStroke
{
	GENERATED_USTRUCT_BODY()
	
public:

	/** The world location at which a stroke was painted*/
	UPROPERTY()
	FVector WorldLocation;
	
	/** The brush size in word space units. Depending on the UV workflow used, actual results may be UV Island dependant (eg; for decal/text projection)*/
	UPROPERTY()
	float BrushSizeWS;

	/** The rotation with which a decal texture is projected*/
	UPROPERTY()
	int32 BrushDecalRotation;

	/** Used to fill the RGB of the painted texture. You can also use this to pack R, G, B effect masks by choosing the Additive render brush available with this plugin*/
	UPROPERTY()
	FLinearColor BrushColor;

	/** Only use if a Collision UV workflow is enabled for Static Meshes (decal stamping) */
	UPROPERTY()
	int32 FaceIndex;

	/** Metadata associated with a single paint stroke*/
	UPROPERTY()
	FDonPaintStrokeParameters StrokeParams;

	/** Sometimes, we allow users to explicitly specify their Uv method (Eg: Fog-Of-War system made with paint along World XY!)	
	  ** Defaults to "Unspecified" for normal workflows*/
	UPROPERTY()
	EDonUvAcquisitionMethod ExplicitUvMethod = EDonUvAcquisitionMethod::Unspecified;
	
	/** UMG Text used for tattoos, shirts, etc. This contains the actual text content*/
	UPROPERTY()
	FText Text;

	/** Text style with which UMG text is to be rendered onto the texture*/
	UPROPERTY()
	FDonPaintableTextStyle TextStyle;

	UPROPERTY()
	bool bEraseMode;

	FDonPaintStroke() {}

	FDonPaintStroke(const FVector InWorldLocation, const float InRadius, const int32 InBrushDecalRotation, const FLinearColor InBrushColor, const int32 InFaceIndex, 
		const FDonPaintStrokeParameters& InStrokeParams, const FText InText, const FDonPaintableTextStyle& InTextStyle, const bool bEraseModeIn = false) :
		WorldLocation(InWorldLocation), 
		BrushSizeWS(InRadius), 
		BrushDecalRotation(InBrushDecalRotation),
		BrushColor(InBrushColor), 
		FaceIndex(InFaceIndex),
		StrokeParams(InStrokeParams),
		Text(InText),
		TextStyle(InTextStyle),
		bEraseMode(bEraseModeIn)
	{
	}

	FORCEINLINE bool NeedsDecalTexture() const { return StrokeParams.BrushDecalTexture != nullptr; }
	FORCEINLINE bool HasText() const { return !Text.IsEmpty(); }
	FORCEINLINE bool NeedsDecalOrTextProjection() const { return NeedsDecalTexture() || HasText(); }
};

FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonPaintStroke& DonPaintStroke)
{
	Ar << DonPaintStroke.WorldLocation;
	Ar << DonPaintStroke.BrushSizeWS;
	Ar << DonPaintStroke.BrushDecalRotation;
	Ar << DonPaintStroke.BrushColor;
	Ar << DonPaintStroke.FaceIndex;
	Ar << DonPaintStroke.StrokeParams;
	Ar << DonPaintStroke.ExplicitUvMethod;

	bool bHasText;
	if (Ar.IsSaving())
		bHasText = !DonPaintStroke.Text.IsEmpty();

	Ar << bHasText;
	if (bHasText)
	{
		Ar << DonPaintStroke.Text;
		Ar << DonPaintStroke.TextStyle;
	}
	else
	{
		DonPaintStroke.Text = FText::GetEmpty();
		DonPaintStroke.TextStyle = FDonPaintableTextStyle();
	}

	Ar << DonPaintStroke.bEraseMode;

	return Ar;
}

/** Paint Blob Collisions query structure*/
USTRUCT(BlueprintType)
struct FDonPaintCollisionQuery
{
	GENERATED_USTRUCT_BODY()

	/** The layer in which to look for paint strokes. Different layers can contain different collision tags to support complex usecases */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Collision Query")
	int32 Layer = 0;

	/** Set this to the same collision tag that you used in "Paint Stroke" node while painting any collidable effect.	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Collision Query")
	FName CollisionTag;	

	/** This is the minimum blob/collision size this query needs to succeed. Eg: An A.I. may query for a Portal of a certain minimum size to ensure it can believably pass through.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Paintable Collision Query")
	float MinimumPaintBlobSize = 25.f;

	/** Scales the painted collision used for evaluating this query. Typically the correct place to do this is while painting the stroke itself as you have more context of the overall effect there*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Don Paintable Collision Query")
	float CollisionInflationScale = 1.f;

	/** Collision Safety Net. Inflates the painted collision in world-space units. Very useful for projectiles, etc that need additional margin for believable collision effects.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Don Paintable Collision Query")
	float CollisionInflationInAbsoluteUnits = 0.f;

	FDonPaintCollisionQuery(){}

	FDonPaintCollisionQuery(int32 InLayer, FName InCollisionTag, float InMinimumPaintBlobSize = 25.f, float InCollisionInflationScale = 1.f, float InCollisionInflationInAbsoluteUnits = 0.f)
		: Layer(InLayer), CollisionTag(InCollisionTag), MinimumPaintBlobSize(InMinimumPaintBlobSize), CollisionInflationScale(InCollisionInflationScale), CollisionInflationInAbsoluteUnits(InCollisionInflationInAbsoluteUnits)
	{

	}
};

USTRUCT(BlueprintType)
struct FDonPaintTraceQuery : public FDonPaintCollisionQuery
{
	GENERATED_USTRUCT_BODY()	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Don Linetrace")
	int32 MaxRecursion = 10;

	FDonPaintTraceQuery()
	{
		MinimumPaintBlobSize = -1; // for linetraces size should not matter
	}
};

namespace DonMeshPaint
{
	/** Standard paint layer names by Uv acquisition method. Additional permutations (by paint layer index and Uv channel) are derived from these standard names*/	
	const FName DonPaintLayer_MeshGeneric = FName("DonPaintLayer_MeshGeneric");
	const FName DonPaintLayer_WorldSpaceXY = FName("DonPaintLayer_WorldSpaceXY");
	const FName DonPaintLayer_WorldSpaceXZ = FName("DonPaintLayer_WorldSpaceXZ");
	const FName DonPaintLayer_WorldSpaceYZ = FName("DonPaintLayer_WorldSpaceYZ");
	const FName DonPaintLayer_LocalSpaceXY = FName("DonPaintLayer_LocalSpaceXY");
	const FName DonPaintLayer_LocalSpaceXZ = FName("DonPaintLayer_LocalSpaceXZ");
	const FName DonPaintLayer_LocalSpaceYZ = FName("DonPaintLayer_LocalSpaceYZ");

	const FName PaintLayersByUvMethod[7] = { DonPaintLayer_MeshGeneric, DonPaintLayer_WorldSpaceXY,  DonPaintLayer_LocalSpaceXY, 
																		  DonPaintLayer_WorldSpaceXZ, DonPaintLayer_LocalSpaceXZ, DonPaintLayer_WorldSpaceYZ, DonPaintLayer_LocalSpaceYZ };

	// Vital for preventing artifacts in Local Space painting.
	// Some meshes have such a tight bounding box that there is not enough safety room in the bounding box UVs generated from them. Scaling the bounds extent with a safety net solves that.
	const float LocalBoundsSafetyNet = 1.05f;

	// Save Games - Add this tag to any primitive component which needs to be excluded from the global Save
	const FName SkipComponentForSave = FName("SkipComponentForSave");

	/** Conversion from World Space Axes to World Space UV method*/
	inline EDonUvAxes WorldUvAxesFromUvMethod(EDonUvAcquisitionMethod UvMethod)
	{
		switch (UvMethod)
		{
		case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY:
			return EDonUvAxes::XY;
		case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ:
			return EDonUvAxes::XZ;
		case EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ:
			return EDonUvAxes::YZ;
		default:
			return EDonUvAxes::Invalid;
		}
	}

	/** Conversion from Local Space Axes to Local Space UV method*/
	inline EDonUvAxes LocalUvAxesFromUvMethod(EDonUvAcquisitionMethod UvMethod)
	{
		switch (UvMethod)
		{
		case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXY:
			return EDonUvAxes::XY;
		case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceXZ:
			return EDonUvAxes::XZ;
		case EDonUvAcquisitionMethod::PlanarMappingInLocalSpaceYZ:
			return EDonUvAxes::YZ;
		default:
			return EDonUvAxes::Invalid;
		}
	}

	/** Conversion from World Space UV method from World Space Axes*/
	inline EDonUvAcquisitionMethod UvMethodFromWorldUvAxes(EDonUvAxes Axes)
	{
		switch (Axes)
		{
		case EDonUvAxes::XY:
			return EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY;
		case EDonUvAxes::XZ:
			return EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ;
		case EDonUvAxes::YZ:
			return EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ;
		default:
			ensure(false);
			return EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY;
		}
	}

	/* Checks whether a given UV method is World Space planar mapped*/
	inline bool UvMethodIsPlanarWorldSapce(EDonUvAcquisitionMethod UvMethod)
	{
		return UvMethod == EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXY ||
			UvMethod == EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceXZ ||
			UvMethod == EDonUvAcquisitionMethod::PlanarMappingInWorldSpaceYZ;
	}
	
	/** String representation of Axes*/
	inline FString AxesAsString(EDonUvAxes Axes)
	{
		switch (Axes)
		{
		case EDonUvAxes::XY:
			return FString("XY");
		case EDonUvAxes::XZ:
			return FString("XZ");
		case EDonUvAxes::YZ:
			return FString("YZ");
		default:
			return FString("Unknown");
		}
	}

	// Standard DoN Painting material parameters
	// Render Brush:
	const FName P_BrushDecalTexture = FName("BrushDecalTexture");
	const FName P_BrushColor = FName("BrushColor");
	const FName P_EffectiveBrushSizeAveraged = FName("EffectiveBrushSizeAveraged");
	const FName P_EffectiveBrushSizeU = FName("EffectiveBrushSizeU");
	const FName P_EffectiveBrushSizeV = FName("EffectiveBrushSizeV");
	const FName P_IsNonUniformBrush = FName("IsNonUniformBrush");
	const FName P_BrushRotation = FName("BrushRotation");
	const FName P_BrushOpacity = FName("BrushOpacity");
	const FName P_BrushHardness = FName("BrushHardness");
	const FName P_IsPositionsTextureAvailable = FName("IsPositionsTextureAvailable");
	const FName P_IsBrushDecalTextureAvailable = FName("IsBrushDecalTextureAvailable");
	const FName P_UVx = FName("UVx");
	const FName P_UVy = FName("UVy");
	const FName P_LocalPositionsTexture = FName("LocalPositionsTexture");
	const FName P_LocalPosition = FName("Local Position");
	const FName P_BoundsMin = FName("BoundsMin");
	const FName P_BoundsMax = FName("BoundsMax"); // used in the render brush for Mesh UVs workflow (positions texture lookup)

	const FName P_RenderTarget = FName("RenderTarget");
	const FName P_RenderTargetPrevious = FName("RenderTargetPrevious");

	// Target Material:
	const FName P_DonLocalBoundsMin = FName("DonLocalBoundsMin");
	const FName P_DonLocalBoundsMax = FName("DonLocalBoundsMax"); // used in the target material for Local Space workflow (BoundingBoxBasedUVW generation)

	// Positions Texture Baking:
	const FName P_DonUvChannel = FName("DonUvChannel"); // low priority:- consider using a dedicated unwrapper per UV channel to optimize this further.

	// Global Painting configuration: (MPC)
	const FName P_WorldMin = FName("World Min");
	const FName P_WorldMax = FName("World Max");

	// Serialization:
	const FName P_TextureToRender = FName("Texture To Render");
	const FName P_ExcludeFromBaking = FName("Exclude From Paint Baking");

	inline bool NeedsBakedTextures(UObject* Paintable)
	{
		bool bNeedsBakedTextures = Paintable->IsA(USkinnedMeshComponent::StaticClass()); // skinned meshes need to use baked data as they are pose dependent
		if (bNeedsBakedTextures)
		{
			// Advanced usecase: Allow users to specify certain skinned meshes that never need to be baked (enemies/crowd damage effects/etc)
			auto paintableAsComponent = Cast<UActorComponent>(Paintable);
			if (paintableAsComponent->ComponentHasTag(P_ExcludeFromBaking))
				bNeedsBakedTextures = false;
		}

		return bNeedsBakedTextures;
	}
}