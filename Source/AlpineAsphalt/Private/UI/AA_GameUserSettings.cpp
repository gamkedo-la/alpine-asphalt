// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AA_GameUserSettings.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"

using namespace AA;

void UAA_GameUserSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	UE_LOG(LogAlpineAsphalt, Display, TEXT("%s: ApplySettings: bCheckForCommandLineOverrides=%s"), *GetName(), LoggingUtils::GetBoolString(bCheckForCommandLineOverrides));

	Super::ApplySettings(bCheckForCommandLineOverrides);

	OnGameUserSettingsUpdated.Broadcast();
}