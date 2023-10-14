// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_AIRacerEvents.h"

#include "AA_AIGetUnstuckComponent.generated.h"

class IAA_RacerContextProvider;
class AAA_WheeledVehiclePawn;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALPINEASPHALT_API UAA_AIGetUnstuckComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAA_AIGetUnstuckComponent();

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleStuck OnVehicleStuck {};


protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ResetBuffer();
	FVector CalculateIdealSeekPosition(const AAA_WheeledVehiclePawn& VehiclePawn) const;

private:

	struct FStuckState
	{
		FVector Position{ EForceInit::ForceInitToZero };
		int8 ThrottleSign : 1 {};
	};

	IAA_RacerContextProvider* RacerContextProvider{};
	TUniquePtr<TCircularBuffer<FStuckState>> PositionsPtr{};

	UPROPERTY(Category = "Detection", EditAnywhere)
	float MinStuckTime{ 3.0f };

	UPROPERTY(Category = "Detection", EditAnywhere)
	float MinAverageSpeed{ 100.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float UnstuckSeekOffset{ 1000.0f };

	int32 NextBufferIndex{};
	bool bSufficientSamples{};
};
