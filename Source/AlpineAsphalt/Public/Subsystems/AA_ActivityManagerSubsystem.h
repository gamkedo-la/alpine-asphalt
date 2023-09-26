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
	
	void LaunchActivity(UAA_BaseActivity* Activity);

	void LoadScreenMinimumCompleted(bool LoadingActivity);
	
	UFUNCTION()
	bool CanLaunchActivity() const;

	UFUNCTION(BlueprintCallable,Exec)
	void DestroyActivity();

	FLoadActivityCompleted OnLoadActivityCompleted;
	FDestroyActivityCompleted OnDestroyActivityCompleted;
protected:

	UFUNCTION()
	void StartActivity();

	UFUNCTION()
	void DestroyActivityFinished();

private:
	
	UPROPERTY()
	UAA_BaseActivity* CurrentActivity;

	UPROPERTY()
	float MinimumLoadScreenTime = 1.f;

	bool LoadScreenFinished = false;
	bool ActivityLoaded = false;
};
