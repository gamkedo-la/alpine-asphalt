// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AA_RacerContextProvider.h"

#include "AA_RacerVerbalBarksComponent.generated.h"

class AAA_WheeledVehiclePawn;
class AAA_PlayerController;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_RacerVerbalBarksComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_RacerVerbalBarksComponent();

	UFUNCTION()
	void OnStuck(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void CheckRelativePlayerPosition();

private:

	UPROPERTY(Transient)
	TObjectPtr<AAA_PlayerController> PlayerController{};

	IAA_RacerContextProvider* RacerContextProvider{};

	TArray<bool> PlayerPositionChanges{};
};
