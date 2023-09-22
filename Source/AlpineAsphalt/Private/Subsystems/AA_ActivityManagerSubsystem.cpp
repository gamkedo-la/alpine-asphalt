#include "Subsystems/AA_ActivityManagerSubsystem.h"
#include "Interface/AA_ActivityInterface.h"

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
	
	//TODO: Show Load Screen
	CurrentActivity->LoadActivity();
}

bool UAA_ActivityManagerSubsystem::CanLaunchActivity() const
{
	return !CurrentActivity;
}

void UAA_ActivityManagerSubsystem::EndActivity()
{
	//TODO: Show Loading Screen
	
	//AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [CurrentActivity]()
	//{
		CurrentActivity->DestroyActivity();
	//});
}

void UAA_ActivityManagerSubsystem::StartActivity()
{
	if(!CurrentActivity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("Can't Start CurrentActivity: CurrentActivity was nullptr"))
		return;
	}
	//TODO: End Loading Screen
}

void UAA_ActivityManagerSubsystem::EndActivityFinished()
{
	//TODO: End Loading Screen

}
