// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_AIRacerController.h"

#include "AI/AA_AIVehicleControlComponent.h"
#include "AI/AA_RacerSplineFollowingComponent.h"
#include "AI/AA_ObstacleDetectionComponent.h"
#include "AI/AA_RacerObstacleAvoidanceComponent.h"
#include "AI/AA_AIGetUnstuckComponent.h"
#include "AI/AA_RacerVerbalBarksComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Landscape.h"
#include "Components/SplineComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"

#include "Actors/AA_TrackInfoActor.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "UI/AA_GameUserSettings.h"

#include <limits>

using namespace AA;

namespace
{
	constexpr float AIRacerControllerOneShotTimerDelay = 0.25f;
}

AAA_AIRacerController::AAA_AIRacerController()
{
	VehicleControlComponent = CreateDefaultSubobject<UAA_AIVehicleControlComponent>(TEXT("Vehicle Control"));
	RacerSplineFollowingComponent = CreateDefaultSubobject<UAA_RacerSplineFollowingComponent>(TEXT("Racer Spline Following"));
	ObstacleDetectionComponent = CreateDefaultSubobject<UAA_ObstacleDetectionComponent>(TEXT("Obstacle Detection"));
	RacerObstacleAvoidanceComponent = CreateDefaultSubobject<UAA_RacerObstacleAvoidanceComponent>(TEXT("Racer Obstacle Avoidance"));
	GetUnstuckComponent = CreateDefaultSubobject<UAA_AIGetUnstuckComponent>(TEXT("Get Unstuck"));
	RacerVerbalBarksComponent = CreateDefaultSubobject<UAA_RacerVerbalBarksComponent>(TEXT("Racer Verbal Barks"));

	const auto NumDifficulties = static_cast<int32>(EAA_AIDifficulty::Hard) + 1;
	DifficultySettings.Reset(NumDifficulties);

	for (int32 i = 0; i < NumDifficulties; ++i)
	{
		DifficultySettings.Add(FAA_RacerAISettings{ .Difficulty = static_cast<EAA_AIDifficulty>(i) });
	}
}

void AAA_AIRacerController::SetTrackInfo(AAA_TrackInfoActor* TrackInfoActor)
{
	RacerContext.SetRaceTrack(TrackInfoActor);
}

void AAA_AIRacerController::StopRacing()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Display, TEXT("%s: StopRacing"), *GetName());

	if (GetUnstuckComponent)
	{
		GetUnstuckComponent->Deactivate();
	}
	if (RacerSplineFollowingComponent)
	{
		RacerSplineFollowingComponent->Deactivate();
	}
	if (RacerVerbalBarksComponent)
	{
		RacerVerbalBarksComponent->Deactivate();
	}
	if (RacerObstacleAvoidanceComponent)
	{
		RacerObstacleAvoidanceComponent->Deactivate();
	}
	if (ObstacleDetectionComponent)
	{
		ObstacleDetectionComponent->Deactivate();
	}
	if (VehicleControlComponent)
	{
		VehicleControlComponent->Deactivate();
	}

	if (auto VehiclePawn = Cast<AAA_WheeledVehiclePawn>(GetPawn()); VehiclePawn)
	{
		// TODO: For some reason the current tick cannot be aborted on VehicleControlComponent even with an active check so schedule for next tick
		// Slow down to a stop
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakVehicle = TWeakObjectPtr<AAA_WheeledVehiclePawn>(VehiclePawn)]()
		{
			if (auto VehiclePawn = WeakVehicle.Get(); VehiclePawn)
			{
				VehiclePawn->SetSteering(FMath::RandBool() ? RaceEndSteeringDeviation : -RaceEndSteeringDeviation);
				VehiclePawn->SetBrake(RaceEndBrakeAmount);
				VehiclePawn->SetThrottle(0);
				VehiclePawn->SetHandbrake(false);

				UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: Race End parameters applied"), *GetName());
			}
		}));
	}
}

FAA_AIRacerSnapshotData AAA_AIRacerController::CaptureSnapshot() const
{
	return FAA_AIRacerSnapshotData
	{
		.RaceState = RacerContext.RaceState,
		.MovementTarget = RacerContext.MovementTarget,
		.DesiredSpeedMph = RacerContext.DesiredSpeedMph,
		.TargetDistanceAlongSpline = RacerContext.TargetDistanceAlongSpline
	};
}

void AAA_AIRacerController::RestoreFromSnapshot(const FAA_AIRacerSnapshotData& InSnapshotData, float InRewindTime)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s: RestoreFromSnapshot: InRewindTime=%f"),
		*GetName(), InRewindTime);

	RacerContext.RaceState = InSnapshotData.RaceState;
	RacerContext.DesiredSpeedMph = InSnapshotData.DesiredSpeedMph;
	RacerContext.MovementTarget = InSnapshotData.MovementTarget;
	RacerContext.TargetDistanceAlongSpline = InSnapshotData.TargetDistanceAlongSpline;
}

#if ENABLE_VISUAL_LOG
void AAA_AIRacerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	auto& Category = Snapshot->Status[0];

	Category.Add(TEXT("Race Track"), LoggingUtils::GetName(RacerContext.RaceTrack));
	Category.Add(TEXT("Difficulty"), CurrentDifficulty ? UEnum::GetDisplayValueAsText(*CurrentDifficulty).ToString(): TEXT("N/A"));

	if (VehicleControlComponent)
	{
		VehicleControlComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (RacerSplineFollowingComponent)
	{
		RacerSplineFollowingComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (ObstacleDetectionComponent)
	{
		ObstacleDetectionComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (RacerObstacleAvoidanceComponent)
	{
		RacerObstacleAvoidanceComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (GetUnstuckComponent)
	{
		GetUnstuckComponent->DescribeSelfToVisLog(Snapshot);
	}
}
#endif

void AAA_AIRacerController::BeginPlay()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();
}

void AAA_AIRacerController::OnPossess(APawn* InPawn)
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnPossess: %s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::OnPossess(InPawn);

	RacerContext = {};

	auto VehiclePawn = Cast<AAA_WheeledVehiclePawn>(InPawn);

	if (!VehiclePawn)
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: OnPossess: %s (%s) is not a AAA_WheeledVehiclePawn!"),
			*GetName(), *LoggingUtils::GetName(InPawn), InPawn ? *LoggingUtils::GetName(InPawn->GetClass()) : TEXT("NULL"));
		return;
	}

	auto GameUserSettings = UAA_GameUserSettings::GetInstance();
	if (GameUserSettings)
	{
		GameUserSettings->OnGameUserSettingsUpdated.AddDynamic(this, &ThisClass::OnRacerSettingsUpdated);
	}
	else
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: OnPossess: %s (%s) GameUserSettings are NULL!"),
			*GetName(), *LoggingUtils::GetName(InPawn), InPawn ? *LoggingUtils::GetName(InPawn->GetClass()) : TEXT("NULL"));
	}

	SetAIParameters(GetCurrentRacerAISettings());

	// The vehicle changes parameters on start so defer setting these 
	FTimerHandle OneShotTimer;
	
	GetWorldTimerManager().SetTimer(OneShotTimer,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			SetVehicleParameters(GetCurrentRacerAISettings());
		}),
		AIRacerControllerOneShotTimerDelay, false);

	if(!RacerContext.RaceTrack)
	{
		SetRaceTrack(*VehiclePawn);
	}

	check(VehicleControlComponent);
	VehicleControlComponent->SetVehiclePawn(VehiclePawn);

	RacerContext.SetVehiclePawn(VehiclePawn);
	RacerContext.DesiredSpeedMph = VehicleControlComponent->GetDesiredSpeedMph();

	// TODO: Plan to use AI State Tree to manage AI behavior.  Since this is new to UE 5.1 and haven't used it, 
	// fallback would be to use a behavior tree or even just code up the logic in the AI Controller itself
	
	SetupComponentEventBindings();

	RegisterRewindable(ERestoreTiming::Resume);

	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnPossess: RaceTrack=%s"),
		*GetName(), *LoggingUtils::GetName(RacerContext.RaceTrack));
}

void AAA_AIRacerController::OnUnPossess()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnUnPossess: PreviousPawn=%s"), *GetName(), *LoggingUtils::GetName(GetPawn()));

	Super::OnUnPossess();

	UnregisterRewindable();

	RacerContext = {};

	if (VehicleControlComponent)
	{
		VehicleControlComponent->SetVehiclePawn(nullptr);
	}
}

void AAA_AIRacerController::SetupComponentEventBindings()
{
	check(VehicleControlComponent);
	check(RacerObstacleAvoidanceComponent);
	check(RacerSplineFollowingComponent);
	check(GetUnstuckComponent);
	check(RacerVerbalBarksComponent);

	VehicleControlComponent->OnVehicleReachedTarget.AddDynamic(RacerSplineFollowingComponent, &UAA_RacerSplineFollowingComponent::SelectNewMovementTarget);
	RacerSplineFollowingComponent->OnVehicleTargetUpdated.AddDynamic(VehicleControlComponent, &UAA_AIVehicleControlComponent::OnVehicleTargetUpdated);
	ObstacleDetectionComponent->OnVehicleObstaclesUpdated.AddDynamic(RacerObstacleAvoidanceComponent, &UAA_RacerObstacleAvoidanceComponent::OnVehicleObstaclesUpdated);
	RacerObstacleAvoidanceComponent->OnVehicleAvoidancePositionUpdated.AddDynamic(RacerSplineFollowingComponent, &UAA_RacerSplineFollowingComponent::OnVehicleAvoidancePositionUpdated);
	GetUnstuckComponent->OnVehicleStuck.AddDynamic(RacerSplineFollowingComponent, &UAA_RacerSplineFollowingComponent::SelectUnstuckTarget);

	GetUnstuckComponent->OnVehicleStuck.AddDynamic(RacerVerbalBarksComponent, &UAA_RacerVerbalBarksComponent::OnStuck);
	RacerVerbalBarksComponent->OnPossessedVehiclePawn(RacerContext.VehiclePawn);

	RacerSplineFollowingComponent->OnRaceCompleted.AddDynamic(this, &ThisClass::OnRaceCompleted);
}

void AAA_AIRacerController::SetRaceTrack(const AAA_WheeledVehiclePawn& VehiclePawn)
{
	// TODO: This will probably be passed in externally
	// here we just find the nearest one that has smallest completion percentage

	const auto GameWorld = GetWorld();

	AAA_TrackInfoActor* NearestRaceTrack{};
	double SmallestCompletion{ std::numeric_limits<double>::max() };
	double SmallestDistance{ std::numeric_limits<double>::max() };

	const auto& VehicleLocation = VehiclePawn.GetActorLocation();

	for (TObjectIterator<AAA_TrackInfoActor> It; It; ++It)
	{
		if (GameWorld != It->GetWorld() || !It->Spline)
		{
			continue;
		}

		AAA_TrackInfoActor* RaceTrack = *It;
		auto Spline = RaceTrack->Spline;

		const auto NearestSplineLocation = Spline->FindLocationClosestToWorldLocation(VehicleLocation, ESplineCoordinateSpace::World);
		const double DistMeters = FVector::Distance(VehicleLocation, NearestSplineLocation) / 100;
		// Don't find one too far away
		if (DistMeters > MaxRaceDistance)
		{
			continue;
		}

		const auto Key = Spline->FindInputKeyClosestToWorldLocation(NearestSplineLocation);
		const auto DistanceAlongSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
		// If we are close to the end of the race just wrap back to beginning
		auto CompletionFraction = DistanceAlongSpline / Spline->GetSplineLength();
		if (CompletionFraction > 0.9f)
		{
			CompletionFraction = 0.0f;
		}
			
		if (CompletionFraction < SmallestCompletion || (FMath::IsNearlyZero(CompletionFraction) && DistMeters < SmallestDistance))
		{
			NearestRaceTrack = RaceTrack;
			SmallestCompletion = CompletionFraction;
			SmallestDistance = DistMeters;
		}
	}

	RacerContext.SetRaceTrack(NearestRaceTrack);
	
	if (NearestRaceTrack)
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: SetRaceTrack: NearestRaceTrack=%s"),
			*GetName(), *NearestRaceTrack->GetName());
	}
	else
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: SetRaceTrack: NearestRaceTrack could not be determined!"),
			*GetName());
	}
}

void AAA_AIRacerController::SetVehicleParameters(const FAA_RacerAISettings& RacerAISettings)
{
	auto VehiclePawn = RacerContext.VehiclePawn;

	if (!VehiclePawn)
	{
		return;
	}

	CurrentDifficulty = RacerAISettings.Difficulty;

	VehiclePawn->SetTractionControlState(RacerAISettings.bEnableTractionControl);
	VehiclePawn->SetABSState(RacerAISettings.bEnableABS);
	VehiclePawn->BoostBrakingForce(RacerAISettings.BrakingForceBoostMultiplier);
	VehiclePawn->SetWheelLoadRatio(RacerAISettings.WheelLoadRatio);

	SetAIParameters(RacerAISettings);
}

void AAA_AIRacerController::SetAIParameters(const FAA_RacerAISettings& RacerAISettings)
{
	RacerSplineFollowingComponent->SetMinSpeedMph(RacerAISettings.MinSpeedMph);
	RacerSplineFollowingComponent->SetMaxSpeedMph(RacerAISettings.MaxSpeedMph);
	RacerSplineFollowingComponent->SetSpeedCurvatureCurve(RacerAISettings.SpeedVsCurvatureCurve);
	RacerSplineFollowingComponent->SetBankAngleCurvatureReductionCurve(RacerAISettings.CurvatureReductionVsBankAngleCurve);
}

FAA_RacerAISettings AAA_AIRacerController::GetCurrentRacerAISettings() const
{
	const auto GameUserSettings = UAA_GameUserSettings::GetInstance();
	if (!GameUserSettings)
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: GetCurrentRacerAISettings: GameUserSettings is NULL - using a default value"),
			*GetName());

		return {};
	}

	const auto Difficulty = GameUserSettings->GetAIDifficulty();
	if (static_cast<int32>(Difficulty) >= DifficultySettings.Num())
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: GetCurrentRacerAISettings: Difficulty=%s is out of bounds of current DifficultySettings with size=%d - using a default value"),
			*GetName(), *UEnum::GetDisplayValueAsText(Difficulty).ToString(), DifficultySettings.Num());

		return {};
	}

	const auto& ChosenSettings = DifficultySettings[static_cast<int32>(Difficulty)];

	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: GetCurrentRacerAISettings: %s"),
		*GetName(), *ChosenSettings.ToString());

	return ChosenSettings;
}

void AAA_AIRacerController::OnRacerSettingsUpdated()
{
	const auto& RacerAISettings = GetCurrentRacerAISettings();
	const bool bChanged = !CurrentDifficulty.has_value() || RacerAISettings.Difficulty != *CurrentDifficulty;

	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnRacerSettingsUpdated; Changed=%s"),
		*GetName(), LoggingUtils::GetBoolString(bChanged));

	if (bChanged)
	{
		SetVehicleParameters(RacerAISettings);
	}
}

void AAA_AIRacerController::OnRaceCompleted(AAA_WheeledVehiclePawn* VehiclePawn)
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnRaceCompleted: %s"), *GetName(), *LoggingUtils::GetName(VehiclePawn));

	StopRacing();
}

FString FAA_RacerAISettings::ToString() const
{
	return FString::Printf(TEXT("Difficulty=%s; MinSpeedMph=%f; MaxSpeedMph=%f; bEnableABS=%s; bEnableTractionControl=%s; BrakingForceBoostMultiplier=%f; WheelLoadRatio=%f"),
		*UEnum::GetDisplayValueAsText(Difficulty).ToString(),
		MinSpeedMph,
		MaxSpeedMph,
		LoggingUtils::GetBoolString(bEnableABS),
		LoggingUtils::GetBoolString(bEnableTractionControl),
		BrakingForceBoostMultiplier,
		WheelLoadRatio
	);
}
