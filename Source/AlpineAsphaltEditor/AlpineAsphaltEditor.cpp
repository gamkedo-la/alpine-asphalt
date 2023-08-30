// Fill out your copyright notice in the Description page of Project Settings.

#include "AlpineAsphaltEditor.h"

#include "AA_CheckpointVisualizer.h"
#include "UnrealEdGlobals.h"
#include "Components/AA_CheckpointComponent.h"
#include "Editor/UnrealEdEngine.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_GAME_MODULE(FAlpineAsphaltEditorModule, AlpineAsphaltEditor);

void FAlpineAsphaltEditorModule::StartupModule()
{
	IModuleInterface::StartupModule();

	const TSharedPtr<AA_CheckpointVisualizer> Visualizer = MakeShareable(new AA_CheckpointVisualizer);
	if(Visualizer.IsValid())
	{
		GUnrealEd->RegisterComponentVisualizer(UAA_CheckpointComponent::StaticClass()->GetFName(),Visualizer);
		Visualizer->OnRegister();
	}
}

void FAlpineAsphaltEditorModule::ShutdownModule()
{
	IModuleInterface::ShutdownModule();
	if(GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UAA_CheckpointComponent::StaticClass()->GetFName());
	}
}
