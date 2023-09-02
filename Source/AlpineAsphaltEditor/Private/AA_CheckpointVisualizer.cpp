#include "..\Public\AA_CheckpointVisualizer.h"

#include "Components/AA_CheckpointComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"

IMPLEMENT_HIT_PROXY(HitCheckpointVisProxy, HComponentVisProxy)
IMPLEMENT_HIT_PROXY(HitCheckpointProxy, HitCheckpointVisProxy)

void AA_CheckpointVisualizer::OnRegister()
{
	FComponentVisualizer::OnRegister();
}

void AA_CheckpointVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	if(const UAA_CheckpointComponent* CheckpointComponent = Cast<UAA_CheckpointComponent>(Component))
	{
		const USplineComponent* Spline = CheckpointComponent->GetSpline();
		for(int i = 0; i < CheckpointComponent->CheckpointPositionData.Num(); i++)
		{
			PDI->SetHitProxy(new HitCheckpointProxy(Component,i));
			FTransform Transform = FTransform(CheckpointComponent->CheckpointPositionData[i].Rotation,CheckpointComponent->CheckpointPositionData[i].Position);
			FColor Color = SelectedCheckpointIndex == i ? FColor::Yellow : FColor::White;
			auto XAxis = UKismetMathLibrary::GetRightVector(CheckpointComponent->CheckpointPositionData[i].Rotation);
			auto YAxis = UKismetMathLibrary::GetUpVector(CheckpointComponent->CheckpointPositionData[i].Rotation);
			float Width = CheckpointComponent->CheckpointPositionData[i].Width;
			float Height = CheckpointComponent->CheckpointPositionData[i].Height;
			DrawRectangle(PDI,CheckpointComponent->CheckpointPositionData[i].Position, XAxis,YAxis,Color,Width,Height,0,25);
			PDI->SetHitProxy(NULL);
		}
	}
}



bool AA_CheckpointVisualizer::VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy,
                                                  const FViewportClick& Click)
{
	bool bEditing = false;
	if(VisProxy && VisProxy->Component.IsValid())
	{
		bEditing = true;
		if(VisProxy->IsA(HitCheckpointProxy::StaticGetType()))
		{
			HitCheckpointProxy* Proxy = static_cast<HitCheckpointProxy*>(VisProxy);
			SelectedCheckpointIndex = Proxy->TargetIndex;
			EditedComponent = Cast<UAA_CheckpointComponent>(VisProxy->Component.Get()->GetOwner()->GetComponentByClass(UAA_CheckpointComponent::StaticClass()));
		}else
		{
			SelectedCheckpointIndex = INDEX_NONE;
		}
	}
	return bEditing;
}

void AA_CheckpointVisualizer::EndEditing()
{
	SelectedCheckpointIndex = INDEX_NONE;
	EditedComponent = nullptr;
}


bool AA_CheckpointVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
	if(	EditedComponent
		&& SelectedCheckpointIndex > INDEX_NONE
		&& SelectedCheckpointIndex < EditedComponent->CheckpointPositionData.Num())
	{
		OutLocation = EditedComponent->CheckpointPositionData[SelectedCheckpointIndex].Position;
		return true;
	}
	return false;
}

bool AA_CheckpointVisualizer::GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient,
	FMatrix& OutMatrix) const
{

	OutMatrix = FMatrix::Identity;
	return true;
	
}

bool AA_CheckpointVisualizer::HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport,
                                               FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale)
{
	if(EditedComponent && SelectedCheckpointIndex != INDEX_NONE){
		const auto Spline = EditedComponent->GetSpline();
		FCheckpointStruct* CheckpointStruct = &EditedComponent->CheckpointPositionData[SelectedCheckpointIndex];

		//process translation
		if(!DeltaTranslate.IsNearlyZero())
		{
			FVector Location = CheckpointStruct->Position;
			Location += DeltaTranslate;
			const auto InputKey = Spline->FindInputKeyClosestToWorldLocation(Location);
			
			Location = Spline->GetLocationAtSplineInputKey(InputKey,ESplineCoordinateSpace::World);
			const FRotator Rotation = Spline->GetRotationAtSplineInputKey(InputKey,ESplineCoordinateSpace::World);
			
			CheckpointStruct->Position = Location;
			CheckpointStruct->Rotation = Rotation;
		}

		if(!DeltaRotate.IsNearlyZero())
		{
			CheckpointStruct->Rotation += DeltaRotate;
		}
		return true;
	}
	return false;
}

bool AA_CheckpointVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
	EInputEvent Event)
{
	bool bHandled = false;
	if(Key == EKeys::Delete && EditedComponent)
	{
		DeleteCheckpoint(SelectedCheckpointIndex);
		SelectedCheckpointIndex = INDEX_NONE;
		bHandled = true;
	}
	return bHandled;
}

TSharedPtr<SWidget> AA_CheckpointVisualizer::GenerateContextMenu() const
{
	return FComponentVisualizer::GenerateContextMenu();
}

bool AA_CheckpointVisualizer::IsVisualizingArchetype() const
{
	return false;
}

UActorComponent* AA_CheckpointVisualizer::GetEditedComponent() const
{
	return Cast<UActorComponent>(EditedComponent);
}

void AA_CheckpointVisualizer::DeleteCheckpoint(const int IndexToDelete) const
{
	if(IndexToDelete > INDEX_NONE && IndexToDelete < EditedComponent->CheckpointPositionData.Num())
	{
		EditedComponent->CheckpointPositionData.RemoveAt(IndexToDelete);
	}
}

