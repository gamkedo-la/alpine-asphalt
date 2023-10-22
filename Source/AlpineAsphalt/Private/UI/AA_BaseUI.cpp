#include "UI/AA_BaseUI.h"

UAA_ExtendedCommonActivatableWidget* UAA_BaseUI::PushHUD_Implementation(
	TSubclassOf<UAA_ExtendedCommonActivatableWidget> HUDClass)
{
	return nullptr;
}

UAA_ExtendedCommonActivatableWidget* UAA_BaseUI::PushMenu_Implementation(
	TSubclassOf<UAA_ExtendedCommonActivatableWidget> MenuClass)
{
	return nullptr;
}

UAA_ExtendedCommonActivatableWidget* UAA_BaseUI::PushPopup_Implementation(
	TSubclassOf<UAA_ExtendedCommonActivatableWidget> PopupClass)
{
	return nullptr;
}

void UAA_BaseUI::ShowLoadingScreen_Implementation(){}

void UAA_BaseUI::HideLoadingScreen_Implementation(){}

void UAA_BaseUI::ShowPauseOverlay_Implementation(){}

void UAA_BaseUI::HidePauseOverlay_Implementation(){}
