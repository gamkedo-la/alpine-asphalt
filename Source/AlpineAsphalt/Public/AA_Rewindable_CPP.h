#pragma once

#include "AA_Rewindable_CPP.generated.h"

UINTERFACE(Blueprintable)
class ALPINEASPHALT_API UAA_Rewindable_CPP : public UInterface
{
	GENERATED_BODY()	
};

class IAA_Rewindable_CPP
{
	GENERATED_BODY()
public:
	virtual void SetRewindTime(float Time);
	virtual void PauseRecordingSnapshots();
	virtual void ResumeRecordingSnapshots();
};

