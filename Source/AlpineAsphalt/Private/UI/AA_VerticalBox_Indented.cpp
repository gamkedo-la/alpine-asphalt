// Fill out your copyright notice in the Description page of Project Settings.


#include "UI\AA_VerticalBox_Indented.h"

#include "Components/VerticalBoxSlot.h"

void UAA_VerticalBox_Indented::SetIndentForAllChildren()
{
	UWidget* Widget;
	auto Children = GetAllChildren();
	//Remove collapsed children
	for(int i = Children.Num()-1; i >=0; i--)
	{
		if(Children[i]->GetVisibility() == ESlateVisibility::Collapsed)
		{
			Children.RemoveAt(i);
		}
	}
	//Start indenting
	for (int i = 0; i < Children.Num(); i++)
	{
		Widget = Children[i];
		auto InSlot = CastChecked<UVerticalBoxSlot>(Widget->Slot);
		FMargin Margin = InSlot->GetPadding();
		if(ReverseIndentOrder)
		{
			Margin.Left = (Children.Num() - i - 1) * IndentAmount;
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
void UAA_VerticalBox_Indented::UpdateIndent()
{
	SetIndentForAllChildren();
}
#if WITH_EDITOR
void UAA_VerticalBox_Indented::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	SetIndentForAllChildren();
	RebuildWidget();
}
#endif
