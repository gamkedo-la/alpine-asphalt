// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AA_AIRacerEvents.h"

#include "AA_ObstacleDetectionComponent.generated.h"

class IAA_RacerContextProvider;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_ObstacleDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_ObstacleDetectionComponent();

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleObstaclesUpdated OnVehicleObstaclesUpdated {};

private:
	IAA_RacerContextProvider* RacerContextProvider{};
};
