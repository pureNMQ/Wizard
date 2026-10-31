#include "UI/WizardIconDragPreview.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"

void UWizardIconDragPreview::Configure(UTexture2D* Texture)
{
	if (Icon) Icon->SetBrushFromTexture(Texture);
}

void WizardIconDrag::SetVisual(UDragDropOperation* Operation, APlayerController* Player, TSubclassOf<UWizardIconDragPreview> PreviewClass, UTexture2D* Texture)
{
	if (!Operation || !IsValid(Player) || !PreviewClass) return;
	Operation->Pivot = EDragPivot::CenterCenter;
	if (UWizardIconDragPreview* Visual = CreateWidget<UWizardIconDragPreview>(Player, PreviewClass))
	{
		Visual->Configure(Texture);
		Operation->DefaultDragVisual = Visual;
	}
}
