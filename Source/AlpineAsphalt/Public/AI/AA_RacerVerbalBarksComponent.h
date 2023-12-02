// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AA_RacerContextProvider.h"
#include "Interface/AA_BaseRewindable.h"

#include "AA_RacerVerbalBarksComponent.generated.h"

class AAA_WheeledVehiclePawn;
class AAA_PlayerController;
class UAudioComponent;
class USoundBase;

struct FAA_RacerVerbalBarksComponentSnapshotData
{
	TArray<bool,TInlineAllocator<8>> PlayerPositionChanges{};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_RacerVerbalBarksComponent : public UActorComponent, public TAA_BaseRewindable<FAA_RacerVerbalBarksComponentSnapshotData>
{
	GENERATED_BODY()

protected:
	using FSnapshotData = FAA_RacerVerbalBarksComponentSnapshotData;

public:	
	UAA_RacerVerbalBarksComponent();

	UFUNCTION()
	void OnStuck(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition, bool bAtMaxRetries);

	void OnPossessedVehiclePawn(AAA_WheeledVehiclePawn* VehiclePawn);

	virtual void Deactivate() override;
	virtual void Activate(bool bReset = false) override;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Inherited via TAA_BaseRewindable
	virtual FSnapshotData CaptureSnapshot() const override;
	virtual void RestoreFromSnapshot(const FSnapshotData& InSnapshotData, float InRewindTime) override;

	virtual void OnRewindBegin() override;

private:
	bool CheckRelativePlayerPosition();
	bool CheckSideways();
	void StopAllAudio();

	void RegisterEvents(AAA_WheeledVehiclePawn* VehiclePawn);
	void UnregisterEvents(AAA_WheeledVehiclePawn* VehiclePawn);

	bool PlayClipIfApplicable(USoundBase* Clip);

	void OnAudioComponentFinished(UAudioComponent* AudioComponent, USoundBase* Clip);

	void RegisterAudioClip(USoundBase* Clip, float CooldownTimeSeconds, const TCHAR* ParameterName);

	UFUNCTION()
	void OnVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	const AAA_WheeledVehiclePawn* DetermineInstigatorVehicle(const AAA_WheeledVehiclePawn* MyVehicle, const AAA_WheeledVehiclePawn* OtherVehicle, const FVector& HitLocation) const;

	// Inherited via TAA_BaseRewindable
	virtual UObject* AsUObject() override { return this; }

private:

	static const FName PitchSoundParameterName;

	struct FAudioState
	{
		UAudioComponent* LastPlayedSound{};
		float LastPlayEndTimeSeconds{};
		float CooldownTimeSeconds{};
	};

	UPROPERTY(Transient)
	TObjectPtr<AAA_PlayerController> PlayerController{};

	IAA_RacerContextProvider* RacerContextProvider{};

	TArray<bool, TInlineAllocator<8>> PlayerPositionChanges{};

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float SidewaysTractionFractionMax{ 0.2f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float SidewaysSpeedMphMin{ 15.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float PitchMinOffset{ -5.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float PitchMaxOffset{ 5.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float PassingAudioCooldownTimeSeconds { 10.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float HitByPlayerAudioCooldownTimeSeconds{ 5.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float StuckAudioCooldownTimeSeconds{ 30.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float HitPropAudioCooldownTimeSeconds{ 20.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float PropHitMinImpulse{ 10000.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float VehicleHitMinImpulse{ 5000.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	float SidewaysAudioCooldownTimeSeconds{ 5.0f };

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	TObjectPtr<USoundBase> SidewaysAudioClip{};

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	TObjectPtr<USoundBase> PassPlayerAudioClip{};

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	TObjectPtr<USoundBase> PlayerPassesAudioClip{};

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	TObjectPtr<USoundBase> HitByPlayerAudioClip{};

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	TObjectPtr<USoundBase> StuckAudioClip{};

	UPROPERTY(Category = Audio, EditDefaultsOnly)
	TObjectPtr<USoundBase> HitPropAudioClip{};

	TMap<USoundBase*, FAudioState> AudioStateMap{};
	float PitchOffsetValue{};
};
