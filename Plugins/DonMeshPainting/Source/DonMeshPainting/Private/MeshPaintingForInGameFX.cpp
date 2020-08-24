// Copyright(c) 2017 Venugopalan Sreedharan. All rights reserved.

#include "DonMeshPaintingPrivatePCH.h"

DEFINE_LOG_CATEGORY(LogDonMeshPainting);

class FDonMeshPainting : public IDonMeshPainting
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FDonMeshPainting, DonMeshPainting )



void FDonMeshPainting::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FDonMeshPainting::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}



