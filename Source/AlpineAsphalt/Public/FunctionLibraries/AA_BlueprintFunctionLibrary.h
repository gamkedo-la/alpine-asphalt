// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AA_BlueprintFunctionLibrary.generated.h"

class AAA_TrackInfoActor;
class AAA_RoadSplineActor;
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_BlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable,Category=LandscapeSpline)
	static void GenerateRoadSpline(class AActor* LandscapeSpline,TSubclassOf<AAA_RoadSplineActor> RoadSplineBP);

	UFUNCTION(BlueprintCallable,Category=RaceSpline)
	static void GenerateRaceSpline(TArray<class AActor*> RoadSplines, TSubclassOf<AAA_TrackInfoActor> RaceSplineBP);
	
};
