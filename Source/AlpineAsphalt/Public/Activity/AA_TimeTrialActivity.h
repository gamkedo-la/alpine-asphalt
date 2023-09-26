#pragma once
#include "AA_BaseActivity.h"
#include "Engine/DataTable.h"
#include "AA_TimeTrialActivity.generated.h"


class UAA_TimeTrialScoreScreenUI;
class AAA_TrackInfoActor;

USTRUCT(BlueprintType)
struct FDriverName : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FString DriverName;
};
UCLASS(Blueprintable)
class UAA_TimeTrialActivity : public UAA_BaseActivity
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

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAA_TimeTrialScoreScreenUI> ScoreScreenClass;

	UPROPERTY(EditDefaultsOnly)
	UDataTable* DriverTable;
	
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
