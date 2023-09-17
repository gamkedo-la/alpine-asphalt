// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AA_RaceActor.h"

#include "GeometryTypes.h"
#include "Actors/AA_Checkpoint.h"
#include "Components/AA_CheckpointComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "GameFramework/GameSession.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"

// Sets default values
AAA_RaceActor::AAA_RaceActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Spline = CreateDefaultSubobject<USplineComponent>("RacingLineSpline");
	CheckpointComponent = CreateDefaultSubobject<UAA_CheckpointComponent>("CheckpointComponent");
	InteractableCollision = CreateDefaultSubobject<USphereComponent>("InteractableCollision");
	SetRootComponent(Spline);

	InteractableCollision->OnComponentBeginOverlap.AddDynamic(this, &AAA_RaceActor::OnOverlapBegin);
	InteractableCollision->OnComponentEndOverlap.AddDynamic(this,&AAA_RaceActor::OnOverlapEnd);
	InteractableCollision->SetupAttachment(Spline);
	InteractableCollision->InitSphereRadius(500.f);
}

// Called when the game starts or when spawned
void AAA_RaceActor::BeginPlay()
{
	Super::BeginPlay();
}

void AAA_RaceActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AAA_RaceActor::CheckpointHit(int IndexCheckpointHit)
{
	if(IndexCheckpointHit == LastCheckpointHitIndex + 1)
	{
		LastCheckpointHitIndex = IndexCheckpointHit;
		UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Success"))
		if(LastCheckpointHitIndex == CheckpointComponent->SpawnedCheckpoints.Num()-1)
		{
			FinishTime = UGameplayStatics::GetTimeSeconds(this);
			UGameplayStatics::GetGameInstance(this)->StopRecordingReplay();
			FTimerHandle TimerHandle;
			GetWorldTimerManager().SetTimer(TimerHandle,this,&AAA_RaceActor::GoToEndRaceState,FinishDelay,false);
			UE_LOG(LogTemp,Log,TEXT("Last Checkpoint Hit: Race Over"))
			UE_LOG(LogTemp,Log,TEXT("Finish Time: %f"),(FinishTime-StartTime));
		}
	}else
	{
		UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Out of order"))
	}
}

void AAA_RaceActor::GoToEndRaceState()
{
	//Play Replay
	//UGameplayStatics::GetGameInstance(this)->PlayReplay(FString("Replay"));

	//Lock actual car
	Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0))->SetLockLocation(true);
	
	//Show UI Screen
	//UGameplayStatics::GetPlayerController(GetWorld(),0)->GetHUD();
	
	//Wait for Confirmation
}

void AAA_RaceActor::ReturnToOpenWorldState()
{
	//Get Player
	const auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	if(PlayerVehicle)
	{
		PlayerVehicle->SetLockLocation(false);

		//Put Player back to race start 
		auto Rotation = Spline->GetRotationAtTime(0.f,ESplineCoordinateSpace::World);
		auto Location = Spline->GetLocationAtTime(0.f,ESplineCoordinateSpace::World);
		PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
		PlayerVehicle->ResetVehicle();
	}
	UnloadRace();
}

void AAA_RaceActor::OnFinishDelayFinish()
{
	GoToEndRaceState();
}

float AAA_RaceActor::GetWidthAtDistance(float Distance)
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

void AAA_RaceActor::LoadRace()
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

void AAA_RaceActor::UnloadRace()
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

void AAA_RaceActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(const auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(const auto PC = Cast<AAA_PlayerController>(HitVehicle->Controller))
		{
			PC->AddInteractables(this);
			UE_LOG(LogTemp,Warning,TEXT("Hit Race"));
		}
	}
}

void AAA_RaceActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(const auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(const auto PC = Cast<AAA_PlayerController>(HitVehicle->Controller))
		{
			PC->RemoveInteractable(this);
			UE_LOG(LogTemp,Warning,TEXT("Left Race"));
		}
	}
}

void AAA_RaceActor::Interact(AAA_PlayerController* Interactor)
{
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(Interactor->GetPawn());
	//Set Actor at start
	auto Rotation = Spline->GetRotationAtTime(0.f,ESplineCoordinateSpace::World);
	auto Location = Spline->GetLocationAtTime(0.f,ESplineCoordinateSpace::World);
	
	PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
	PlayerVehicle->ResetVehicle();
	
	//Load the Race Decorations
	LoadRace();
	//Spawn CheckPoints
	for (auto Checkpoint : CheckpointComponent->SpawnedCheckpoints)
	{
		Checkpoint->CheckpointHit.AddDynamic(this, &AAA_RaceActor::CheckpointHit);
	}
	LastCheckpointHitIndex = -1;
	
	UGameplayStatics::GetGameInstance(this)->StartRecordingReplay(FString("Replay"),FString("Replay"));

	StartTime = UGameplayStatics::GetTimeSeconds(this);
}


