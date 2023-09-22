#include "GameInstance/AA_GameInstance.h"

#include "Subsystems/AA_ActivityManagerSubsystem.h"

void UAA_GameInstance::EndActivity() const
{
	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->DestroyActivity();
}
