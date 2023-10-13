// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_RacerObstacleAvoidanceComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_RacerObstacleAvoidanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_RacerObstacleAvoidanceComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
};
