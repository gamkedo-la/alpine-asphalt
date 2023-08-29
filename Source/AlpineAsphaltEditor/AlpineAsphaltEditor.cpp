// Fill out your copyright notice in the Description page of Project Settings.

#include "AlpineAsphaltEditor.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_GAME_MODULE(FAlpineAsphaltEditorModule, AlpineAsphaltEditor);

void FAlpineAsphaltEditorModule::StartupModule()
{
	IModuleInterface::StartupModule();
}

void FAlpineAsphaltEditorModule::ShutdownModule()
{
	IModuleInterface::ShutdownModule();
}
