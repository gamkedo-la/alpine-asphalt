// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerVerbalBarksComponent.h"

#include "Pawn/AA_WheeledVehiclePawn.h"
#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"

#include "Controllers/AA_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"

using namespace AA;


const FName UAA_RacerVerbalBarksComponent::PitchSoundParameterName = TEXT("InPitchOffset");

#define AA_VB_REGISTER_AUDIO_CLIP(Name,CooldownTimeSeconds) RegisterAudioClip(Name, CooldownTimeSeconds, TEXT(#Name))

UAA_RacerVerbalBarksComponent::UAA_RacerVerbalBarksComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.5f;
}

void UAA_RacerVerbalBarksComponent::OnStuck(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: OnStuck"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	PlayClipIfApplicable(StuckAudioClip);
}

void UAA_RacerVerbalBarksComponent::OnPossessedVehiclePawn(AAA_WheeledVehiclePawn* VehiclePawn)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: OnPossessedVehiclePawn: %s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(VehiclePawn));

	check(VehiclePawn);

	auto Mesh = VehiclePawn->GetMesh();
	check(Mesh);

	// Ensure that we can register hit events
	Mesh->OnComponentHit.AddDynamic(this, &ThisClass::OnVehicleHit);
}

void UAA_RacerVerbalBarksComponent::BeginPlay()
{
	Super::BeginPlay();
	
	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		SetComponentTickEnabled(false);
		return;
	}

	PitchOffsetValue = FMath::RandRange(PitchMinOffset, PitchMaxOffset);

	AA_VB_REGISTER_AUDIO_CLIP(SidewaysAudioClip, SidewaysAudioCooldownTimeSeconds);
	AA_VB_REGISTER_AUDIO_CLIP(PassPlayerAudioClip, PassingAudioCooldownTimeSeconds);
	AA_VB_REGISTER_AUDIO_CLIP(PlayerPassesAudioClip, PassingAudioCooldownTimeSeconds);
	AA_VB_REGISTER_AUDIO_CLIP(HitByPlayerAudioClip, HitByPlayerAudioCooldownTimeSeconds);
	AA_VB_REGISTER_AUDIO_CLIP(StuckAudioClip, StuckAudioCooldownTimeSeconds);
	AA_VB_REGISTER_AUDIO_CLIP(HitPropAudioClip, HitPropAudioCooldownTimeSeconds);

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: BeginPlay - PitchOffsetValue=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), PitchOffsetValue);
}

void UAA_RacerVerbalBarksComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PlayerController)
	{
		PlayerController = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	}

	if (!PlayerController)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: CheckRelativePlayerPosition - PlayerController %s is not a AAA_PlayerController"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(UGameplayStatics::GetPlayerController(GetWorld(), 0)));

		return;
	}

	if (CheckRelativePlayerPosition())
	{
		return;
	}

	if (CheckSideways())
	{
		return;
	}
}

void UAA_RacerVerbalBarksComponent::Deactivate()
{
	Super::Deactivate();

	if (!RacerContextProvider)
	{
		return;
	}

	const auto& Context = RacerContextProvider->GetRacerContext();

	if (auto Vehicle = Context.VehiclePawn; Vehicle && Vehicle->GetMesh())
	{
		// Ensure that we can register hit events
		Vehicle->GetMesh()->OnComponentHit.RemoveDynamic(this, &ThisClass::OnVehicleHit);
	}
}

bool UAA_RacerVerbalBarksComponent::CheckRelativePlayerPosition()
{
	check(RacerContextProvider);
	check(PlayerController);
	
	const auto& Context = RacerContextProvider->GetRacerContext();

	const auto& PlayerSplineInfo = PlayerController->GetPlayerSplineInfo();
	if (!PlayerSplineInfo)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: CheckRelativePlayerPosition - PlayerController %s does not yet have spline info set"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(PlayerController));

		return false;
	}

	const auto DistanceDelta = Context.RaceState.GetTotalDistance() - PlayerSplineInfo->RaceState.GetTotalDistance();
	const bool bAheadOfPlayer = DistanceDelta >= 0;

	if (PlayerPositionChanges.IsEmpty())
	{
		PlayerPositionChanges.Add(bAheadOfPlayer);
		return false;
	}

	if (bAheadOfPlayer && !PlayerPositionChanges.Last())
	{
		PlayerPositionChanges.Add(true);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-%s: CheckRelativePlayerPosition - AI Passed player"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		// TODO: May want to track if the "Just you wait...I'm gonna catch ya!" clip plays
		// and then if (PlayerPositionChanges.Num() > 2) then play the "Told ya, I would catch ya!" clip
		return PlayClipIfApplicable(PassPlayerAudioClip);
	}
	else if (!bAheadOfPlayer && PlayerPositionChanges.Last())
	{
		PlayerPositionChanges.Add(false);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-%s: CheckRelativePlayerPosition - Player passed AI"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		return PlayClipIfApplicable(PlayerPassesAudioClip);
	}

	return false;
}

bool UAA_RacerVerbalBarksComponent::CheckSideways()
{
	check(RacerContextProvider);

	auto Vehicle = RacerContextProvider->GetRacerContext().VehiclePawn;
	if (!Vehicle)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: CheckSideways - FALSE - Vehicle is nullptr on RacerContext"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return false;
	}

	const auto SpeedMph = Vehicle->GetVehicleSpeedMph();
	if (SpeedMph < SidewaysSpeedMphMin)
	{
		return false;
	}

	const auto TractionFraction = Vehicle->GetTraction();
	if (TractionFraction < SidewaysTractionFractionMax)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: CheckSideways - TRUE - Detected vehicle going sideways: SpeedMph=%f; TractionFraction=%f < SidewaysTractionFractionMax=%f"),
			*GetName(), *LoggingUtils::GetName(GetOwner()),
			SidewaysSpeedMphMin,
			TractionFraction, SidewaysTractionFractionMax
		);
		
		return PlayClipIfApplicable(SidewaysAudioClip);
	}

	return false;
}

bool UAA_RacerVerbalBarksComponent::PlayClipIfApplicable(USoundBase* Clip)
{
	if (!Clip)
	{
		return false;
	}

	FAudioState& AudioState = AudioStateMap[Clip];

	// We will handle concurrently playing clips in the concurrency settings of the audio asset by only allowing at most 1 sound active at a time
	// and then stopping previous before playing next one
	if (auto LastPlayed = AudioState.LastPlayedSound; LastPlayed && LastPlayed->IsPlaying())
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: PlayClipIfApplicable - FALSE - Clip=%s is already playing"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Clip->GetName());
		return false;
	}

	// Make sure we aren't in cooldown for this sound
	if (AudioState.LastPlayEndTimeSeconds > 0)
	{
		const auto TimeSeconds = GetWorld()->GetTimeSeconds();
		const auto DeltaTime = TimeSeconds - AudioState.LastPlayEndTimeSeconds;

		if(DeltaTime <= AudioState.CooldownTimeSeconds)
		{
			UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
				TEXT("%s-%s: PlayClipIfApplicable - FALSE - Clip=%s is still in cooldown; DeltaTime=%f <= %f"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Clip->GetName(), DeltaTime, AudioState.LastPlayEndTimeSeconds);
			return false;
		}
	}

	auto SpawnedAudioComponent = UGameplayStatics::SpawnSoundAttached(Clip, GetOwner()->GetRootComponent(), NAME_None,
		FVector::ZeroVector, EAttachLocation::KeepRelativeOffset, true);

	if (!SpawnedAudioComponent)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: PlayClipIfApplicable - FALSE - Unable to spawn audio component for clip=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Clip->GetName());
		return false;
	}

	SpawnedAudioComponent->bAutoDestroy = true;
	SpawnedAudioComponent->SetFloatParameter(PitchSoundParameterName, PitchOffsetValue);

	// subscribe to when the audio finishes playing - pass additional clip parameter so we can update the map
	auto AudioFinishedDelegate = FOnAudioFinishedNative::FDelegate::CreateUObject(this, &ThisClass::OnAudioComponentFinished, Clip);
	SpawnedAudioComponent->OnAudioFinishedNative.Add(AudioFinishedDelegate);

	SpawnedAudioComponent->Play();

	AudioState.LastPlayedSound = SpawnedAudioComponent;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: PlayClipIfApplicable - TRUE - Playing clip=%s -> %s with PitchOffsetValue=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *Clip->GetName(), *SpawnedAudioComponent->GetName(), PitchOffsetValue);
	
	return true;
}

void UAA_RacerVerbalBarksComponent::OnAudioComponentFinished(UAudioComponent* AudioComponent, USoundBase* Clip)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: OnAudioComponentFinished - AudioComponent=%s; Clip=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(AudioComponent), *LoggingUtils::GetName(Clip));

	if (!Clip)
	{
		return;
	}

	auto& AudioState = AudioStateMap[Clip];
	AudioState.LastPlayedSound = nullptr;
	AudioState.LastPlayEndTimeSeconds = GetWorld()->GetTimeSeconds();
}

void UAA_RacerVerbalBarksComponent::RegisterAudioClip(USoundBase* Clip, float CooldownTimeSeconds, const TCHAR* ParameterName)
{
	if (Clip)
	{
		AudioStateMap.Add(Clip, FAudioState{
			.CooldownTimeSeconds = CooldownTimeSeconds
		});
	}
	else
	{
		UE_LOG(LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - %s not defined"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), ParameterName);
	}
}

void UAA_RacerVerbalBarksComponent::OnVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: OnVehicleHit - OtherActor=%s; OtherComp=%s; NormalImpulse=%s; BlockingHit=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(OtherActor), *LoggingUtils::GetName(OtherComp),
		*NormalImpulse.ToCompactString(), LoggingUtils::GetBoolString(Hit.bBlockingHit)
	);

	check(RacerContextProvider);
	auto MyVehicle = RacerContextProvider->GetRacerContext().VehiclePawn;

	if (!MyVehicle || !Hit.bBlockingHit || (!OtherActor && !OtherComp))
	{
		return;
	}

	if (const auto Vehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor); Vehicle)
	{
		// if vehicle is player controlled, then play audio 
		if (Vehicle->GetController() && Vehicle->GetController()->IsPlayerController())
		{
			// check if impulse min is met
			const auto ImpulseMagnitude = NormalImpulse.Size();
			if (ImpulseMagnitude < VehicleHitMinImpulse)
			{
				UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
					TEXT("%s-%s: OnVehicleHit - FALSE - OtherActor=%s is player controlled, but ImpulseMagnitude=%f < VehicleHitMinImpulse=%f"),
					*GetName(), *LoggingUtils::GetName(GetOwner()), *Vehicle->GetName(), ImpulseMagnitude, VehicleHitMinImpulse);
				return;
			}

			// make sure player is instigator
			if (DetermineInstigatorVehicle(MyVehicle, Vehicle, Hit.Location) != Vehicle)
			{
				UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
					TEXT("%s-%s: OnVehicleHit - FALSE - AI hit player instead - OtherActor=%s is player controlled and ImpulseMagnitude=%f >= VehicleHitMinImpulse=%f"),
					*GetName(), *LoggingUtils::GetName(GetOwner()), *Vehicle->GetName(), ImpulseMagnitude, VehicleHitMinImpulse);
				return;
			}

			UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
				TEXT("%s-%s: OnVehicleHit - TRUE - Play Vehicle Hit Clip - OtherActor=%s is player controlled and ImpulseMagnitude=%f >= VehicleHitMinImpulse=%f"),
				*GetName(), *LoggingUtils::GetName(GetOwner()), *Vehicle->GetName(), ImpulseMagnitude, VehicleHitMinImpulse);

			PlayClipIfApplicable(HitByPlayerAudioClip);
		}

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: OnVehicleHit - FALSE - OtherActor=%s is not player controlled"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Vehicle->GetName());

		// CPU vehicle - don't play clip
		return;
	}

	const auto ImpulseMagnitude = NormalImpulse.Size();
	if (ImpulseMagnitude < PropHitMinImpulse)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: OnVehicleHit - FALSE - Hit Prop - OtherActor=%s; OtherComp=%s, but ImpulseMagnitude=%f < PropHitMinImpulse=%f"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(OtherActor), *LoggingUtils::GetName(OtherComp), ImpulseMagnitude, PropHitMinImpulse);
		return;
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: OnVehicleHit - TRUE - Play Hit Prop Clip - OtherActor=%s; OtherComp=%s, and ImpulseMagnitude=%f >= PropHitMinImpulse=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(OtherActor), *LoggingUtils::GetName(OtherComp), ImpulseMagnitude, PropHitMinImpulse);

	PlayClipIfApplicable(HitPropAudioClip);
}

const AAA_WheeledVehiclePawn* UAA_RacerVerbalBarksComponent::DetermineInstigatorVehicle(const AAA_WheeledVehiclePawn* MyVehicle,
	const AAA_WheeledVehiclePawn* OtherVehicle, const FVector& HitLocation) const
{
	check(MyVehicle);
	check(OtherVehicle);

	// determine who hit whom - If impulse vector in direction of movement then that is the instigator
	// if they are in same direction - one with greatest speed is the instigator

	const FVector& OwnerVelocity = MyVehicle->GetVelocity();
	const FVector& OtherVelocity = OtherVehicle->GetVelocity();

	const FVector ImpactDirectionRelativeToOther = (HitLocation - OtherVehicle->GetActorLocation()).GetSafeNormal();
	const FVector ImpactDirectionRelativeToOwner = (HitLocation - MyVehicle->GetActorLocation()).GetSafeNormal();

	const FVector OwnerVelocityDirection = OwnerVelocity.GetSafeNormal();
	const FVector OtherVelocityDirection = OtherVelocity.GetSafeNormal();

	const auto OwnerProjection = OwnerVelocityDirection | ImpactDirectionRelativeToOwner;
	const auto OtherProjection = OtherVelocityDirection | ImpactDirectionRelativeToOther;

	const AAA_WheeledVehiclePawn* InstigatorVehicle{};

	// Determine by speed if both in direction of impulse or both not in direction of impulse
	if ((OwnerProjection >= 0 && OtherProjection >= 0) ||
		OwnerProjection < 0 && OtherProjection < 0)
	{
		auto OwnerSpeed = OwnerVelocity.Size();
		auto OtherSpeed = OtherVelocity.Size();

		if (OwnerSpeed >= OtherSpeed)
		{
			InstigatorVehicle = MyVehicle;
		}
		else
		{
			InstigatorVehicle = OtherVehicle;
		}

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: Hit Vehicle %s InstigatorVehicle=%s determined by speed difference: OwnerProjection=%f; OtherProjection=%f; OwnerVelocity=%s; OtherVelocity=%s; HitLocation=%s; OwnerSpeed=%f; OtherSpeed=%f"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *OtherVehicle->GetName(), *InstigatorVehicle->GetName(),
			OwnerProjection,
			OtherProjection,
			*OwnerVelocity.ToCompactString(),
			*OtherVelocity.ToCompactString(),
			*HitLocation.ToCompactString(),
			OwnerSpeed,
			OtherSpeed
		);
	}
	else
	{
		// Choose instigator vehicle to be one most aligned with collision impulse
		// This could be buggy but only using it to determine if audio should play
		if (OtherProjection >= OwnerProjection)
		{
			InstigatorVehicle = OtherVehicle;
		}
		else
		{
			InstigatorVehicle = MyVehicle;
		}

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: Hit Vehicle %s InstigatorVehicle=%s: OwnerProjection=%f; OtherProjection=%f; OwnerVelocity=%s; OtherVelocity=%s; HitLocation=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *OtherVehicle->GetName(), *InstigatorVehicle->GetName(),
			OwnerProjection,
			OtherProjection,
			*OwnerVelocity.ToCompactString(),
			*OtherVelocity.ToCompactString(),
			*HitLocation.ToCompactString()
		);
	}

	return InstigatorVehicle;
}
