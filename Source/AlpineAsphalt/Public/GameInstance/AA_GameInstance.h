#pragma once

#include "AA_GameInstance.generated.h"
UCLASS(Blueprintable,BlueprintType)
class UAA_GameInstance : public UGameInstance
{
	GENERATED_BODY()
public:

	UFUNCTION(Exec)
	void EndActivity() const;
};
