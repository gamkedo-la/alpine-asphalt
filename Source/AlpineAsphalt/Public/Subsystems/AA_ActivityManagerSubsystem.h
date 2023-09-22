#pragma once

#include "CoreMinimal.h"
#include "Interface/AA_ActivityInterface.h"
#include "AA_ActivityManagerSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(ActivityManagerSubsystem, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLoadActivityCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDestroyActivityCompleted);

UCLASS(BlueprintType)
class UAA_ActivityManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UAA_ActivityManagerSubsystem();
	
	void LaunchActivity(IAA_ActivityInterface* Activity);
	
	UFUNCTION()
	bool CanLaunchActivity() const;

	UFUNCTION(Exec)
	void EndActivity();

	FLoadActivityCompleted OnLoadActivityCompleted;
	FDestroyActivityCompleted OnDestroyActivityCompleted;
protected:

	UFUNCTION()
	void StartActivity();

	UFUNCTION()
	void EndActivityFinished();

private:
	
	UPROPERTY()
	TScriptInterface<IAA_ActivityInterface> CurrentActivity;
	
};
