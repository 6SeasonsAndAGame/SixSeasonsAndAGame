// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "DonMeshPaintingDefinitions.h"

#include "DonMeshEffectsRaycastHelper.generated.h"

/**
 * 
 */
UCLASS()
class DONMESHPAINTING_API UDonMeshEffectsRaycastHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// ~~~ Don Raycast Functions! ~~~
	//
	// These are paint collision aware versions of Unreal's Blueprint raycast functions. The API is identical to the Kismet (Blueprint) raycast API found in KismetSystemLibrary.h
	// If you're a C++ user who prefers dealing with low-level raycast functions inside (eg: World->LineTraceSingleByChannel, etc), then you can still leverage the same setup; just change the input parameters of this API and DonTraceRecursive function (in CPP) to match your needs.
	//
	// ~*~*~*~

	/**
	* Does a collision trace along the given line and returns the first blocking hit encountered.
	* This trace finds the objects that RESPONDS to the given TraceChannel
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param TraceChannel
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | LineTraceByChannel", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool DonLineTraceSingle(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Does a collision trace along the given line and returns all hits encountered up to and including the first blocking hit.
	* This trace finds the objects that RESPOND to the given TraceChannel
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param TraceChannel	The channel to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | MultiLineTraceByChannel", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool DonLineTraceMulti(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Sweeps a sphere along the given line and returns the first blocking hit encountered.
	* This trace finds the objects that RESPONDS to the given TraceChannel
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius		Radius of the sphere to sweep
	* @param TraceChannel
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | SphereTraceByChannel", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	static bool DonSphereTraceSingle(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Sweeps a sphere along the given line and returns all hits encountered up to and including the first blocking hit.
	* This trace finds the objects that RESPOND to the given TraceChannel
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius		Radius of the sphere to sweep
	* @param TraceChannel
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	* @return				True if there was a blocking hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | MultiSphereTraceByChannel", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	static bool DonSphereTraceMulti(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	// ~ For Objects ~

	/**
	* Does a collision trace along the given line and returns the first hit encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param ObjectTypes	Array of Object Types to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | LineTraceForObjects", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool DonLineTraceSingleForObjects(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Does a collision trace along the given line and returns all hits encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param ObjectTypes	Array of Object Types to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | MultiLineTraceForObjects", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "raycast"))
	static bool DonLineTraceMultiForObjects(UObject* WorldContextObject, const FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Sweeps a sphere along the given line and returns the first hit encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius		Radius of the sphere to sweep
	* @param ObjectTypes	Array of Object Types to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHit		Properties of the trace hit.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | SphereTraceForObjects", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	static bool DonSphereTraceSingleForObjects(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

	/**
	* Sweeps a sphere along the given line and returns all hits encountered.
	* This only finds objects that are of a type specified by ObjectTypes.
	*
	* @param WorldContext	World context
	* @param DonTraceQuery - Supplies the "paint portal" name and other attributes which determine whether your raycasts will pass through a painted area
	* @param Start			Start of line segment.
	* @param End			End of line segment.
	* @param Radius		Radius of the sphere to sweep
	* @param ObjectTypes	Array of Object Types to trace
	* @param bTraceComplex	True to test against complex collision, false to test against simplified collision.
	* @param OutHits		A list of hits, sorted along the trace from start to finish.  The blocking hit will be the last hit, if there was one.
	* @return				True if there was a hit, false otherwise.
	*/
	UFUNCTION(BlueprintCallable, Category = "Collision", meta = (bIgnoreSelf = "true", WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Don Paint | MultiSphereTraceForObjects", AdvancedDisplay = "TraceColor,TraceHitColor,DrawTime", Keywords = "sweep"))
	static bool DonSphereTraceMultiForObjects(UObject* WorldContextObject, UPARAM(ref) FDonPaintTraceQuery& PaintPortalSetup, const FVector Start, const FVector End, float Radius, const TArray<TEnumAsByte<EObjectTypeQuery> > & ObjectTypes, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits, bool bIgnoreSelf, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawTime = 5.0f);

};
