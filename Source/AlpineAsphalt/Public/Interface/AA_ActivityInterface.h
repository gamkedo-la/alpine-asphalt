#pragma once

#include "AA_ActivityInterface.generated.h"



UINTERFACE(Blueprintable)
class ALPINEASPHALT_API UAA_ActivityInterface : public UInterface
{
	GENERATED_BODY()	
};

class IAA_ActivityInterface
{
	GENERATED_BODY()
	
public:
	//loads the activity and all needed actors
	virtual void LoadActivity();

	//starts the activity
	virtual void StartActivity();

	//destroys the activity
	virtual void DestroyActivity();

};