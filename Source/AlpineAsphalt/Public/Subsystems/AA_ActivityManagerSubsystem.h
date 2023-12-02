#pragma once

#include "CoreMinimal.h"
#include "AA_ActivityManagerSubsystem.generated.h"

class UAA_BaseActivity;
DECLARE_LOG_CATEGORY_EXTERN(ActivityManagerSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLoadActivityCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDestroyActivityCompleted);

UCLASS(BlueprintType)
class UAA_ActivityManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UAA_ActivityManagerSubsystem();
	
	void LaunchActivity(UAA_BaseActivity* Activity, bool RestartingActivity = false);

	void LoadScreenMinimumCompleted(bool LoadingActivity);
	
	UFUNCTION(BlueprintPure)
	bool CanLaunchActivity() const;

	UFUNCTION(BlueprintPure)
	bool IsActivityActive() const;

	UFUNCTION(BlueprintPure)
	bool IsLoadingScreenActive() const;

	UFUNCTION(BlueprintPure)
	bool IsActivityActiveButPlayerCompleted() const;

	UFUNCTION(BlueprintCallable,Exec)
	void DestroyActivity();

	FLoadActivityCompleted OnLoadActivityCompleted;
	FDestroyActivityCompleted OnDestroyActivityCompleted;
protected:

	UFUNCTION()
	void StartActivity();

	UFUNCTION(BlueprintCallable)
	void RestartActivity();

	UFUNCTION()
	void DestroyActivityFinished();

private:
	
	UPROPERTY()
	UAA_BaseActivity* CurrentActivity;

	UPROPERTY()
	float MinimumLoadScreenTime = 2.f;

	bool LoadScreenFinished = false;
	bool ActivityLoaded = false;
};

inline bool UAA_ActivityManagerSubsystem::IsActivityActive() const
{
	return CurrentActivity != nullptr;
}

inline bool UAA_ActivityManagerSubsystem::IsLoadingScreenActive() const
{
	return !IsActivityActive() && !LoadScreenFinished;
}
