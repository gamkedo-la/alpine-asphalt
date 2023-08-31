#include "..\Public\AA_CheckpointVisualizer.h"

#include "Components/AA_CheckpointComponent.h"
#include "Components/SplineComponent.h"

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
			FVector Location = Spline->GetLocationAtDistanceAlongSpline(CheckpointComponent->CheckpointPositionData[i].Position,
				ESplineCoordinateSpace::World);
			FTransform Transform = FTransform(Location);
			FLinearColor Color = SelectedCheckpointIndex == i ? FLinearColor::Yellow : FLinearColor::White;
			DrawWireSphere(PDI,Transform, Color,500,16,0,25);
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
		OutLocation = EditedComponent->
						GetSpline()->
						GetLocationAtDistanceAlongSpline(
							EditedComponent->
							CheckpointPositionData[SelectedCheckpointIndex].Position,ESplineCoordinateSpace::World);
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
		FVector Location = EditedComponent->GetSpline()->GetLocationAtDistanceAlongSpline(EditedComponent->CheckpointPositionData[SelectedCheckpointIndex].Position,ESplineCoordinateSpace::World);
		Location += DeltaTranslate;
		(EditedComponent->CheckpointPositionData[SelectedCheckpointIndex].Position = EditedComponent->GetSpline()->GetDistanceAlongSplineAtSplineInputKey(EditedComponent->GetSpline()->FindInputKeyClosestToWorldLocation(Location)));
		return true;
	}
	return true;
	//return FComponentVisualizer::HandleInputDelta(ViewportClient, Viewport, DeltaTranslate, DeltaRotate, DeltaScale);
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

