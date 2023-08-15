// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AA_RoadSpline_CPP.generated.h"

UCLASS()
class ALPINEASPHALT_API AAA_RoadSpline_CPP : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAA_RoadSpline_CPP();

	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	class USplineComponent* RoadSpline;

	//Road width at each segment
	UPROPERTY(BlueprintReadOnly)
	TArray<float> RoadWidth;
};
