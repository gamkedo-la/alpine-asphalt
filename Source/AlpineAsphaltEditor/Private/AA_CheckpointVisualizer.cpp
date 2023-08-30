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
			FLinearColor Color = SelectedTargetIndex == i ? FLinearColor::Yellow : FLinearColor::White;
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
			SelectedTargetIndex = Proxy->TargetIndex;
			EditedComponent = Cast<UAA_CheckpointComponent>(VisProxy->Component.Get()->GetOwner()->GetComponentByClass(UAA_CheckpointComponent::StaticClass()));
		}else
		{
			SelectedTargetIndex = INDEX_NONE;
		}
	}
	return bEditing;
}

void AA_CheckpointVisualizer::EndEditing()
{
	SelectedTargetIndex = INDEX_NONE;
	EditedComponent = nullptr;
}


bool AA_CheckpointVisualizer::GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const
{
	if(EditedComponent && SelectedTargetIndex != INDEX_NONE){
		const FVector Temp = EditedComponent->GetSpline()->GetLocationAtDistanceAlongSpline(EditedComponent->CheckpointPositionData[SelectedTargetIndex].Position,ESplineCoordinateSpace::World);
		OutLocation = Temp;
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
	return true;
	//return FComponentVisualizer::HandleInputDelta(ViewportClient, Viewport, DeltaTranslate, DeltaRotate, DeltaScale);
}

bool AA_CheckpointVisualizer::HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
	EInputEvent Event)
{
	return FComponentVisualizer::HandleInputKey(ViewportClient, Viewport, Key, Event);
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

