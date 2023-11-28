// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AA_RewindSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(RewindSubsystem, Log, All);

//Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRewindModeActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRewindModeDeactivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCurrentRewindPercentageValueChanged,float,Percentage);

/**
 * 
 */
UCLASS(BlueprintType)
class ALPINEASPHALT_API UAA_RewindSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

private:
	bool RewindModeEnabled = true;
	//whether or not player is currently rewinding time
	bool RewindModeActive = false;

	float CurrentRewindValue = 0.f;

	float RewindStartTime = -1.f;

	TArray<class IAA_RewindableInterface*> RewindableActors;

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	float GetMaxRewindTime();
	//How Often to record snapshot
	const float RecordingResolution = .1f;

	const float MaxRecordingLength = 100.f;
	
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	void RegisterRewindable(IAA_RewindableInterface* Rewindable);
	void UnregisterRewindable(IAA_RewindableInterface* Rewindable);
	void Rewind(float AmountToRewind);
	void FastForward(float AmountToFastForward);
	bool EnterRewindMode();
	void CancelRewindMode();
	void ConfirmRewind();
	void ResetRewindHistory();

	UFUNCTION(BlueprintPure)
	bool IsRewindModeActive() const;

	UFUNCTION(BlueprintCallable)
	bool CanEnterRewindMode() const;
	
	UFUNCTION(BlueprintCallable)
	void EnableRewindMode(bool bisEnabled);

	UPROPERTY(BlueprintAssignable)
	FRewindModeActivated ActivatedDelegate;

	UPROPERTY(BlueprintAssignable)
	FRewindModeDeactivated DeactivatedDelegate;

	UPROPERTY(BlueprintAssignable)
	FCurrentRewindPercentageValueChanged CurrentRewindValueChangedDelegate;
};
