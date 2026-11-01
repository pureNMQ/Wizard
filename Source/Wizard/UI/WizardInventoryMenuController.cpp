#include "UI/WizardInventoryMenuController.h"

bool UWizardInventoryMenuController::Open(UWizardInventoryComponent* InModel)
{
	Close();
	if (!IsValid(InModel)) return false;
	Model = InModel;
	SessionId = FGuid::NewGuid();
	ChangedHandle = Model->OnChanged.AddUObject(this, &ThisClass::HandleModelChanged);
	return true;
}
void UWizardInventoryMenuController::Close()
{
	if (Model.IsValid()) Model->OnChanged.Remove(ChangedHandle);
	ChangedHandle.Reset();
	Model.Reset();
	Selected = {};
	SessionId.Invalidate();
	OnChanged.Clear();
	OnFeedback.Clear();
}
FWizardInventorySnapshot UWizardInventoryMenuController::GetSnapshot() const { return Model.IsValid() ? Model->GetSnapshot() : FWizardInventorySnapshot(); }
void UWizardInventoryMenuController::Select(int32 Slot, bool bEquipment)
{
	if (!Model.IsValid()) return;
	Selected = bEquipment ? Model->MakeEquipmentRequest(static_cast<EWizardEquipmentSlot>(Slot)) : Model->MakeRequest(Slot);
	OnChanged.Broadcast();
}
FWizardInventoryEntry UWizardInventoryMenuController::GetSelectedEntry() const
{
	const auto Snapshot = GetSnapshot();
	const auto& Entries = Selected.bEquipment ? Snapshot.Equipment : Snapshot.Slots;
	if (!Entries.IsValidIndex(Selected.Slot) || Snapshot.Revision != Selected.Revision) return {};
	const auto& Entry = Entries[Selected.Slot];
	return Entry.InstanceId == Selected.InstanceId ? Entry : FWizardInventoryEntry();
}
bool UWizardInventoryMenuController::MakeDragRequest(int32 Slot, FWizardInventoryDragRequest& Request, bool bEquipment) const
{
	const auto Snapshot = GetSnapshot();
	const auto& Entries = bEquipment ? Snapshot.Equipment : Snapshot.Slots;
	if (!Model.IsValid() || !Entries.IsValidIndex(Slot) || Entries[Slot].IsEmpty()) return false;
	Request.Item = bEquipment ? Model->MakeEquipmentRequest(static_cast<EWizardEquipmentSlot>(Slot)) : Model->MakeRequest(Slot);
	Request.Session = const_cast<UWizardInventoryMenuController*>(this);
	Request.SessionId = SessionId;
	return true;
}
bool UWizardInventoryMenuController::HandleDrop(const FWizardInventoryDragRequest& Request, int32 TargetSlot, bool bEquipment)
{
	if (!Model.IsValid() || Request.Session.Get() != this || Request.SessionId != SessionId) return false;
	if (bEquipment) Report(Model->Equip(Request.Item, static_cast<EWizardEquipmentSlot>(TargetSlot)));
	else if (Request.Item.bEquipment) Report(Model->Unequip(Request.Item, TargetSlot));
	else Report(Model->Move(Request.Item, TargetSlot));
	return true;
}
void UWizardInventoryMenuController::UseSelected()
{
	if (!Model.IsValid()) return;
	const auto Entry = GetSelectedEntry();
	if (Selected.bEquipment) Report(Model->Unequip(Selected));
	else if (Entry.Definition && Entry.Definition->Type == EWizardItemType::Equipment) Report(Model->Equip(Selected, Entry.Definition->EquipmentSlot));
	else Report(Model->Use(Selected));
}
void UWizardInventoryMenuController::SplitSelected(int32 Quantity)
{
	FWizardInventoryQuantityRequest Request;
	if (PrepareQuantity(false, Request)) SubmitQuantity(Request, Quantity);
}
void UWizardInventoryMenuController::DiscardSelected(int32 Quantity)
{
	FWizardInventoryQuantityRequest Request;
	if (PrepareQuantity(true, Request)) SubmitQuantity(Request, Quantity);
}
bool UWizardInventoryMenuController::PrepareQuantity(bool bDiscard, FWizardInventoryQuantityRequest& Request)
{
	Request = {};
	if (!Model.IsValid() || !SessionId.IsValid() || GetSelectedEntry().IsEmpty())
	{
		Report(EWizardInventoryResult::StaleRequest);
		return false;
	}
	if (Selected.bEquipment)
	{
		Report(EWizardInventoryResult::InvalidSlot);
		return false;
	}
	if (!bDiscard && GetSelectedEntry().Quantity <= 1)
	{
		Report(EWizardInventoryResult::InvalidQuantity);
		return false;
	}
	Request.Item = Selected;
	Request.SessionId = SessionId;
	Request.bDiscard = bDiscard;
	return true;
}
void UWizardInventoryMenuController::SubmitQuantity(const FWizardInventoryQuantityRequest& Request, int32 Quantity)
{
	if (!Model.IsValid() || !SessionId.IsValid() || Request.SessionId != SessionId)
	{
		Report(EWizardInventoryResult::StaleRequest);
		return;
	}
	const auto Snapshot = Model->GetSnapshot();
	if (Request.Item.bEquipment || !Snapshot.Slots.IsValidIndex(Request.Item.Slot) || Snapshot.Revision != Request.Item.Revision ||
		!Request.Item.InstanceId.IsValid() || Snapshot.Slots[Request.Item.Slot].InstanceId != Request.Item.InstanceId)
	{
		Report(EWizardInventoryResult::StaleRequest);
		return;
	}
	if (!Request.bDiscard)
	{
		const int32 Empty = Snapshot.Slots.IndexOfByPredicate([](const FWizardInventoryEntry& Entry) { return Entry.IsEmpty(); });
		Report(Model->Split(Request.Item, Quantity, Empty));
		return;
	}
	Report(Model->Discard(Request.Item, Quantity));
}
void UWizardInventoryMenuController::HandleModelChanged()
{
	// 保留旧请求用于明确的StaleRequest反馈，显示层只将有效版本作为当前选择。
	OnChanged.Broadcast();
}
void UWizardInventoryMenuController::Report(EWizardInventoryResult Result)
{
	// 仅本会话显式提交成功时刷新同一实例的选择，外部更新始终保留旧版本。
	if (Result == EWizardInventoryResult::Success && Model.IsValid() && Selected.InstanceId.IsValid())
	{
		const auto Snapshot = Model->GetSnapshot();
		const int32 Slot = Snapshot.Slots.IndexOfByPredicate([this](const FWizardInventoryEntry& Entry) { return Entry.InstanceId == Selected.InstanceId; });
		if (Slot != INDEX_NONE) Selected = Model->MakeRequest(Slot);
		else
		{
			const int32 EquipmentSlot = Snapshot.Equipment.IndexOfByPredicate([this](const FWizardInventoryEntry& Entry) { return Entry.InstanceId == Selected.InstanceId; });
			Selected = Model->MakeEquipmentRequest(static_cast<EWizardEquipmentSlot>(EquipmentSlot));
		}
		OnChanged.Broadcast();
	}
	OnFeedback.Broadcast(Result);
}
void UWizardInventoryMenuController::BeginDestroy() { Close(); Super::BeginDestroy(); }
FText WizardInventory::ResultText(EWizardInventoryResult Result)
{
	const TCHAR* Text = TEXT("操作未完成");
	switch (Result)
	{
	case EWizardInventoryResult::Success: Text = TEXT(""); break;
	case EWizardInventoryResult::InvalidSlot: Text = TEXT("没有可用的目标格"); break;
	case EWizardInventoryResult::InvalidQuantity: Text = TEXT("数量无效；拆分数量需小于当前数量"); break;
	case EWizardInventoryResult::StaleRequest: Text = TEXT("物品已变化，请重新选择"); break;
	case EWizardInventoryResult::NoCapacity: Text = TEXT("背包空间不足"); break;
	case EWizardInventoryResult::NotEquipment: Text = TEXT("该物品不能装备"); break;
	case EWizardInventoryResult::WrongEquipmentSlot: Text = TEXT("装备槽位不匹配"); break;
	case EWizardInventoryResult::NotConsumable: Text = TEXT("该物品不能使用"); break;
	case EWizardInventoryResult::FullResource: Text = TEXT("对应生命或法力已满"); break;
	case EWizardInventoryResult::Dead: Text = TEXT("死亡时不能使用物品"); break;
	case EWizardInventoryResult::Busy: Text = TEXT("当前正在处理物品操作"); break;
	case EWizardInventoryResult::DropUnavailable: Text = TEXT("无法生成地面物品，物品已保留"); break;
	case EWizardInventoryResult::EffectFailed: Text = TEXT("药水效果未生效，物品已保留"); break;
	default: Text = TEXT("物品配置或状态无效"); break;
	}
	return FText::FromString(Text);
}
