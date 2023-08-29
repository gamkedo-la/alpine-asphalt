// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AA_BlueprintFunctionLibrary_CPP.generated.h"

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_BlueprintFunctionLibrary_CPP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,Category=LandscapeSpline)
	static void GenerateRoadSpline(class AActor* LandscapeSpline,TSubclassOf<AAA_RoadSpline_CPP> RoadSplineBP);

	UFUNCTION(BlueprintCallable,Category=RaceSpline)
	static void GenerateRaceSpline(TArray<class AActor*> RoadSplines, TSubclassOf<AAA_RaceSpline_CPP> RaceSplineBP);
	
};