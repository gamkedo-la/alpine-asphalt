// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AA_TimeTrialScoreScreenUI.generated.h"

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_TimeTrialScoreScreenUI : public UUserWidget
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void AddDriverScore(const FString& Name, float Time, bool ShouldHighlight = false);
};
