// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AA_ExtendedCommonActivatableWidget.h"
#include "AA_TimeTrialScoreScreenUI.generated.h"

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_TimeTrialScoreScreenUI : public UAA_ExtendedCommonActivatableWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void AddDriverScore(const FString& Name, float Time, bool ShouldHighlight = false);
};
