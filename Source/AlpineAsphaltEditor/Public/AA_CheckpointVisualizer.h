#pragma once

#include "ComponentVisualizer.h"
class UAA_CheckpointComponent;

struct HitCheckpointVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HitCheckpointVisProxy(const UActorComponent* InComponent) : HComponentVisProxy(InComponent,HPP_Wireframe){}
};
struct HitCheckpointProxy : public HitCheckpointVisProxy
{
	DECLARE_HIT_PROXY()

	HitCheckpointProxy(const UActorComponent* InComponent, int32 InTargetIndex) : HitCheckpointVisProxy(InComponent), TargetIndex(InTargetIndex){}
	
	int32 TargetIndex;
};
class AA_CheckpointVisualizer : public FComponentVisualizer
{
public:
	virtual void OnRegister() override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	//virtual void DrawVisualizationHUD(const UActorComponent* Component, const FViewport* Viewport, const FSceneView* View, FCanvas* Canvas) override;
	virtual bool VisProxyHandleClick(FEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual void EndEditing() override;
	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;
	virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override;
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual TSharedPtr<SWidget> GenerateContextMenu() const override;
	virtual bool IsVisualizingArchetype() const override;
	virtual UActorComponent* GetEditedComponent() const override;
private:
	int32 SelectedTargetIndex = INDEX_NONE;
	UAA_CheckpointComponent* EditedComponent = nullptr;
};
