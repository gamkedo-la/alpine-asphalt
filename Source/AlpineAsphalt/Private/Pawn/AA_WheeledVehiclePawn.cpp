// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/AA_WheeledVehiclePawn.h"

#include "Subsystems/AA_RewindSubsystem.h"
#include "DataAsset/AA_VehicleDataAsset.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"

#include <numbers>

#include "Components/AA_ChaosWheeledVehicleMovementComponent.h"
#include "Components/AudioComponent.h"
#include "UI/AA_GameUserSettings.h"

FName AAA_WheeledVehiclePawn::VehicleMovementComponentName(TEXT("WheeledVehicleMovementComp"));
FName AAA_WheeledVehiclePawn::VehicleMeshComponentName(TEXT("VehicleMesh"));

//Define Log for Vehicle
DEFINE_LOG_CATEGORY(LogAAVehicle);

AAA_WheeledVehiclePawn::AAA_WheeledVehiclePawn(const class FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(VehicleMeshComponentName);
	Mesh->SetCollisionProfileName(UCollisionProfile::Vehicle_ProfileName);
	Mesh->BodyInstance.bSimulatePhysics = false;
	Mesh->BodyInstance.bNotifyRigidBodyCollision = true;
	Mesh->BodyInstance.bUseCCD = true;
	Mesh->SetGenerateOverlapEvents(true);
	Mesh->SetCanEverAffectNavigation(false);

	//set properties based on mesh
	PaintMaterial = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0,Mesh->GetMaterial(0));
	RootComponent = Mesh;

	VehicleMovementComponent = CreateDefaultSubobject<UAA_ChaosWheeledVehicleMovementComponent>(VehicleMovementComponentName);
	VehicleMovementComponent->SetIsReplicated(true); // Enable replication by default
	VehicleMovementComponent->UpdatedComponent = Mesh;

	AudioComponent = CreateDefaultSubobject<UAudioComponent>("Audio Component");
	AudioComponent->SetupAttachment(RootComponent);
}

void AAA_WheeledVehiclePawn::BeginPlay()
{
	Super::BeginPlay();

	SetColorOne(ColorOne);
	SetColorTwo(ColorTwo);
	SetColorThree(ColorThree);
	SetColorFour(ColorFour);
	
	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		GetWorldTimerManager().SetTimer(RecordingSnapshotTimerHandle, this, &AAA_WheeledVehiclePawn::RecordSnapshot,RewindSystem->RecordingResolution,true);
		MaxSnapshots = FMath::FloorToInt(RewindSystem->MaxRecordingLength / RewindSystem->RecordingResolution);
		RewindResolution = RewindSystem->RecordingResolution;
		RewindSystem->RegisterRewindable(this);
	}else
	{
		UE_LOG(LogAAVehicle,Error,TEXT("Rewind Subsystem Unavailable"))
	}
	
}

void AAA_WheeledVehiclePawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorldTimerManager().ClearTimer(RecordingSnapshotTimerHandle);
	if (UAA_RewindSubsystem* RewindSystem = GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->UnregisterRewindable(this);
		SnapshotData.Empty();
	}
}

UAA_ChaosWheeledVehicleMovementComponent* AAA_WheeledVehiclePawn::GetVehicleMovementComponent() const
{
	return VehicleMovementComponent;
}

void AAA_WheeledVehiclePawn::SetVehiclePaint(int PaintIndex)
{
	ensure(VehicleData);
	ensure(PaintMaterial);
	if(VehicleData->PaintStyles.Num() > PaintIndex)
	{
		PaintMaterial->SetTextureParameterValue(FName("PaintTexture"),VehicleData->PaintStyles[PaintIndex].PaintTexture);
		PaintTextureIndex = PaintIndex;
	}
}
int AAA_WheeledVehiclePawn::GetPaintIndex()
{
	return PaintTextureIndex;
}
int AAA_WheeledVehiclePawn::GetDecalIndex()
{
	return DecalTextureIndex;
}
void AAA_WheeledVehiclePawn::SetVehicleDecal(int DecalIndex)
{
	ensure(VehicleData);
	ensure(PaintMaterial);
	if(VehicleData->Decals.Num() > DecalIndex)
	{
		PaintMaterial->SetTextureParameterValue(FName("DecalTexture"),VehicleData->Decals[DecalIndex].DecalTexture);
		DecalTextureIndex = DecalIndex;
	}
}

void AAA_WheeledVehiclePawn::SetColorOne(FColor ColorToSet)
{
	PaintMaterial->SetVectorParameterValue(FName("ColorOne"),ColorToSet);
	ColorOne = ColorToSet;
}
FColor AAA_WheeledVehiclePawn::GetColorOne() const
{
	return ColorOne;
}
void AAA_WheeledVehiclePawn::SetColorTwo(FColor ColorToSet)
{
	PaintMaterial->SetVectorParameterValue(FName("ColorTwo"),ColorToSet);
	ColorTwo = ColorToSet;
}

FColor AAA_WheeledVehiclePawn::GetColorTwo() const
{
	return ColorTwo;
}

void AAA_WheeledVehiclePawn::SetColorThree(FColor ColorToSet)
{
	PaintMaterial->SetVectorParameterValue(FName("ColorThree"),ColorToSet);
	ColorThree = ColorToSet;
}

FColor AAA_WheeledVehiclePawn::GetColorThree() const
{
	return ColorThree;
}

void AAA_WheeledVehiclePawn::SetColorFour(FColor ColorToSet)
{
	PaintMaterial->SetVectorParameterValue(FName("ColorFour"),ColorToSet);
	ColorFour = ColorToSet;
}

FColor AAA_WheeledVehiclePawn::GetColorFour() const
{
	return ColorFour;
}

FColor AAA_WheeledVehiclePawn::GetColor(int Index) const
{
	switch(Index)
	{
	case 1:
		return GetColorOne();
	case 2:
		return GetColorTwo();
	case 3:
		return GetColorThree();
	case 4:
		return GetColorFour();
	default:
		UE_LOG(LogAAVehicle,Warning,TEXT("Tried to get Color out of range using index: %d, Returning magenta"),Index);
		return FColor::Magenta;
	}
}

void AAA_WheeledVehiclePawn::SetABSState(bool Enabled)
{
	ensure(VehicleMovementComponent);
	ABSEnabled = Enabled;
	auto Wheels = 	VehicleMovementComponent->Wheels;
	for(int i = 0; i < Wheels.Num(); i++)
	{
		Wheels[i]->bABSEnabled = Enabled;
	}
}

void AAA_WheeledVehiclePawn::SetTractionControlState(bool Enabled)
{
	ensure(VehicleMovementComponent);
	TractionControlEnabled = Enabled;
	auto Wheels = 	VehicleMovementComponent->Wheels;
	for(int i = 0; i < Wheels.Num(); i++)
	{
		Wheels[i]->bTractionControlEnabled = Enabled;
	}
}

void AAA_WheeledVehiclePawn::BoostBrakingForce(float BrakeForceMultiplier)
{
	ensure(VehicleMovementComponent);
	auto Wheels = VehicleMovementComponent->Wheels;

	for (auto Wheel : Wheels)
	{
		if (Wheel->bAffectedByBrake)
		{
			Wheel->MaxBrakeTorque *= BrakeForceMultiplier;
		}

		if (Wheel->bAffectedByHandbrake)
		{
			Wheel->MaxHandBrakeTorque *= BrakeForceMultiplier;
		}
	}
}

void AAA_WheeledVehiclePawn::SetAutomaticShifting(bool Enabled)
{
	AutomaticShiftingEnabled = Enabled;
	VehicleMovementComponent->SetUseAutomaticGears(Enabled);
}

void AAA_WheeledVehiclePawn::SetWheelLoadRatio(float WheelLoadRatio)
{
	ensure(VehicleMovementComponent);
	auto Wheels = VehicleMovementComponent->Wheels;

	for (auto Wheel : Wheels)
	{
		Wheel->WheelLoadRatio = WheelLoadRatio;
	}
}

float AAA_WheeledVehiclePawn::GetVehicleSpeed() const
{
	if (!VehicleMovementComponent)
	{
		return 0.0f;
	}

	return VehicleMovementComponent->GetForwardSpeed();
}

float AAA_WheeledVehiclePawn::GetVehicleSpeedMph() const
{
	if (!VehicleMovementComponent)
	{
		return 0.0f;
	}

	return VehicleMovementComponent->GetForwardSpeedMPH();
}

#if WITH_EDITOR

void AAA_WheeledVehiclePawn::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	
	if(GetWorld() && GetWorld()->WorldType != EWorldType::PIE)
	{
		return;
	}
	//If property changed is VehicleData
	if(PropertyChangedEvent.Property->GetName() == "VehicleData")
	{
		SetVehicleData(VehicleData);
	}
	if(PropertyChangedEvent.Property->GetName() == "PaintTextureIndex")
	{
		SetVehiclePaint(PaintTextureIndex);
	}
	if(PropertyChangedEvent.Property->GetName() == "DecalTextureIndex")
	{
		SetVehicleDecal(DecalTextureIndex);
	}
	if(PropertyChangedEvent.Property->GetName() == "ColorOne")
	{
		SetColorOne(ColorOne);
	}
	if(PropertyChangedEvent.Property->GetName() == "ColorTwo")
	{
		SetColorTwo(ColorTwo);
	}
	if(PropertyChangedEvent.Property->GetName() == "ColorThree")
	{
		SetColorThree(ColorThree);
	}
	if(PropertyChangedEvent.Property->GetName() == "ColorFour")
	{
		SetColorFour(ColorFour);
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
	UE_LOG(LogAAVehicle, Log, TEXT("Setting Vehicle Data"))
	if(NewVehicleData == nullptr)
	{
		UE_LOG(LogAAVehicle, Warning,TEXT("New VehicleData was nullptr"))
		return;
	}
	//TODO: Add support for changing wheel count?
	if(VehicleMovementComponent->WheelSetups.Num() != NewVehicleData->WheelSetups.Num())
	{
		UE_LOG(LogAAVehicle, Warning,TEXT("Changing Wheel Count Not Currently Supported"))
		return;
	}
	
	
	//store the vehicle data
	VehicleData = NewVehicleData;
	
	//Set Skeletal Mesh
	Mesh->SetSkeletalMesh(NewVehicleData->VehicleMesh);
	
	AudioComponent->SetSound(NewVehicleData->Sound);

	PaintMaterial = Mesh->CreateAndSetMaterialInstanceDynamicFromMaterial(0,Mesh->GetMaterial(0));
	if(PaintMaterial)
	{
		SetVehiclePaint(PaintTextureIndex);
		SetVehicleDecal(DecalTextureIndex);
		SetColorOne(ColorOne);
		SetColorTwo(ColorTwo);
		SetColorThree(ColorThree);
		SetColorFour(ColorFour);
	}
	
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

	SetABSState(ABSEnabled);
	SetTractionControlState(TractionControlEnabled);
	SetAutomaticShifting(AutomaticShiftingEnabled);

}

void AAA_WheeledVehiclePawn::SetRewindTime(float Time)
{
	IAA_RewindableInterface::SetRewindTime(Time);
	RewindTime = Time;

	int index = FMath::FloorToInt(RewindTime/RewindResolution);
	index = SnapshotData.Num() - (index + 1);
	index = index >= 0 ? index : 0;
	if(index < SnapshotData.Num())
	{
		UE_LOG(LogAAVehicle,Verbose,TEXT("Setting Snapshot %d of %d"),index, SnapshotData.Num())
		const FWheeledSnaphotData& Snapshot = SnapshotData[index]; 
		VehicleMovementComponent->SetSnapshot(Snapshot);
	}else
	{
		UE_LOG(LogAAVehicle,Error,TEXT("Snapshot Index out of bounds while rewinding."))
	}
}

void AAA_WheeledVehiclePawn::PauseRecordingSnapshots()
{
	RecordSnapshot();
	GetWorldTimerManager().PauseTimer(RecordingSnapshotTimerHandle);
	SnapshotsPaused = true;
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
	SnapshotsPaused = false;
	GetWorldTimerManager().UnPauseTimer(RecordingSnapshotTimerHandle);
}

void AAA_WheeledVehiclePawn::ResetRewindHistory()
{
	SnapshotData.Empty();
}

void AAA_WheeledVehiclePawn::RecordSnapshot()
{
	if(SnapshotsPaused){return;}
	if(SnapshotData.Num() > MaxSnapshots)
	{
		SnapshotData.RemoveAt(0);
	}
	SnapshotData.Add(VehicleMovementComponent->GetSnapshot());
}

float AAA_WheeledVehiclePawn::GetTraction() const
{
	const auto& Velocity = GetVelocity();
	const auto& ForwardVector = GetActorForwardVector();

	const float DotProduct = Velocity.GetSafeNormal() | ForwardVector;
	const float TractionFraction = DotProduct > 0 ? 2 * FMath::Asin(DotProduct) / std::numbers::pi : 0;

	return TractionFraction;
}

bool AAA_WheeledVehiclePawn::IsAccelerating() const
{
	ensure(VehicleMovementComponent);

	return VehicleMovementComponent->GetThrottleInput() > 0;
}

bool AAA_WheeledVehiclePawn::IsBraking() const
{
	ensure(VehicleMovementComponent);

	return VehicleMovementComponent->GetBrakeInput() > 0 || VehicleMovementComponent->GetHandbrakeInput();
}

bool AAA_WheeledVehiclePawn::IsReversing() const
{
	ensure(VehicleMovementComponent);

	return VehicleMovementComponent->GetCurrentGear() < 0;
}

FBox AAA_WheeledVehiclePawn::GetAABB() const
{
	FVector ActorOrigin, BoxExtent;

	GetActorBounds(true, ActorOrigin, BoxExtent, false);

	// ActorOrigin aligns with the AABB Origin correctly
	return FBox::BuildAABB(ActorOrigin, BoxExtent);
}

//////////////// Visual Logger ////////////////////////
#if ENABLE_VISUAL_LOG


void AAA_WheeledVehiclePawn::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	using namespace AA;

	// helpful article about the Visual Logger
	// https://benui.ca/unreal/visual-logger/

	if (!VehicleMovementComponent || !Mesh)
	{
		return;
	}

	// Get reference to the current category
	const int32 CatIndex = Snapshot->Status.AddZeroed();
	FVisualLogStatusCategory& Category = Snapshot->Status[CatIndex];
	Category.Category = FString::Printf(TEXT("Vehicle (%s)"), *GetName());

	const auto ThrottleValue = VehicleMovementComponent->GetThrottleInput();
	const auto BrakeValue = VehicleMovementComponent->GetBrakeInput();
	const auto SteeringValue = VehicleMovementComponent->GetSteeringInput();

	Category.Add(TEXT("Speed MPH"), FString::Printf(TEXT("%.1f"), GetVehicleSpeedMph()));
	Category.Add(TEXT("Traction %"), FString::Printf(TEXT("%.1f"), GetTraction() * 100));
	Category.Add(TEXT("Gear"), FString::Printf(TEXT("%d"), VehicleMovementComponent->GetCurrentGear()));
	Category.Add(TEXT("Steering"), FString::Printf(TEXT("%.2f"), SteeringValue));
	Category.Add(TEXT("Throttle"), FString::Printf(TEXT("%.2f"), ThrottleValue));
	Category.Add(TEXT("Brake"), FString::Printf(TEXT("%.2f"), BrakeValue));

	const bool bHandbrake = VehicleMovementComponent->GetHandbrakeInput();
	Category.Add(TEXT("Handbrake"), LoggingUtils::GetBoolString(bHandbrake));

	const auto& Wheels = VehicleMovementComponent->Wheels;
	if (!Wheels.IsEmpty())
	{
		const auto AnyWheel = Wheels[0];
		Category.Add(TEXT("ABS"), LoggingUtils::GetBoolString(AnyWheel->bABSEnabled));
		Category.Add(TEXT("Traction Control"), LoggingUtils::GetBoolString(AnyWheel->bTractionControlEnabled));
		Category.Add(TEXT("Wheel Load Ratio"), *FString::Printf(TEXT("%f"), AnyWheel->WheelLoadRatio));
	}

	const auto& FrontWorldLocation = GetFrontWorldLocation();
	const auto& ForwardVector = GetActorForwardVector();

	// Change color based on speed and stopping
	FColor BoxColor;
	if (bHandbrake)
	{
		BoxColor = FColor::Red;
	}
	else if (BrakeValue > 0)
	{
		BoxColor = FColor::Orange;
	}
	else
	{
		// Blue -> Green
		BoxColor = UKismetMathLibrary::LinearColorLerp(FColor::Blue, FColor::Green, FMath::Abs(ThrottleValue)).ToFColor(true);
	}

	// Add oriented bounded box for vehicle
	const FVector& BoundsExtent = Mesh->GetLocalBounds().BoxExtent;
	const FVector ZOffset(0, 0, BoundsExtent.Z);

	const auto OBB = FBox::BuildAABB(ZOffset, BoundsExtent);
	const auto TransformMatrix = GetActorTransform().ToMatrixNoScale();

	Snapshot->AddElement(OBB, TransformMatrix, LogAlpineAsphalt.GetCategoryName(), ELogVerbosity::Log, BoxColor);

	const auto MyController = GetController();
	Snapshot->AddElement(GetActorLocation() + ZOffset, LogAlpineAsphalt.GetCategoryName(), ELogVerbosity::Log, BoxColor,
		FString::Printf(TEXT("%s\n%s"), *GetName(), *LoggingUtils::GetName(MyController)));

	// Forward vector
	Snapshot->AddArrow(FrontWorldLocation, FrontWorldLocation + ForwardVector * 100.0f, LogAlpineAsphalt.GetCategoryName(), ELogVerbosity::Log, FColor::Red, TEXT("F"));
}

#endif //ENABLE_VISUAL_LOG

void AAA_WheeledVehiclePawn::ResetVehicleAtLocation(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	SetActorTransform(FTransform(WorldRotation, WorldLocation), false, nullptr, ETeleportType::ResetPhysics);
	ResetVehicle();
}
