// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AA_TrackInfoActor.h"

#include "Activity/AA_TimeTrialActivity.h"
#include "Components/AA_CheckpointComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_ActivityManagerSubsystem.h"
#include "UI/AA_VehicleUI.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"

// Sets default values
AAA_TrackInfoActor::AAA_TrackInfoActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Spline = CreateDefaultSubobject<USplineComponent>("RacingLineSpline");
	CheckpointComponent = CreateDefaultSubobject<UAA_CheckpointComponent>("CheckpointComponent");
	InteractableCollision = CreateDefaultSubobject<USphereComponent>("InteractableCollision");
	SetRootComponent(Spline);

	InteractableCollision->OnComponentBeginOverlap.AddDynamic(this, &AAA_TrackInfoActor::OnOverlapBegin);
	InteractableCollision->OnComponentEndOverlap.AddDynamic(this,&AAA_TrackInfoActor::OnOverlapEnd);
	InteractableCollision->SetupAttachment(Spline);
	InteractableCollision->InitSphereRadius(500.f);
}

float AAA_TrackInfoActor::GetWidthAtDistance(float Distance)
{
	float Key = Spline->GetInputKeyAtDistanceAlongSpline(Distance);
	int Lower = FMath::FloorToInt(Key);
	int Upper = FMath::CeilToInt(Key);
	if(Lower < 0)
	{
		if(RoadWidth.IsEmpty()){ return 0; }
		return RoadWidth[0];			
	}
	if(Upper > RoadWidth.Num())
	{
		return RoadWidth.Last();
	}
	const float Alpha = FMath::Fractional(Key);
	return FMath::Lerp(RoadWidth[Lower],RoadWidth[Upper],Alpha);
}

void AAA_TrackInfoActor::LoadRace()
{
	if(HasAuthority())
	{
		if(UDataLayerSubsystem* DataLayerSubsystem = UWorld::GetSubsystem<UDataLayerSubsystem>(GetWorld()))
		{
			for (const auto DataLayer : DataLayersToLoad)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Activated);
			}
			for (const auto DataLayer : DataLayersToUnload)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Unloaded);
			}
		}
		//Spawn Checkpoints
		CheckpointComponent->SpawnCheckpoints();
	}
}

void AAA_TrackInfoActor::UnloadRace()
{
	if(HasAuthority())
	{
		if(UDataLayerSubsystem* DataLayerSubsystem = UWorld::GetSubsystem<UDataLayerSubsystem>(GetWorld()))
		{
			for (const auto DataLayer : DataLayersToUnload)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Activated);
			}
			for (const auto DataLayer : DataLayersToLoad)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Unloaded);
			}
		}
		//Destroy Checkpoints
		CheckpointComponent->DestroyCheckpoints();
	}
}

void AAA_TrackInfoActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(const auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->CanLaunchActivity())
		{
			if(const auto PC = Cast<AAA_PlayerController>(HitVehicle->Controller))
			{
				PC->AddInteractables(this);
				PC->VehicleUI->ShowRaceInteraction(TrackName,true);
				UE_LOG(LogTemp,Warning,TEXT("Hit Race"));
			}
		}
	}
}

void AAA_TrackInfoActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(const auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(const auto PC = Cast<AAA_PlayerController>(HitVehicle->Controller))
		{
			PC->RemoveInteractable(this);
			PC->VehicleUI->ShowRaceInteraction(TrackName,false);
			UE_LOG(LogTemp,Warning,TEXT("Left Race"));
		}
	}
}

void AAA_TrackInfoActor::Interact(AAA_PlayerController* Interactor)
{
	if(!ActivityType)
	{
		UE_LOG(LogTemp,Error,TEXT("No Activity Set on Object %s: Cannot start Activity"), *this->GetName());
		return;
	}
	const auto PlayerController = UGameplayStatics::GetPlayerController(GetWorld(),0);
	if(const auto PC = Cast<AAA_PlayerController>(PlayerController))
	{
		PC->RemoveInteractable(this);
		PC->VehicleUI->ShowRaceInteraction(TrackName,false);
	}
	//Launch Activity
	auto NewActivity = NewObject<UAA_BaseActivity>(this,ActivityType);
	NewActivity->Initialize(this);
	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->LaunchActivity(NewActivity);
}


