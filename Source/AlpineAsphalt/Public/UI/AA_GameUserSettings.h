// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "AI/AA_AIDifficulty.h"

#include "AA_GameUserSettings.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameUserSettingsUpdated);

/**
 * 
 */
UCLASS(BlueprintType, config = GameUserSettings, configdonotcheckdefaults)
class ALPINEASPHALT_API UAA_GameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, DisplayName = "Get AA GameUserSettings", Category = GameUserSettings)
	static UAA_GameUserSettings* GetInstance();

	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	UFUNCTION(BlueprintCallable)
	void SetAIDifficulty(EAA_AIDifficulty Difficulty);

	UFUNCTION(BlueprintPure)
	EAA_AIDifficulty GetAIDifficulty() const;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnGameUserSettingsUpdated OnGameUserSettingsUpdated {};

private:
	
	// TODO: Defaulting to Hard as AI isn't finished
	UPROPERTY(Config)
	EAA_AIDifficulty AIDifficulty{ EAA_AIDifficulty::Hard };
};

#pragma region Inline Definitions

inline UAA_GameUserSettings* UAA_GameUserSettings::GetInstance()
{
	return Cast< UAA_GameUserSettings>(GetGameUserSettings());
}

inline void UAA_GameUserSettings::SetAIDifficulty(EAA_AIDifficulty Difficulty)
{
	AIDifficulty = Difficulty;
}

inline EAA_AIDifficulty UAA_GameUserSettings::GetAIDifficulty() const
{
	return AIDifficulty;
}

#pragma endregion Inline Definitions