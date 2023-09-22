#pragma once
#include "Interface/AA_ActivityInterface.h"
#include "AA_TimeTrialActivity.generated.h"


class AAA_TrackInfoActor;

UCLASS()
class UAA_TimeTrialActivity : public UObject, public IAA_ActivityInterface
{
	
	GENERATED_BODY()
	
public:
	//Initialize Activity with all needed info
	UFUNCTION()
	virtual void Initialize(AAA_TrackInfoActor* Track);
	
	//loads the activity and all needed actors
	UFUNCTION()
	virtual void LoadActivity() override;

	//starts the activity
	UFUNCTION()
	virtual void StartActivity() override;

	//destroys the activity
	UFUNCTION()
	virtual void DestroyActivity() override;

	//call when TimeTrial ends
	UFUNCTION()
	virtual void RaceEnded();

	UFUNCTION()
	virtual UWorld* GetWorld() const override;
protected:
	UFUNCTION()
	void CheckpointHit(int IndexCheckpointHit);

private:
	UPROPERTY()
	AAA_TrackInfoActor* Track;

	UPROPERTY()
	int LastCheckpointHitIndex = -1;

	UPROPERTY()
	int NumCheckpoints = -1;

	UPROPERTY()
	float FinishDelay = 1.f;

	UPROPERTY()
	float StartTime;
	
	UPROPERTY()
	float FinishTime;
	
};
