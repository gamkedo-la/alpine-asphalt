#pragma once

#include "AA_BaseActivity.generated.h"


class AAA_TrackInfoActor;

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
};