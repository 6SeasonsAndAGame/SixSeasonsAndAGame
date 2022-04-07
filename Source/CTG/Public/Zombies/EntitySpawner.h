// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "EntitySpawner.generated.h"


UENUM(BlueprintType, meta=(DisplayName="SpawnState"))
enum class ESpawnState : uint8
{
	Random			UMETA(DisplayName="Random"),
	Centered		UMETA(DisplayName="Centered"),
	Rastered		UMETA(DisplayName="Rastered")
};


/**
 * A Volume used to spawn AI Pawns
 * NOTE: Make sure to adjust the objects height above the ground as the Entities might spawn floating otherwise
 *		 Clipping can be prevented by setting *SpawnZOffset*
 */
UCLASS(HideCategories=(Collision, HLOD, LOD, Navigation, WorldPartition, Cooking, Actor), meta=(DisplayName="Zombie Spawner"))
class CTG_API AEntitySpawner final : public AVolume
{
	GENERATED_BODY()

public:
	AEntitySpawner();
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginDestroy() override;

	// Activates the spawner
	UFUNCTION(BlueprintCallable)
	void Activate();

	/** Deactivates the spawner
	 * NOTE: Does NOT kill alive Entities
	 * Use *CleanDeactivate* instead*/
	UFUNCTION(BlueprintCallable)
	void Deactivate();

	// Deactivates the spawner and deletes remaining Entities
	UFUNCTION(BlueprintCallable)
	void CleanDeactivate();
	
protected:
	// The classes of the Entities to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(DisplayThumbnail="true"))
	TArray<TSubclassOf<APawn>> EntityClasses;

	UPROPERTY(BlueprintReadOnly)
	TArray<APawn*> Entities;

	// A Z-Offset applied to a spawned Entity to ensure they aren't clipping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	float SpawnZOffset = 188.0f;

	// The amount of objects to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner", meta=(EditCondition="SpawnState!=ESpawnState::Rastered"))
	int Population = 10;

	// How many Entities are active
	UPROPERTY(BlueprintReadOnly)
	int EntitiesAlive = 0;

	// Determines the spawning pattern
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	ESpawnState SpawnState;

	// The closest something is spawned to the center of the Volume
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Centered", meta=(EditCondition="SpawnState==ESpawnState::Centered"))
	float InnerRing = 0.1f;

	// The furthest something is spawned to the center of the Volume
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Centered", meta=(EditCondition="SpawnState==ESpawnState::Centered"))
	float OuterRing = 1.0f;

	// Whether a custom or automatic CenterDensity should be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Centered", meta=(EditCondition="SpawnState==ESpawnState::Centered"))
	bool bRandomCenterDensity = true;

	// Influences the average closeness to the center something is spawned at
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Centered", meta=(ClampMin = 0.0f, EditCondition="SpawnState==ESpawnState::Centered && !bRandomCenterDensity"))
	float CenterDensity = 2.0f;

	/** Determines the grid layout for *Rastered* spawning
	 * A setting of 3x3 would spawn 9 Entities in 3 rows and 3 columns */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Rastered", meta=(EditCondition="SpawnState==ESpawnState::Rastered"))
	FIntPoint RasterLayout = FIntPoint(3, 3);

	// How much of the volumes size should be used
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Rastered", meta=(ClampMin=0.1f, ClampMax=1.0f, EditCondition="SpawnState==ESpawnState::Rastered"))
	float BoxRasterScaling = 1.0f;

	/** Whether the objects will be  spawned on an object any distance
	 * under the bounding box, or if they should spawn at the bounding
	 * box floor if no other object was hit							*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	bool bBoundingBoxIsLowest = true;

	/** whether the actors should be spawned on BeginPlay;
	 * or if they will be spawned at another time      */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner")
	bool bSpawnOnBeginPlay = false;

	/** Time required to pass since *MinNumberBeforeRespawn* amount of Entities is under passed
	 * before a new batch (*RespawnBatchSize*) of Entities is spawned */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Respawn")
	float TimeToRespawn = 60.0f;

	// The amount of actors to respawn every time respawn is called
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Respawn")
	int RespawnBatchSize = 1;

	// Time passed since the last Entity was spawned
	UPROPERTY(BlueprintReadOnly)
	float TimeSinceRespawn = 0.0f;

	// The minimum number of Entities that can be alive before any are respawned
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawner|Respawn")
	int MinNumberBeforeRespawn = 2;
	
	// Gets spawn points from random points in the volume
	UFUNCTION(BlueprintCallable, Category="Spawner")
	void GetRandomSpawnPoints(int Amount);

	// Gets spawn points from random points distributed spherically around the center of the volume
	UFUNCTION(BlueprintCallable)
	void GetSphereCenteredSpawnPoints(int Amount);

	// Gets spawn points from points in a raster of the volume
	UFUNCTION(BlueprintCallable)
	void GetRasteredSpawnPoints();

	// Spawns a single Entity
	UFUNCTION()
	APawn* SpawnEntity(FVector SpawnPoint, const FRotator Rotation);

	/** Spawns *Population* amount of Entities
	 * NOTE: Deletes all previously spawned Entities */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Spawner")
	void Spawn();

	// Adds *Population* amount of Entities
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Spawner")
	void AddEntity();
	void AddEntity(const int Amount);

	// Deletes all active Entities
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Spawner")
	void DeleteAllEntities();

private:
	UPROPERTY()
	TArray<FVector> SpawnPoints;

	UPROPERTY()
	TArray<FRotator> SpawnRotation;

	UPROPERTY()
	bool bCanRespawn = true;
	
	void SetCanRespawn(const bool bValue);

	UFUNCTION()
	bool LineTraceToGround(FVector &NewPoint, FRotator &OutRotation) const;
	
	UFUNCTION()
	void VerifyActiveEntities();
};
