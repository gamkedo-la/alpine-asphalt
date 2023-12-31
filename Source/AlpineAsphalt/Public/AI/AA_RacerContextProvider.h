// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "AA_AIRacerContext.h"

#include "AA_RacerContextProvider.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, NotBlueprintable)
class UAA_RacerContextProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ALPINEASPHALT_API IAA_RacerContextProvider
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Racer Context")
	virtual FAA_AIRacerContext& GetRacerContext() = 0;

	const FAA_AIRacerContext& GetRacerContext() const { return const_cast<IAA_RacerContextProvider*>(this)->GetRacerContext(); }
};
