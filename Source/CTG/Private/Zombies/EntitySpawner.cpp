// Fill out your copyright notice in the Description page of Project Settings.

#include "Zombies/EntitySpawner.h"
#include "Components/BrushComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


AEntitySpawner::AEntitySpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	GetBrushComponent()->SetMobility(EComponentMobility::Movable);
	SetActorEnableCollision(false);
}

void AEntitySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (bSpawnOnBeginPlay)
	{
		Spawn();
	}
	
	SetActorTickEnabled(bCanRespawn);
}

void AEntitySpawner::BeginDestroy()
{
	DeleteAllEntities();

	Super::BeginDestroy();
}

void AEntitySpawner::SetCanRespawn(const bool bValue)
{
	bCanRespawn = bValue;
	SetActorTickEnabled(bValue);
}

void AEntitySpawner::Activate()
{
	SetCanRespawn(true);
}

void AEntitySpawner::Deactivate()
{
	SetCanRespawn(false);
}

void AEntitySpawner::CleanDeactivate()
{
	SetCanRespawn(false);
	DeleteAllEntities();
}

void AEntitySpawner::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeSinceRespawn += DeltaSeconds;
	if (TimeSinceRespawn > TimeToRespawn)
	{
		VerifyActiveEntities();
		if (EntitiesAlive <= MinNumberBeforeRespawn)
		{
			AddEntity(RespawnBatchSize);
		}
		TimeSinceRespawn = 0.0f;
	}
}

bool AEntitySpawner::LineTraceToGround(FVector &NewPoint, FRotator &OutRotation) const
{
	FVector InOrigin;
	FVector InBoxExtent;
	FBox BoundingBox = GetComponentsBoundingBox();
	BoundingBox.GetCenterAndExtents(InOrigin, InBoxExtent);
	
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, this);
	TraceParams.bTraceComplex = true;
	TraceParams.bReturnPhysicalMaterial = false;
	FHitResult OutHit(ForceInit);
	FVector Start = NewPoint;
	
	FVector End = FVector(Start.X, Start.Y, InOrigin.Z * InBoxExtent.Z * -1);
	if (bBoundingBoxIsLowest)
	{
		End = FVector(Start.X, Start.Y, InOrigin.Z - InBoxExtent.Z);
	}
	
	ECollisionChannel Channel = ECC_Visibility;
	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, Channel, TraceParams);
	if (OutHit.GetActor())
	{
		NewPoint = OutHit.ImpactPoint;
		OutRotation = (OutHit.ImpactNormal + Start.DownVector).Rotation();
		return true;
	}
	if (bBoundingBoxIsLowest)
	{
		NewPoint = End;
		return true;
	}
	return false;
}

void AEntitySpawner::GetRandomSpawnPoints(int Amount)
{
	for (int i = 0; i < Amount; i++)
	{
		FVector InOrigin;
		FVector InBoxExtent;
		FBox BoundingBox = GetComponentsBoundingBox();
		BoundingBox.GetCenterAndExtents(InOrigin, InBoxExtent);
		FVector NewPoint = UKismetMathLibrary::RandomPointInBoundingBox(InOrigin, InBoxExtent);
		FRotator OutRotation;
		if (LineTraceToGround(NewPoint, OutRotation))
		{
			SpawnPoints.Add(NewPoint);
			SpawnRotation.Add(OutRotation);
		}
	}
}

void AEntitySpawner::GetSphereCenteredSpawnPoints(int Amount)
{
	for (int i = 0; i < Amount; i++)
	{
		FVector InOrigin;
		FVector InBoxExtent;
		FBox BoundingBox = GetComponentsBoundingBox();
		BoundingBox.GetCenterAndExtents(InOrigin, InBoxExtent);
		
		// Calculate a point in a circle with a higher chance to spawn near the circles center
		if (bRandomCenterDensity)
		{
			CenterDensity = FMath::FRand();
		}

		// Radius is unified
		float Radius = FMath::Pow(FMath::FRand(), CenterDensity);
		Radius = Radius < InnerRing ? InnerRing : Radius;
		Radius = Radius > OuterRing ? OuterRing : Radius;
		Radius *= FMath::Min(InBoxExtent.X, InBoxExtent.Y);
		
		const float Degree = FMath::FRandRange(0.0f, 360.0f);
		const float Y = InOrigin.Y + std::cos(Degree) * Radius;
		const float X = InOrigin.X + std::sin(Degree) * Radius;
		FVector NewPoint = FVector(X, Y, InOrigin.Z);
		
		FRotator OutRotation;
		if (LineTraceToGround(NewPoint, OutRotation))
		{
			SpawnPoints.Add(NewPoint);
			SpawnRotation.Add(OutRotation);
		}
	}
}

void AEntitySpawner::GetRasteredSpawnPoints()
{
	FVector InOrigin;
	FVector InBoxExtent;
	const FBox BoundingBox = GetComponentsBoundingBox();
	BoundingBox.GetCenterAndExtents(InOrigin, InBoxExtent);
	
	const float Scaling = 2.0f * BoxRasterScaling;
	const float StartX = RasterLayout.X == 1 ? InOrigin.X : InOrigin.X - InBoxExtent.X / (1 / BoxRasterScaling);
	const float StartY = RasterLayout.Y == 1 ? InOrigin.Y : InOrigin.Y - InBoxExtent.Y / (1 / BoxRasterScaling);
	const float SegmentLengthX = RasterLayout.X == 1 ? 0.0f : InBoxExtent.X * Scaling / (RasterLayout.X - 1);
	const float SegmentLengthY = RasterLayout.Y == 1 ? 0.0f : InBoxExtent.Y * Scaling / (RasterLayout.Y - 1);
	
	for (int i = 0; i < RasterLayout.Y; i++)
	{
		for (int j = 0; j < RasterLayout.X; j++)
		{
			FVector NewPoint = FVector(StartX + SegmentLengthX * j, StartY + SegmentLengthY * i, InOrigin.Z);
			FRotator OutRotation;
			if (LineTraceToGround(NewPoint, OutRotation))
			{
				SpawnPoints.Add(NewPoint);
				SpawnRotation.Add(OutRotation);
			}
		}
	}
}

APawn* AEntitySpawner::SpawnEntity(FVector SpawnPoint, const FRotator Rotation)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	SpawnPoint.Z += SpawnZOffset;
	
	const int RandActor = FMath::RandRange(0, EntityClasses.Num() - 1);
	checkf(EntityClasses[RandActor], TEXT("AIEntityClasses Array in SpawnerVolume Object has empty field"));

	return GetWorld()->SpawnActor<APawn>(EntityClasses[RandActor], SpawnPoint, Rotation, SpawnParams);
}

void AEntitySpawner::Spawn()
{
	DeleteAllEntities();
	AddEntity();
}

void AEntitySpawner::AddEntity()
{
	AddEntity(Population);
}

void AEntitySpawner::AddEntity(const int Amount)
{
	SpawnPoints.Empty();
	SpawnRotation.Empty();
	
	switch (SpawnState)
	{
	case ESpawnState::Centered:
		GetSphereCenteredSpawnPoints(Amount);
		break;
		
	case ESpawnState::Rastered:
		GetRasteredSpawnPoints();
		break;
		
		//  Random case as default spawn setting
		default:
			GetRandomSpawnPoints(Amount);
		break;
	}

	EntitiesAlive += SpawnPoints.Num();
	for (int i = 0; i < SpawnPoints.Num(); i++)
	{
		const FVector SpawnPoint = SpawnPoints[i];
		FRotator ActorRotation = SpawnRotation[i];
		ActorRotation.Yaw += FMath::RandRange(0.0f, 360.0f); 

		if (APawn* NewAI = SpawnEntity(SpawnPoint, ActorRotation))
		{
			Entities.Add(NewAI);
		}
	}
}

void AEntitySpawner::VerifyActiveEntities()
{
	EntitiesAlive = 0;
	for (int i = 0; i < Entities.Num(); i++)
	{
		EntitiesAlive += IsValid(Entities[i]) ? 1 : 0;
	}
}

void AEntitySpawner::DeleteAllEntities()
{
	for (int i = 0; i < Entities.Num(); i++)
	{
		if (Entities[i])
		{
			Entities[i]->Destroy();
		}
	}
	
	for (int i = 0; i < Entities.Num(); i++)
	{
		if (!Entities[i]) { Entities.RemoveAt(i, 1, true); }
	}
	
	EntitiesAlive = 0;
}
