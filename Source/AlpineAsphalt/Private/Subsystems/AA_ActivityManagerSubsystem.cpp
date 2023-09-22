#include "Subsystems/AA_ActivityManagerSubsystem.h"

#include "Controllers/AA_PlayerController.h"
#include "Interface/AA_ActivityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AA_VehicleUI.h"

DEFINE_LOG_CATEGORY(ActivityManagerSubsystem);

UAA_ActivityManagerSubsystem::UAA_ActivityManagerSubsystem()
{
	OnLoadActivityCompleted.AddDynamic(this,&UAA_ActivityManagerSubsystem::StartActivity);
	OnDestroyActivityCompleted.AddDynamic(this,&UAA_ActivityManagerSubsystem::DestroyActivityFinished);
}

void UAA_ActivityManagerSubsystem::LaunchActivity(IAA_ActivityInterface* Activity)
{
	if(!Activity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("%hs: Can't Launch Activity: Activity was nullptr"),__func__)
		return;
	}
	if(CurrentActivity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("%hs: Can't Launch Activity: Activity already taking place"),__func__)
		return;
	}
	//Store activity
	CurrentActivity.SetInterface(Activity);
	CurrentActivity.SetObject(Cast<UObject>(Activity));
	
	//Show Load Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->ShowLoadingScreen();

	//Show Load Screen for minimum time
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this,&UAA_ActivityManagerSubsystem::LoadScreenMinimumCompleted,true);
	LoadScreenFinished = false;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,TimerDelegate,MinimumLoadScreenTime,false);

	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		CurrentActivity->LoadActivity();
		ActivityLoaded = true;
		StartActivity();
	});
}

void UAA_ActivityManagerSubsystem::LoadScreenMinimumCompleted(bool LoadingActivity)
{
	LoadScreenFinished = true;
	if(LoadingActivity)
	{
		StartActivity();
	}else
	{
		DestroyActivityFinished();
	}
}

bool UAA_ActivityManagerSubsystem::CanLaunchActivity() const
{
	return !CurrentActivity;
}

void UAA_ActivityManagerSubsystem::DestroyActivity()
{
	//Show Loading Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->ShowLoadingScreen();

	//Show Load Screen for minimum time
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this,&UAA_ActivityManagerSubsystem::LoadScreenMinimumCompleted,false);
	LoadScreenFinished = false;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,TimerDelegate,MinimumLoadScreenTime,false);

	//Start Task to destroy Activity
	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		CurrentActivity->DestroyActivity();
		ActivityLoaded = false;
		DestroyActivityFinished();
	});
}

void UAA_ActivityManagerSubsystem::StartActivity()
{
	//Wait for both
	if(!LoadScreenFinished || !ActivityLoaded){return;}
	
	if(!CurrentActivity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("Can't Start CurrentActivity: CurrentActivity was nullptr"))
		return;
	}
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->HideLoadingScreen();
	
	CurrentActivity->StartActivity();
}

void UAA_ActivityManagerSubsystem::DestroyActivityFinished()
{
	if(!LoadScreenFinished || ActivityLoaded){return;}

	//End Loading Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->HideLoadingScreen();
	
	//Set current activity back to nullptr
	CurrentActivity.SetInterface(nullptr);
	CurrentActivity.SetObject(nullptr);
	


}
