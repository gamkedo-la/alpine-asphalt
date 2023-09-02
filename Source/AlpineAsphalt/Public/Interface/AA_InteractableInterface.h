#pragma once

#include "AA_InteractableInterface.generated.h"

class AAA_PlayerController;

UINTERFACE(Blueprintable)
class ALPINEASPHALT_API UAA_InteractableInterface : public UInterface
{
	GENERATED_BODY()	
};

class IAA_InteractableInterface
{
	GENERATED_BODY()
public:
	virtual void Interact(AAA_PlayerController* Interactor);

};