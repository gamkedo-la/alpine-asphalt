// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_RacerSplineFollowingComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_RacerSplineFollowingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_RacerSplineFollowingComponent();

protected:
	virtual void BeginPlay() override;
		
};
