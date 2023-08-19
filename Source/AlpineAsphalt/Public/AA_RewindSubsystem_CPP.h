// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AA_RewindSubsystem_CPP.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(RewindSubsystem, Log, All);

//Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRewindModeActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRewindModeDeactivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCurrentRewindPercentageValueChanged,float,Percentage);

/**
 * 
 */
UCLASS(BlueprintType)
class ALPINEASPHALT_API UAA_RewindSubsystem_CPP : public UTickableWorldSubsystem
{
	GENERATED_BODY()

private:

	//whether or not player is currently rewinding time
	bool RewindModeActive = false;

	float CurrentRewindValue = 0.f;

	float MaxRewindValue = 0.f;

	TArray<class IAA_Rewindable_CPP*> RewindableActors;

public:
	//How Often to record snapshot
	const float RecordingResolution = .1f;

	const float MaxRecordingLength = 100.f;
	
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	void RegisterRewindable(IAA_Rewindable_CPP* Rewindable);
	void Rewind(float AmountToRewind);
	void FastForward(float AmountToFastForward);
	void EnterRewindMode();
	void CancelRewindMode();
	void ConfirmRewind();
	bool IsRewindModeActive();

	UPROPERTY(BlueprintAssignable)
	FRewindModeActivated ActivatedDelegate;

	UPROPERTY(BlueprintAssignable)
	FRewindModeDeactivated DeactivatedDelegate;

	UPROPERTY(BlueprintAssignable)
	FCurrentRewindPercentageValueChanged CurrentRewindValueChangedDelegate;
};
