// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/AA_WheeledVehiclePawn.h"

#include "Subsystems/AA_RewindSubsystem.h"
#include "DataAsset/AA_VehicleDataAsset.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "DerivedDataCache/Public/DerivedDataCacheUsageStats.h"

FName AAA_WheeledVehiclePawn::VehicleMovementComponentName(TEXT("WheeledVehicleMovementComp"));
FName AAA_WheeledVehiclePawn::VehicleMeshComponentName(TEXT("VehicleMesh"));

//Define Log for Vehicle
DEFINE_LOG_CATEGORY(Vehicle);

AAA_WheeledVehiclePawn::AAA_WheeledVehiclePawn(const class FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(VehicleMeshComponentName);
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->BodyInstance.bSimulatePhysics = false;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(false);
	RootComponent = Mesh;

	VehicleMovementComponent = CreateDefaultSubobject<UChaosWheeledVehicleMovementComponent>(VehicleMovementComponentName);
	VehicleMovementComponent->SetIsReplicated(true); // Enable replication by default
	VehicleMovementComponent->UpdatedComponent = Mesh;
}

void AAA_WheeledVehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		GetWorldTimerManager().SetTimer(RecordingSnapshotTimerHandle, this, &AAA_WheeledVehiclePawn::RecordSnapshot,RewindSystem->RecordingResolution,true);
		MaxSnapshots = FMath::FloorToInt(RewindSystem->MaxRecordingLength / RewindSystem->RecordingResolution);
		RewindResolution = RewindSystem->RecordingResolution;
		RewindSystem->RegisterRewindable(this);
	}else
	{
		UE_LOG(Vehicle,Error,TEXT("Rewind Subsystem Unavailable"))
	}
	
}

UChaosWheeledVehicleMovementComponent* AAA_WheeledVehiclePawn::GetVehicleMovementComponent() const
{
	return VehicleMovementComponent;
}

#if WITH_EDITOR
void AAA_WheeledVehiclePawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//If property changed is VehicleData
	if(PropertyChangedEvent.Property->GetName() == "VehicleData")
	{
		SetVehicleData(VehicleData);
	}
}
#endif

void AAA_WheeledVehiclePawn::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL,
                                              float& YPos)
{
	static FName NAME_Vehicle = FName(TEXT("Vehicle"));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
}

void AAA_WheeledVehiclePawn::SetVehicleData(UAA_VehicleDataAsset* NewVehicleData)
{
	UE_LOG(Vehicle, Log, TEXT("Setting Vehicle Data"))
	if(NewVehicleData == nullptr)
	{
		UE_LOG(Vehicle,Warning,TEXT("New VehicleData was nullptr"))
		return;
	}
	//TODO: Add support for changing wheel count?
	if(VehicleMovementComponent->WheelSetups.Num() != NewVehicleData->WheelSetups.Num())
	{
		UE_LOG(Vehicle,Warning,TEXT("Changing Wheel Count Not Currently Supported"))
		return;
	}
	
	//TODO: Don't set things like Automatic vs Manual Transmission

	//Set Skeletal Mesh
	Mesh->SetSkeletalMesh(NewVehicleData->VehicleMesh);

	//I feel like this is needed, but it only works without it?
	//TODO: figure out why this doesn't work
	//Mesh->SetAnimInstanceClass(dynamic_cast<UClass*>(NewVehicleData->AnimationInstance));

	//Set Vehicle Movement
	VehicleMovementComponent->WheelSetups = NewVehicleData->WheelSetups;
	VehicleMovementComponent->EnableSuspension(NewVehicleData->bSuspensionEnabled);
	VehicleMovementComponent->EnableWheelFriction(NewVehicleData->bWheelFrictionEnabled);
	VehicleMovementComponent->bLegacyWheelFrictionPosition = NewVehicleData->bLegacyWheelFrictionPosition;
	VehicleMovementComponent->WheelTraceCollisionResponses = NewVehicleData->WheelTraceCollisionResponses;
	VehicleMovementComponent-> bMechanicalSimEnabled = NewVehicleData->bMechanicalSimEnabled;
	VehicleMovementComponent->EngineSetup = NewVehicleData->EngineSetup;
	VehicleMovementComponent->DifferentialSetup = NewVehicleData->DifferentialSetup;
	VehicleMovementComponent->TransmissionSetup = NewVehicleData->TransmissionSetup;
	VehicleMovementComponent->SteeringSetup = NewVehicleData->SteeringSetup;

	//Restart the vehicle
	VehicleMovementComponent->ResetVehicleState();

	//store the vehicle data
	VehicleData = NewVehicleData;
}

void AAA_WheeledVehiclePawn::SetRewindTime(float Time)
{
	IAA_RewindableInterface::SetRewindTime(Time);
	RewindTime = Time;

	int index = FMath::FloorToInt(RewindTime/RewindResolution);
	index = SnapshotData.Num() - (index + 1); 
	if(index < SnapshotData.Num())
	{
		UE_LOG(Vehicle,Verbose,TEXT("Setting Snapshot %d of %d"),index, SnapshotData.Num())
		const FWheeledSnaphotData Snapshot = SnapshotData[index]; 
		VehicleMovementComponent->SetSnapshot(Snapshot);
	}else
	{
		UE_LOG(Vehicle,Error,TEXT("Snapshot Index out of bounds while rewinding."))
	}
}

void AAA_WheeledVehiclePawn::PauseRecordingSnapshots()
{
	RecordSnapshot();
	GetWorldTimerManager().PauseTimer(RecordingSnapshotTimerHandle);
}

void AAA_WheeledVehiclePawn::ResumeRecordingSnapshots()
{
	int index = FMath::FloorToInt(RewindTime/RewindResolution);
	index = SnapshotData.Num() - (index + 1);
	if(index < SnapshotData.Num())
	{
		//reverse through list
		for (int i = SnapshotData.Num()-1; i > index; i--)
		{
			SnapshotData.RemoveAt(i);
		}
	}
	GetWorldTimerManager().UnPauseTimer(RecordingSnapshotTimerHandle);
}

void AAA_WheeledVehiclePawn::RecordSnapshot()
{
	if(SnapshotData.Num() > MaxSnapshots)
	{
		SnapshotData.RemoveAt(0);
	}
	SnapshotData.Add(VehicleMovementComponent->GetSnapshot());
}