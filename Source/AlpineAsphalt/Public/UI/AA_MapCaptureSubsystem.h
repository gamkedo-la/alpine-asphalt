// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AA_MapCaptureSubsystem.generated.h"

class AAA_MapCaptureActor;

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_MapCaptureSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Map)
	void SetCaptureEnabled(bool bCaptureEnabled);

	UFUNCTION(BlueprintPure, Category = Map)
	bool HasValidCaptureTarget() const;

protected:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	void FindAndSetCaptureActor(UWorld& InWorld);

private:
	UPROPERTY(Transient)
	TObjectPtr<AAA_MapCaptureActor> CaptureActor{};
};

#pragma region Inline Definitions

inline bool UAA_MapCaptureSubsystem::HasValidCaptureTarget() const
{
	return CaptureActor != nullptr;
}

#pragma endregion Inline Definitions
