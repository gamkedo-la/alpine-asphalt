// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AA_RoadSplineActor.generated.h"

class USplineMeshComponent;

UCLASS()
class ALPINEASPHALT_API AAA_RoadSplineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAA_RoadSplineActor();
	
	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	class USplineComponent* Spline;

};
