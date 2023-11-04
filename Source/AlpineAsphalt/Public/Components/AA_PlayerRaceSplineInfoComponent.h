// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include <optional>

#include "AA_PlayerRaceSplineInfoComponent.generated.h"

class AAA_TrackInfoActor;
class AAA_WheeledVehiclePawn;

USTRUCT(BlueprintType)
struct FPlayerSplineInfo
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadOnly)
	float DistanceAlongSpline{};

	UPROPERTY(Transient, BlueprintReadOnly)
	float SplineLength{};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_PlayerRaceSplineInfoComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_PlayerRaceSplineInfoComponent();

	void SetTrackInfo(AAA_TrackInfoActor* InTrackInfoActor);
	void SetVehicle(const AAA_WheeledVehiclePawn* InVehicle);

	std::optional<FPlayerSplineInfo> GetPlayerSplineInfo() const { return PlayerSplineInfo; }

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateSplineInfo();

private:
	UPROPERTY(Transient)
	TObjectPtr<AAA_TrackInfoActor> TrackInfoActor{};

	UPROPERTY(Transient)
	TObjectPtr<const AAA_WheeledVehiclePawn> Vehicle{};

	std::optional<FPlayerSplineInfo> PlayerSplineInfo{};
};
