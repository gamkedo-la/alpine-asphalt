#pragma once

#include "Race/AA_RaceState.h"

#include "AA_BaseActivity.generated.h"


class AAA_TrackInfoActor;
class AAA_WheeledVehiclePawn;
class UAA_VehicleUI;
class AAA_PlayerController;

UCLASS(Abstract)
class ALPINEASPHALT_API UAA_BaseActivity : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(AAA_TrackInfoActor*);
	//loads the activity and all needed actors
	virtual void LoadActivity();

	//starts the activity
	virtual void StartActivity();

	//destroys the activity
	virtual void DestroyActivity();

	// Activity still active but in a completed state for the player E.g. a score screen is showing
	virtual bool IsPlayerCompleted() const;

	virtual TArray<FAA_RaceState> GetAllRaceStates() const PURE_VIRTUAL(, return TArray<FAA_RaceState>(););

protected:
	FAA_RaceState GetPlayerRaceState() const;
	AAA_PlayerController* GetPlayerController() const;
	UAA_VehicleUI* GetVehicleUI() const;
};