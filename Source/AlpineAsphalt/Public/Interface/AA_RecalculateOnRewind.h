// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AA_RecalculateOnRewind.generated.h"

class AA_RecalculateOnRewind;
class UAA_RewindSubsystem;

UCLASS()
class ALPINEASPHALT_API UAA_RecalculateOnRewindDelegateHolder : public UObject
{
	GENERATED_BODY()

public:
	void SetParent(AA_RecalculateOnRewind& InParent) { Parent = &InParent; }

	void Register(UAA_RewindSubsystem& Subsystem);
	void Deactivate(UAA_RewindSubsystem& Subsystem);
private:

	UFUNCTION()
	void OnRewindModeDeactivated();

	UFUNCTION()
	void OnRewindPercentageChanged(float Percentage);

private:
	AA_RecalculateOnRewind* Parent{};
	bool bWasRewound{};
};

/**
 * 
 */
class ALPINEASPHALT_API AA_RecalculateOnRewind
{

public:
	virtual ~AA_RecalculateOnRewind() = default;

protected:

	void RegisterRewindCallback();
	void UnregisterRewindCallback();

	virtual void RecalculateOnRewind() = 0;
	virtual UObject* AsUObject() = 0;

private:
	friend class UAA_RecalculateOnRewindDelegateHolder;
	UAA_RecalculateOnRewindDelegateHolder* Holder{};
};
