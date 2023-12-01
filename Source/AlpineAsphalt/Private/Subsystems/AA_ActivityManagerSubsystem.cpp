#include "Subsystems/AA_ActivityManagerSubsystem.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "Activity/AA_BaseActivity.h"
#include "Components/AA_ChaosWheeledVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_RewindSubsystem.h"
#include "UI/AA_BaseUI.h"
#include "UI/AA_VehicleUI.h"

DEFINE_LOG_CATEGORY(ActivityManagerSubsystem);

UAA_ActivityManagerSubsystem::UAA_ActivityManagerSubsystem()
{
	OnLoadActivityCompleted.AddDynamic(this,&UAA_ActivityManagerSubsystem::StartActivity);
	OnDestroyActivityCompleted.AddDynamic(this,&UAA_ActivityManagerSubsystem::DestroyActivityFinished);
}

void UAA_ActivityManagerSubsystem::LaunchActivity(UAA_BaseActivity* Activity, bool RestartingActivity)
{
	if(!Activity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("%hs: Can't Launch Activity: Activity was nullptr"),__func__)
		return;
	}
	if(!RestartingActivity && CurrentActivity)
	{
		UE_LOG(ActivityManagerSubsystem,Error,TEXT("%hs: Can't Launch Activity: Activity already taking place"),__func__)
		return;
	}
	//Store activity
	CurrentActivity = Activity;
	
	//Show Load Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->BaseUI->ShowLoadingScreen();

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
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->BaseUI->ShowLoadingScreen();

	//Show Load Screen for minimum time
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this,&UAA_ActivityManagerSubsystem::LoadScreenMinimumCompleted,false);
	LoadScreenFinished = false;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,TimerDelegate,MinimumLoadScreenTime,false);

	
	//Start Task to destroy Activity
	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
		PlayerVehicle->ResetVehicle();
		PlayerVehicle->GetVehicleMovementComponent()->SetParked(true);
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
	GetWorld()->GetSubsystem<UAA_RewindSubsystem>()->ResetRewindHistory();
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->BaseUI->HideLoadingScreen();
	
	CurrentActivity->StartActivity();
}

void UAA_ActivityManagerSubsystem::RestartActivity()
{

	//Destroy/////////////////////////////////
	//Show Loading Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->BaseUI->ShowLoadingScreen();

	//Show Load Screen for minimum time
	//FTimerHandle TimerHandle;
	FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this,&UAA_ActivityManagerSubsystem::LoadScreenMinimumCompleted,false);
	LoadScreenFinished = false;

	//Don't need this since we're going to Start the activity too?
	//GetWorld()->GetTimerManager().SetTimer(TimerHandle,TimerDelegate,MinimumLoadScreenTime,false);
	
	//Task to Destroy and Restart
	AsyncTask(ENamedThreads::GameThread, [&]()
	{
		CurrentActivity->DestroyActivity();
		GetWorld()->GetSubsystem<UAA_RewindSubsystem>()->ResetRewindHistory();
		ActivityLoaded = false;
		LaunchActivity(CurrentActivity, true);
	});
	//Destroy Done////////////////////////

	
}

void UAA_ActivityManagerSubsystem::DestroyActivityFinished()
{
	if(!LoadScreenFinished || ActivityLoaded){return;}

	GetWorld()->GetSubsystem<UAA_RewindSubsystem>()->ResetRewindHistory();
	//End Loading Screen
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->BaseUI->HideLoadingScreen();

	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(false);
	
	//Set current activity back to nullptr
	CurrentActivity = nullptr;
}
