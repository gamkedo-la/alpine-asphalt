#pragma once

#include "AA_ExtendedCommonActivatableWidget.h"
#include "AA_BaseUI.generated.h"

UCLASS(BlueprintType)
class ALPINEASPHALT_API UAA_BaseUI : public UAA_ExtendedCommonActivatableWidget
{
	
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	UAA_ExtendedCommonActivatableWidget* PushHUD(TSubclassOf<UAA_ExtendedCommonActivatableWidget> HUDClass);
	virtual UAA_ExtendedCommonActivatableWidget* PushHUD_Implementation(TSubclassOf<UAA_ExtendedCommonActivatableWidget> HUDClass);

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	UAA_ExtendedCommonActivatableWidget* PushMenu(TSubclassOf<UAA_ExtendedCommonActivatableWidget> MenuClass);
	virtual UAA_ExtendedCommonActivatableWidget* PushMenu_Implementation(TSubclassOf<UAA_ExtendedCommonActivatableWidget> HUDClass);

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	UAA_ExtendedCommonActivatableWidget* PushPopup(TSubclassOf<UAA_ExtendedCommonActivatableWidget> PopupClass);
	virtual UAA_ExtendedCommonActivatableWidget* PushPopup_Implementation(TSubclassOf<UAA_ExtendedCommonActivatableWidget> HUDClass);

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	void ShowLoadingScreen();
	virtual void ShowLoadingScreen_Implementation();

	UFUNCTION(BlueprintNativeEvent,BlueprintCallable)
	void HideLoadingScreen();
	virtual void HideLoadingScreen_Implementation();
};
