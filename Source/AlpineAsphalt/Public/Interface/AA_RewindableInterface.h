#pragma once

#include "AA_RewindableInterface.generated.h"

UINTERFACE(Blueprintable)
class ALPINEASPHALT_API UAA_RewindableInterface : public UInterface
{
	GENERATED_BODY()	
};

class IAA_RewindableInterface
{
	GENERATED_BODY()
public:
	virtual void SetRewindTime(float Time);
	virtual void PauseRecordingSnapshots();
	virtual void ResumeRecordingSnapshots();
	virtual void ResetRewindHistory();
};

