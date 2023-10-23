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

	UFUNCTION(BlueprintCallable)
	void SetAutomaticShifting(bool AutomaticShiftingEnabled);

	UFUNCTION(BlueprintCallable)
	void SetABSBrakesEnabled(bool bABSBrakesEnabled);

	UFUNCTION(BlueprintCallable)
	void SetTractionControlEnabled(bool bTractionControlEnabled);

	UFUNCTION(BlueprintCallable)
	void SetRewindTimeEnabled(bool bRewindTimeEnabled);

	UFUNCTION(BlueprintCallable)
	void SetSpeedUnitKPH(bool bSpeedUnitKPH);

	UFUNCTION(BlueprintPure)
	EAA_AIDifficulty GetAIDifficulty() const;
	
	UFUNCTION(BlueprintPure)
	bool GetAutomaticShifting() const;
	
	UFUNCTION(BlueprintPure)
	bool GetABSBrakesEnabled() const;
	
	UFUNCTION(BlueprintPure)
	bool GetTractionControlEnabled() const;
	
	UFUNCTION(BlueprintPure)
	bool GetRewindTimeEnabled() const;
	
	UFUNCTION(BlueprintPure)
	bool GetSpeedUnitKPH() const;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnGameUserSettingsUpdated OnGameUserSettingsUpdated {};

private:
	
	// TODO: Defaulting to Hard as AI isn't finished
	UPROPERTY(Config)
	EAA_AIDifficulty AIDifficulty{ EAA_AIDifficulty::Hard };

	UPROPERTY(Config)
	bool IsAutomaticShifting = true;

	UPROPERTY(Config)
	bool ABSBrakesEnabled = true;
	
	UPROPERTY(Config)
	bool TractionControlEnabled = true;

	UPROPERTY(Config)
	bool RewindTimeEnabled = true;

	UPROPERTY(Config)
	bool SpeedUnitKPH = true;
	
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

inline void UAA_GameUserSettings::SetAutomaticShifting(bool bIsAutomaticShifting)
{
	IsAutomaticShifting = bIsAutomaticShifting;
}

inline void UAA_GameUserSettings::SetABSBrakesEnabled(bool bABSBrakesEnabled)
{
	ABSBrakesEnabled = bABSBrakesEnabled;
}

inline void UAA_GameUserSettings::SetTractionControlEnabled(bool bTractionControlEnabled)
{
	TractionControlEnabled = bTractionControlEnabled;
}

inline void UAA_GameUserSettings::SetRewindTimeEnabled(bool bRewindTimeEnabled)
{
	RewindTimeEnabled = bRewindTimeEnabled;
}
inline void UAA_GameUserSettings::SetSpeedUnitKPH(bool bSpeedUnitKPH)
{
	SpeedUnitKPH = bSpeedUnitKPH;
}
inline EAA_AIDifficulty UAA_GameUserSettings::GetAIDifficulty() const
{
	return AIDifficulty;
}
inline bool UAA_GameUserSettings::GetAutomaticShifting() const
{
	return IsAutomaticShifting;
}
inline bool UAA_GameUserSettings::GetABSBrakesEnabled() const
{
	return ABSBrakesEnabled;
}
inline bool UAA_GameUserSettings::GetTractionControlEnabled() const
{
	return TractionControlEnabled;
}
inline bool UAA_GameUserSettings::GetRewindTimeEnabled() const
{
	return RewindTimeEnabled;
}
inline bool UAA_GameUserSettings::GetSpeedUnitKPH() const
{
	return SpeedUnitKPH;
}

#pragma endregion Inline Definitions