// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Caustic.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FCausticModule"

void FCausticModule::StartupModule()
{
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("Caustic"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/Caustic"), PluginShaderDir);
}

void FCausticModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FCausticModule, Caustic)