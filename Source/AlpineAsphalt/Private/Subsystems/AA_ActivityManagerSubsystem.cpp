#include "Subsystems/AA_ActivityManagerSubsystem.h"

#include "Controllers/AA_PlayerController.h"
#include "Interface/AA_ActivityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AA_VehicleUI.h"

DEFINE_LOG_CATEGORY(ActivityManagerSubsystem);

UAA_ActivityManagerSubsystem::UAA_ActivityManagerSubsystem()
{
	OnLoadActivityCompleted.AddDynamic(this,&UAA_ActivityManagerSubsystem::StartActivity);
	OnDestroyActivityCompleted.AddDynamic(this,&UAA_ActivityManagerSubsystem::EndActivityFinished);
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
	CurrentActivity->LoadActivity();
	StartActivity();
}

bool UAA_ActivityManagerSubsystem::CanLaunchActivity() const
{
	return !CurrentActivity;
}

void UAA_ActivityManagerSubsystem::EndActivity()
{
	//Show Loading Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->ShowLoadingScreen();
	//AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [CurrentActivity]()
	//{
		CurrentActivity->DestroyActivity();
	//});
	CurrentActivity.SetInterface(nullptr);
	CurrentActivity.SetObject(nullptr);
}

void UAA_ActivityManagerSubsystem::StartActivity()
{
	if(!CurrentActivity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("Can't Start CurrentActivity: CurrentActivity was nullptr"))
		return;
	}
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->HideLoadingScreen();
	CurrentActivity->StartActivity();
}

void UAA_ActivityManagerSubsystem::EndActivityFinished()
{
	//TEnd Loading Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->HideLoadingScreen();

}
