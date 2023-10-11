// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_AIVehicleControlComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_AIVehicleControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_AIVehicleControlComponent();

protected:
	virtual void BeginPlay() override;
};
