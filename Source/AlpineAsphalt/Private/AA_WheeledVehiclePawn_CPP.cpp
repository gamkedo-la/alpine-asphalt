// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_WheeledVehiclePawn_CPP.h"

#include "AA_VehicleDataAsset_CPP.h"
#include "ChaosWheeledVehicleMovementComponent.h"

FName AAA_WheeledVehiclePawn_CPP::VehicleMovementComponentName(TEXT("WheeledVehicleMovementComp"));
FName AAA_WheeledVehiclePawn_CPP::VehicleMeshComponentName(TEXT("VehicleMesh"));

AAA_WheeledVehiclePawn_CPP::AAA_WheeledVehiclePawn_CPP(const class FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
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

UChaosWheeledVehicleMovementComponent* AAA_WheeledVehiclePawn_CPP::GetVehicleMovementComponent() const
{
	return VehicleMovementComponent;
}

void AAA_WheeledVehiclePawn_CPP::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL,
	float& YPos)
{
	static FName NAME_Vehicle = FName(TEXT("Vehicle"));

	Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
}

void AAA_WheeledVehiclePawn_CPP::SetVehicleData(UAA_VehicleDataAsset_CPP* NewVehicleData)
{
	//TODO: Don't set things like Automatic vs Manual Transmission

	//Set Skeletal Mesh
	Mesh->SetSkeletalMesh(NewVehicleData->VehicleMesh);
	Mesh->SetAnimInstanceClass(dynamic_cast<UClass*>(NewVehicleData->AnimationInstance));

	//Set Vehicle Movement
	VehicleMovementComponent->WheelSetups = NewVehicleData->WheelSetups;
	VehicleMovementComponent->EnableSuspension(NewVehicleData->bSuspensionEnabled);
	VehicleMovementComponent->EnableWheelFriction(NewVehicleData->bWheelFrictionEnabled);
	VehicleMovementComponent->bLegacyWheelFrictionPosition = NewVehicleData->bLegacyWheelFrictionPosition;
	VehicleMovementComponent->WheelTraceCollisionResponses = NewVehicleData->WheelTraceCollisionResponses;
}
