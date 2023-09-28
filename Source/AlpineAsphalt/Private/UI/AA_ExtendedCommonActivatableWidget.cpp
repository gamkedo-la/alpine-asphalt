#include "..\..\Public\UI\AA_ExtendedCommonActivatableWidget.h"

#include "CommonUI/Private/Input/UIActionRouterTypes.h"
#include "Input/CommonUIInputTypes.h"

void UAA_ExtendedCommonActivatableWidget::NativeDestruct()
{
	for (FUIActionBindingHandle Handle : BindingHandles)
	{
		if (Handle.IsValid())
		{
			Handle.Unregister();
		}
	}
	BindingHandles.Empty();

	Super::NativeDestruct();
}

void UAA_ExtendedCommonActivatableWidget::RegisterBinding(FDataTableRowHandle InputAction, const FInputActionExecutedDelegate& Callback, FInputActionBindingHandle& BindingHandle)
{
	FBindUIActionArgs BindArgs(InputAction, FSimpleDelegate::CreateLambda([InputAction, Callback]()
	{
		Callback.ExecuteIfBound(InputAction.RowName);
	}));
	BindArgs.bDisplayInActionBar = true;
	
	BindingHandle.Handle = RegisterUIActionBinding(BindArgs);
	BindingHandles.Add(BindingHandle.Handle);
}

void UAA_ExtendedCommonActivatableWidget::UnregisterBinding(FInputActionBindingHandle BindingHandle)
{
	if (BindingHandle.Handle.IsValid())
	{
		BindingHandle.Handle.Unregister();
		BindingHandles.Remove(BindingHandle.Handle);
	}
}

void UAA_ExtendedCommonActivatableWidget::UnregisterAllBindings()
{
	for (FUIActionBindingHandle Handle : BindingHandles)
	{
		Handle.Unregister();
	}
	BindingHandles.Empty();
}