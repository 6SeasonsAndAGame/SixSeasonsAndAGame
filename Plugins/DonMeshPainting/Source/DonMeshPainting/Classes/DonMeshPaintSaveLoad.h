// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DonMeshPaintingDefinitions.h"
#include "CoreUObject.h"
#include "Engine.h"
#include "DonMeshPainterComponent.h"


#include "DonMeshPaintSaveLoad.generated.h"

// Based on FObjectAndNameAsStringProxyArchive, but with improved handling of "None" strings (null) for log cleanliness
struct FDonMeshPaintArchive : public FNameAsStringProxyArchive
{
	bool bLoadIfFindFails;
	FDonMeshPaintArchive(FArchive &InInnerArchive, bool bInLoadIfFindFails) : FNameAsStringProxyArchive(InInnerArchive), bLoadIfFindFails(bInLoadIfFindFails){ }

	virtual FArchive& operator<<(FWeakObjectPtr& WeakObjectPtr)
	{
		WeakObjectPtr.Serialize(*this);
		return *this;
	}

	virtual FArchive& operator<<(class UObject*& Obj)
	{
		if (IsLoading())
		{
			FString LoadedString;
			InnerArchive << LoadedString;

			Obj = FindObject<UObject>(nullptr, *LoadedString, false);
			if (!Obj && bLoadIfFindFails)
			{
				if (LoadedString != FString("None"))// represents null, no need to find/load these. Theoretically a user could name one of their objects as "None", but to avoid hammering LoadObject, this is considered an acceptable limitation.
					Obj = LoadObject<UObject>(nullptr, *LoadedString);
				else
					Obj = nullptr;
			}
		}
		else
		{
			FString SavedString(Obj->GetPathName());
			InnerArchive << SavedString;
		}
		return *this;
	}
};

/** Actor-level save data unit: 
* Contains paint save data for an entire actor and all its meshes/primitives */
USTRUCT()
struct FDonMeshPaintSaveDataActorUnit
{
	GENERATED_USTRUCT_BODY()

	/* By default  the Actor's name is used as the identifier and assumed to stays constant between save/load cycles.
	   If this is not the case for your project, you need to modify this identifier according to your specific needs (especially relevant for dynamically spawned actors)*/
	UPROPERTY() 
	AActor* Actor;

	UPROPERTY()
	TArray<FDonPaintableMeshData> PaintStrokesPerPrimitive;

	TArray<TArray<FDonBakedPaintMaterials>> BakedPaintData;

	UPROPERTY()
	FName AuditLog;
};

/** Global sava data unit:
* This is what you will be saving to disk through your SaveGame object or any custom C++ save system you may be using */
USTRUCT(BlueprintType)
struct FDonMeshPaintSaveData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FDonMeshPaintSaveDataActorUnit> Data;	

	UPROPERTY()
	bool bNeedsBakedTexturesForSkinnedMeshes;
};

// Archive operators for each of these save data units:

FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonMeshPaintSaveDataActorUnit& ActorUnit)
{
	Ar << ActorUnit.Actor;
	Ar << ActorUnit.PaintStrokesPerPrimitive;
    Ar << ActorUnit.BakedPaintData;
	Ar << ActorUnit.AuditLog;

	return Ar;
}

FORCEINLINE FArchive& operator<<(FArchive &Ar, FDonMeshPaintSaveData& SaveData)
{
	Ar << SaveData.Data;
	Ar << SaveData.bNeedsBakedTexturesForSkinnedMeshes;

	return Ar;
}

/**
 * 
 */
UCLASS()
class DONMESHPAINTING_API UDonMeshPaintSaveLoadHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

private:
	static void CollectSaveData(UDonMeshPainterComponent* PainterComponent, TArray<FDonMeshPaintSaveDataActorUnit> &SaveData, const bool bNeedsBakedTexturesForSkinnedMeshes);
	static void LoadSaveDataForActor(UDonMeshPainterComponent* PainterComponent, const FDonMeshPaintSaveDataActorUnit& ActorUnit, const bool bNeedsBakedTexturesForSkinnedMeshes);

public:

	/** [Save Mesh Paint As Bytes]
	*    Fetches all your mesh paint data as a byte array which you can save using any desired save system (eg: a BP Save Game object or a C++ save system of your choice)
	*    Note:- Remember to store the return value via your save system to actually save it!
	*
	* @param bNeedsBakedTexturesForSkinnedMeshes - If you do not need baked textures for your usecase then keep this disabled. 
	* Turning this on will significantly increase the size of your save game.
	*
	* More details: For static meshes the plugin only needs to save logical paint strokes (instead of baking textures), for skinned meshes with varying anim poses, baking textures 
	* is usually necessary for proper reprojection. It is recommended to test and determine whether your usecase really needs baked textures.
	*
	* Example: If your skinned meshes are only using one pose (character customization screens/etc) or if you they have only a finite number of poses (meaning if you can map save games to poses)
	* then you may benefit from leaving this flag disabled. Another example, is mass / large-scale damage effects such as damage to a crowd of characters where reprojection issues may not be evident.
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting | Save Load", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorFilters"))
	static TArray<uint8> SaveMeshPaintAsBytes(UObject* WorldContextObject, TArray<AActor*> ActorFilters, const bool bNeedsBakedTexturesForSkinnedMeshes = false);

	/* Use this function to load all the painted effects for a world. You need to pass in the data that you saved earlier using GetMeshPaintSaveData
	*  bFlushAllPaint - If this is set to true, all painted effects in the entire world will be reset (whether or not it is actually present in the saved bytes).
	*/
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting | Save Load", meta = (WorldContext = "WorldContextObject"))
	static void LoadMeshPaintFromBytes(UObject* WorldContextObject, const TArray<uint8>& SaveData, const bool bFlushAllPaint = true);	

	/* Use this to take character customizations/etc and load them onto multiple actors/A.I. This allows you to take a saved design and stamp it onto several actors at once.
	 * Important Note:- This function assumes that the first actor in the saved bytes is the source of the design to be cloned.To fulfill this criteria, use the 
	 * SaveMeshPaintAsBytes function and pass in exactly one actor into the ActorFilters parameter. That actor's paint will be used as the source.
	 */
	UFUNCTION(BlueprintCallable, Category = "Don Mesh Painting | Save Load", meta = (WorldContext = "WorldContextObject"))
	static void LoadMeshPaintOntoMultipleActors(UObject* WorldContextObject, const TArray<uint8>& SaveData, TArray<AActor*> ActorsToPaint);
};
