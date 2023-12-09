#pragma once
#include "AA_BaseActivity.h"
#include "AA_TimeTrialActivity.h"
#include "Engine/DataTable.h"
#include "Interface/AA_BaseRewindable.h"

#include "AA_HeadToHeadActivity.generated.h"


class UAA_VehicleDataAsset;
class AAA_WheeledVehiclePawn;
class UAA_TimeTrialScoreScreenUI;
class AAA_TrackInfoActor;
class AAA_PlayerController;

namespace AA_HeadToHeadActivity
{
	struct FSnapshotData
	{
		TArray<float, TInlineAllocator<8>> FinishTimes;
		TMap<AAA_WheeledVehiclePawn*, int, TInlineSetAllocator<8>> LapsCompletedMap;
		float StartTime;
		float FinishTime;
		int LastCheckpointHitIndex;
		int IndexActiveCheckpoint;
	};
}

UCLASS(Blueprintable)
class UAA_HeadToHeadActivity : public UAA_BaseActivity, public TAA_BaseRewindable<AA_HeadToHeadActivity::FSnapshotData>
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

	virtual bool IsPlayerCompleted() const override;
protected:
	UFUNCTION()
	void CheckpointHit(int IndexCheckpointHit, AAA_WheeledVehiclePawn* HitVehicle);

	// Inherited from TAA_BaseRewindable

	virtual AA_HeadToHeadActivity::FSnapshotData CaptureSnapshot() const override;
	virtual void RestoreFromSnapshot(const AA_HeadToHeadActivity::FSnapshotData& InSnapshotData, float InRewindTime) override;
	virtual UObject* AsUObject() override { return this; }

private:
	UFUNCTION()
	void AddAIDriverScore(float TimeScore);

	void HideRaceUIElements(AAA_PlayerController* PlayerController);

	void RegisterRacePositionTimer();
	void UnRegisterRacePositionTimer();
	void UpdatePlayerHUD();

	void LoadDriverNames();
	void UpdatePlayerLapsUI();

	UPROPERTY()
	TArray<FDriverName> DriverNames;

	UPROPERTY()
	TMap<AAA_WheeledVehiclePawn*, int> LapsCompletedMap;

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
	UAA_TimeTrialScoreScreenUI* ScoreScreen;

	UPROPERTY()
	TArray<float> FinishTimes;

	UPROPERTY()
	int LastCheckpointHitIndex = -1;

	UPROPERTY()
	int NumCheckpoints = -1;

	UPROPERTY()
	int IndexActiveCheckpoint;

	UPROPERTY()
	float FinishDelay = 1.f;

	UPROPERTY()
	float ReplayStartDelay = .2f;

	UPROPERTY()
	float StartTime;

	UPROPERTY()
	bool RaceFinished = false;

	UPROPERTY()
	float FinishTime;

	UPROPERTY()
	FTimerHandle RaceHUDUpdateTimer{};

	UPROPERTY(EditDefaultsOnly)
	float RaceHUDUpdateFrequency{ 1.0f };

	// Sometimes AI will hit the checkpoint before last and it will detect an early finish so this is a workaround to avoid that.
	UPROPERTY(EditDefaultsOnly)
	float MinRaceTimeForFinish{ 10.0f };
};
