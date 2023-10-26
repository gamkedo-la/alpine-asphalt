// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "MapCaptureComponent.generated.h"

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UMapCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()

public:

	UMapCaptureComponent();

	UFUNCTION(BlueprintCallable)
	void SetCaptureEnabled(bool bEnabled);

protected:
	virtual void InitializeComponent() override;
};
