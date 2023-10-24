// Fill out your copyright notice in the Description page of Project Settings.


#include "UI\AA_VerticalBox_Indented.h"

#include "Components/VerticalBoxSlot.h"

void UAA_VerticalBox_Indented::SetIndentForAllChildren()
{
	UWidget* Widget;
	for (int i = 0; i < GetChildrenCount(); i++)
	{
		Widget = GetChildAt(i);
		auto InSlot = CastChecked<UVerticalBoxSlot>(Widget->Slot);
		FMargin Margin = InSlot->GetPadding();
		if(ReverseIndentOrder)
		{
			Margin.Left = (GetChildrenCount() - i - 1) * IndentAmount;
		}else
		{
			Margin.Left = i * IndentAmount;
		}
		InSlot->SetPadding(Margin);
	}
}

void UAA_VerticalBox_Indented::OnSlotAdded(UPanelSlot* InSlot)
{
	Super::OnSlotAdded(InSlot);

	SetIndentForAllChildren();
}

void UAA_VerticalBox_Indented::OnSlotRemoved(UPanelSlot* InSlot)
{
	Super::OnSlotRemoved(InSlot);
	
	SetIndentForAllChildren();
}

TSharedRef<SWidget> UAA_VerticalBox_Indented::RebuildWidget()
{
	SetIndentForAllChildren();
	return Super::RebuildWidget();
}

#if WITH_EDITOR
void UAA_VerticalBox_Indented::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SetIndentForAllChildren();
	RebuildWidget();
}
#endif
