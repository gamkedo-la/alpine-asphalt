#pragma once
#include "AA_BaseActivity.h"
#include "Engine/DataTable.h"
#include "AA_HeadToHeadActivity.generated.h"


class UAA_VehicleDataAsset;
class AAA_WheeledVehiclePawn;
class UAA_TimeTrialScoreScreenUI;
class AAA_TrackInfoActor;

UCLASS(Blueprintable)
class UAA_HeadToHeadActivity : public UAA_BaseActivity
{
	
	GENERATED_BODY()
	
public:
	//Initialize Activity with all needed info
	UFUNCTION()
	virtual void Initialize(AAA_TrackInfoActor* Track) override;
	
	//loads the activity and all needed actors
	UFUNCTION()
	virtual void LoadActivity() override;

	void CountdownEnded();
	//starts the activity
	UFUNCTION()
	virtual void StartActivity() override;

	//destroys the activity
	UFUNCTION()
	virtual void DestroyActivity() override;

	void ReplayStartDelayEnded();
	//call when TimeTrial ends
	UFUNCTION()
	virtual void RaceEnded();

	UFUNCTION()
	virtual UWorld* GetWorld() const override;
protected:
	UFUNCTION()
	void CheckpointHit(int IndexCheckpointHit, AAA_WheeledVehiclePawn* HitVehicle);

private:

	UPROPERTY(EditDefaultsOnly)
	TArray<UAA_VehicleDataAsset*> VehiclesToSpawn;
	
	UPROPERTY()
	AAA_TrackInfoActor* Track;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAA_TimeTrialScoreScreenUI> ScoreScreenClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAA_WheeledVehiclePawn> VehicleClass;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* DriverTable;

	UPROPERTY()
	TArray<AAA_WheeledVehiclePawn*> AIRacers;
	
	UPROPERTY()
	int LastCheckpointHitIndex = -1;

	UPROPERTY()
	int NumCheckpoints = -1;

	UPROPERTY()
	float FinishDelay = 1.f;
	
	UPROPERTY()
	float ReplayStartDelay = .2f;

	UPROPERTY()
	float StartTime;
	
	UPROPERTY()
	float FinishTime;
	
};
