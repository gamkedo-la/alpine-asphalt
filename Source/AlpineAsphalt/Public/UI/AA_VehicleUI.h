// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AA_ExtendedCommonActivatableWidget.h"
#include "Blueprint/UserWidget.h"
#include "AA_VehicleUI.generated.h"

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_VehicleUI : public UAA_ExtendedCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void StartCountdown();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void StartTimer();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void HideTimer();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void UpdateSpeed(float NewSpeed);

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void UpdateGear(int NewGear);
};
