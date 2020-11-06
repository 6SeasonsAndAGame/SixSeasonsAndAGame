// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "DonMeshPaintSaveLoad.h"
#include "DonMeshPaintingPrivatePCH.h"

#include "DonMeshPaintingHelper.h"
#include "DonMeshPainterGlobalActor.h"
#include "DonMeshPainterComponent.h"

void UDonMeshPaintSaveLoadHelper::CollectSaveData(UDonMeshPainterComponent* PainterComponent, TArray<FDonMeshPaintSaveDataActorUnit> &SaveData, const bool bNeedsBakedTexturesForSkinnedMeshes)
{
	if (!ensure(PainterComponent))
		return;

	auto actor = PainterComponent->GetOwner();

	FDonMeshPaintSaveDataActorUnit actorData;
	actorData.Actor = actor;

	TArray<FDonPaintableMeshData> paintStrokesFiltered;
	for (auto& meshData : PainterComponent->GetPaintStrokes())
	{
		auto paintableAsPrimitive = Cast<UPrimitiveComponent>(meshData.Paintable.Get());
		if (paintableAsPrimitive && paintableAsPrimitive->ComponentHasTag(DonMeshPaint::SkipComponentForSave))
			continue; // filtered

		paintStrokesFiltered.Add(meshData);
	}	

	actorData.PaintStrokesPerPrimitive = paintStrokesFiltered;
	actorData.AuditLog = actor->GetFName();

	if (bNeedsBakedTexturesForSkinnedMeshes)
		actorData.BakedPaintData = PainterComponent->ExtractBakedPaintData(true /*bIsSaveLoadCycle*/);

	SaveData.Add(actorData);
}

TArray<uint8> UDonMeshPaintSaveLoadHelper::SaveMeshPaintAsBytes(UObject* WorldContextObject, TArray<AActor*> ActorFilters, const bool bNeedsBakedTexturesForSkinnedMeshes /*= true*/)
{
	TArray<uint8> saveDataBytes;

	FDonMeshPaintSaveData dataContainer;
	TArray<FDonMeshPaintSaveDataActorUnit> data;	

	auto globalPainter = UDonMeshPaintingHelper::GetGlobalManager(WorldContextObject);
	if (!ensure(globalPainter))
		return saveDataBytes;

	TArray<UDonMeshPainterComponent*> painterComponents;
	if(ActorFilters.Num())
	{
		for (auto actor : ActorFilters)
			painterComponents.Add(globalPainter->GetMeshPainterComponent(actor));
	}
	else
		painterComponents = globalPainter->GetActivePainterComponents();

	for (auto painterComponent : painterComponents)
	{
		if (!painterComponent)
			continue;

		CollectSaveData(painterComponent, data, bNeedsBakedTexturesForSkinnedMeshes);
	}

	dataContainer.Data = data;
	dataContainer.bNeedsBakedTexturesForSkinnedMeshes = bNeedsBakedTexturesForSkinnedMeshes;

	const bool bIsPersistent = false;
	const bool bLoadIfFindFails = true;
	FMemoryWriter memoryWriter(saveDataBytes, bIsPersistent);
	FDonMeshPaintArchive archive(memoryWriter, bLoadIfFindFails);

	archive << dataContainer;

	return saveDataBytes;
}

void UDonMeshPaintSaveLoadHelper::LoadMeshPaintFromBytes(UObject* WorldContextObject, const TArray<uint8>& SaveData, const bool bFlushAllPaint /*= true*/)
{
	if (!SaveData.Num())
		return;	

	// Step 1. Fetch our data back from the saved bytes
	FDonMeshPaintSaveData dataContainer;

	const bool bIsPersistent = false;
	const bool bLoadIfFindFails = true;
	FMemoryReader memoryReader(SaveData, bIsPersistent);
	FDonMeshPaintArchive archive(memoryReader, bLoadIfFindFails);
	archive << dataContainer;

	// Step 2.  Clear all existing paint textures (only relevant for quick load - i.e. loads that are called without restarting the map via OpenLevel)
	auto globalPainter = UDonMeshPaintingHelper::GetGlobalManager(WorldContextObject);
	if (!ensure(globalPainter))
		return;	

	globalPainter->EnsureInitialSetup();

	if(bFlushAllPaint)
		globalPainter->FlushAllPaint();

	// Step 3. Replay the paint strokes:
	for (const auto& actorUnit : dataContainer.Data)
	{
		if (!actorUnit.Actor)
		{
			UE_LOG(LogDonMeshPainting, Error, TEXT("[Loading] Unable to find Actor %s in this world! Paint data will not be loaded for this actor."), *actorUnit.AuditLog.ToString());
			continue;
		}

		// Create or fetch the painter component for this actor:
		auto painterComponent = globalPainter->GetMeshPainterComponent(actorUnit.Actor);
		if (!ensure(painterComponent))
		{
			UE_LOG(LogDonMeshPainting, Error, TEXT("[Loading] Unable to find component while loading paint strokes for Actor %s. Please ensure all components have been loaded for this actor."));
			continue;
		}

		LoadSaveDataForActor(painterComponent, actorUnit, dataContainer.bNeedsBakedTexturesForSkinnedMeshes);
	}
}

void UDonMeshPaintSaveLoadHelper::LoadSaveDataForActor(UDonMeshPainterComponent* PainterComponent, const FDonMeshPaintSaveDataActorUnit& ActorUnit, const bool bNeedsBakedTexturesForSkinnedMeshes)
{
	if (!ensure(PainterComponent))
		return;

	PainterComponent->Flush();
	PainterComponent->LoadPaintStrokes(ActorUnit.PaintStrokesPerPrimitive);
	PainterComponent->ProcessPaintStrokes(true /*bIsSaveLoadCycle*/, bNeedsBakedTexturesForSkinnedMeshes);

	if (bNeedsBakedTexturesForSkinnedMeshes)
		PainterComponent->LoadBakedPaintData(ActorUnit.BakedPaintData);
}

void UDonMeshPaintSaveLoadHelper::LoadMeshPaintOntoMultipleActors(UObject* WorldContextObject, const TArray<uint8>& SaveData, TArray<AActor*> ActorsToPaint)
{
	if (!SaveData.Num())
		return;

	// Fetch our data back from the saved bytes
	FDonMeshPaintSaveData dataContainer;

	const bool bIsPersistent = false;
	const bool bLoadIfFindFails = true;
	FMemoryReader memoryReader(SaveData, bIsPersistent);
	FDonMeshPaintArchive archive(memoryReader, bLoadIfFindFails);
	archive << dataContainer;

	auto globalPainter = UDonMeshPaintingHelper::GetGlobalManager(WorldContextObject);
	if (!ensure(globalPainter))
		return;

	if (!ensure(dataContainer.Data.Num()))
		return;

	if (dataContainer.bNeedsBakedTexturesForSkinnedMeshes)
	{
		FString message = FString("Baked textures detected while loading a saved design onto multiple actors! This usecase should never require baked textures (which require far more disk space to save)\n");
		message += "Please save your actor with the 'Needs Baked Textures For Skinned Meshes' option set to false";
		UE_LOG(LogDonMeshPainting, Warning, TEXT("%s"), *message);
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, FString::Printf(TEXT("%s"), *message));
		return;
	}

	const auto& actorTemplate = dataContainer.Data[0];	

	for(auto actor : ActorsToPaint)
	{
		if (!actor)
			continue;

		auto actorUnit = actorTemplate;
		for (auto& paintStrokes : actorUnit.PaintStrokesPerPrimitive)
		{
			for (auto component : actor->GetComponents())
			{
				if (component->GetFName().IsEqual(paintStrokes.PaintableName))
				{
					paintStrokes.Paintable = component;
					break;
				}
				else
					UE_LOG(LogTemp, Verbose, TEXT("%s != %s"), *component->GetFName().ToString(), *paintStrokes.PaintableName.ToString());
			}
		}		

		auto painterComponent = globalPainter->GetMeshPainterComponent(actor);
		if (!ensure(painterComponent))
			continue;

		const bool bNeedsBakedTexturesForSkinnedMeshes = false; // Character customization and mass-stamping usecases should never need baked textures. Even for skeletal meshes, stamping will always be pose independent.
		LoadSaveDataForActor(painterComponent, actorUnit, dataContainer.bNeedsBakedTexturesForSkinnedMeshes);		
	}
}