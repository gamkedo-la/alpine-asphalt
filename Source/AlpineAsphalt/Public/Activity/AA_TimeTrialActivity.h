#pragma once
#include "AA_BaseActivity.h"
#include "Engine/DataTable.h"
#include "Interface/AA_BaseRewindable.h"

#include "AA_TimeTrialActivity.generated.h"


class UAA_TimeTrialScoreScreenUI;
class AAA_TrackInfoActor;

namespace AA_TimeTrialActivity
{
	struct FSnapshotData
	{
		int PlayerLapCounter;
		int LastCheckpointHitIndex;
		float StartTime;
		float FinishTime;
		int IndexActiveCheckpoint;
	};
}

USTRUCT(BlueprintType)
struct FDriverName : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString DriverName;
};
UCLASS(Blueprintable)
class UAA_TimeTrialActivity : public UAA_BaseActivity, public TAA_BaseRewindable<AA_TimeTrialActivity::FSnapshotData>
{
	
	GENERATED_BODY()
	
public:
	//Initialize Activity with all needed info
	UFUNCTION()
	virtual void Initialize(AAA_TrackInfoActor* Track) override;
	
	//loads the activity and all needed actors
	UFUNCTION()
	virtual void LoadActivity() override;

	void UpdatePlayerHUD() const;
	void CountdownEnded();
	//starts the activity
	UFUNCTION()
	virtual void StartActivity() override;

	//destroys the activity
	UFUNCTION()
	virtual void DestroyActivity() override;

	void ReplayStartDelayEnded();
	void HideRaceUIElements();
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
	virtual AA_TimeTrialActivity::FSnapshotData CaptureSnapshot() const override;
	virtual void RestoreFromSnapshot(const AA_TimeTrialActivity::FSnapshotData& InSnapshotData, float InRewindTime) override;
	virtual UObject* AsUObject() override { return this; }

private:
	void UpdatePlayerLapsUI();

private:
	UPROPERTY()
	AAA_TrackInfoActor* Track;

	UPROPERTY()
	int PlayerLapCounter = 0;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAA_TimeTrialScoreScreenUI> ScoreScreenClass;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* DriverTable;
	
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
	float FinishTime;

	UPROPERTY()
	FTimerHandle RaceHUDUpdateTimer{};

	UPROPERTY(EditDefaultsOnly)
	float RaceHUDUpdateFrequency{ 1.0f };

	bool bScoreScreenActive{};
};
