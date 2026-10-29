#include "UI/WizardSkillMenuController.h"
#include "Skills/WizardSkillLoadoutComponent.h"

bool UWizardSkillMenuController::Open(UWizardSkillLoadoutComponent* InModel)
{
	Close();
	if (!IsValid(InModel) || !InModel->Catalog || !InModel->CanEditLoadout()) return false;
	Model = InModel;
	ChangedHandle = Model->OnLoadoutChanged.AddUObject(this, &ThisClass::HandleModelChanged);
	return true;
}

void UWizardSkillMenuController::Close()
{
	if (Model.IsValid()) Model->OnLoadoutChanged.Remove(ChangedHandle);
	ChangedHandle.Reset();
	Model.Reset();
	OnChanged.Clear();
	OnFeedback.Clear();
}

void UWizardSkillMenuController::GetEntries(TArray<FWizardSkillMenuEntry>& Candidates, TArray<FWizardSkillMenuEntry>& Slots) const
{
	Candidates.Reset();
	Slots.Reset();
	if (!Model.IsValid() || !Model->Catalog) return;
	const FWizardLoadoutSnapshot Snapshot = Model->GetSnapshot();
	for (const UWizardSkillDefinition* Definition : Model->Catalog->Skills)
	{
		if (Definition) Candidates.Add({Definition, INDEX_NONE, !Snapshot.Slots.Contains(Definition->SkillId)});
	}
	for (int32 Index = 0; Index < Snapshot.Slots.Num(); ++Index) Slots.Add({Model->GetSlot(Index), Index, true});
}

bool UWizardSkillMenuController::CanDragSkill(FName Id, int32 SourceSlot) const
{
	if (!Model.IsValid() || !Model->Find(Id)) return false;
	if (SourceSlot == INDEX_NONE) return !Model->GetSnapshot().Slots.Contains(Id);
	const UWizardSkillDefinition* Source = Model->GetSlot(SourceSlot);
	return Source && Source->SkillId == Id;
}

bool UWizardSkillMenuController::HandleDrop(const FWizardSkillDragRequest& Request, int32 TargetSlot)
{
	if (Request.Source.Get() != this || !CanDragSkill(Request.SkillId, Request.SourceSlot)) return false;
	EWizardSkillResult Result;
	if (Request.SourceSlot != INDEX_NONE)
	{
		Result = TargetSlot == INDEX_NONE ? Model->UnequipSkill(Request.SourceSlot) : Model->SwapSlots(Request.SourceSlot, TargetSlot);
	}
	else
	{
		if (TargetSlot == INDEX_NONE) return false;
		Result = Model->EquipSkill(Request.SkillId, TargetSlot);
	}
	OnFeedback.Broadcast(Result);
	return true;
}

void UWizardSkillMenuController::HandleModelChanged() { OnChanged.Broadcast(); }
void UWizardSkillMenuController::BeginDestroy() { Close(); Super::BeginDestroy(); }
