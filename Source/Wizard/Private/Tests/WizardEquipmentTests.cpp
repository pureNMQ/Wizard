// Wizard 项目：跨背包与装备事务必须守恒；失败及旧请求不能改变物品归属。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Inventory/WizardInventoryComponent.h"
#include "GAS/WizardAttributeSet.h"

namespace WizardEquipmentTests
{
	UWizardItemDefinition* Define(UObject* Outer, FName Id, EWizardEquipmentSlot Slot)
	{
		UWizardItemDefinition* Item = NewObject<UWizardItemDefinition>(Outer);
		Item->ItemId = Id;
		Item->Type = EWizardItemType::Equipment;
		Item->MaxStack = 1;
		Item->EquipmentSlot = Slot;
		return Item;
	}
	bool SameState(const FWizardInventorySnapshot& A, const FWizardInventorySnapshot& B)
	{
		return A.Slots == B.Slots && A.Equipment == B.Equipment && A.Revision == B.Revision;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEquipmentTransactionsTest, "Wizard.Equipment.Transactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardEquipmentTransactionsTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Model = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* First = WizardEquipmentTests::Define(Model, TEXT("FirstHat"), EWizardEquipmentSlot::Hat);
	UWizardItemDefinition* Second = WizardEquipmentTests::Define(Model, TEXT("SecondHat"), EWizardEquipmentSlot::Hat);
	UWizardItemDefinition* Boots = WizardEquipmentTests::Define(Model, TEXT("Boots"), EWizardEquipmentSlot::Boots);
	int32 Accepted = 0;
	Model->AddItems(First, 1, Accepted);
	Model->AddItems(Second, 1, Accepted);
	Model->AddItems(Boots, 1, Accepted);
	const FGuid FirstId = Model->GetSnapshot().Slots[0].InstanceId;
	const FGuid SecondId = Model->GetSnapshot().Slots[1].InstanceId;
	TestEqual(TEXT("四个固定装备槽"), Model->GetSnapshot().Equipment.Num(), 4);
	TestEqual(TEXT("首次装备"), Model->Equip(Model->MakeRequest(0), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
	TestTrue(TEXT("移动保留唯一实例身份"), Model->GetSnapshot().Slots[0].IsEmpty() && Model->GetSnapshot().Equipment[0].InstanceId == FirstId);
	const FWizardInventoryRequest Old = Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat);
	TestEqual(TEXT("替换装备"), Model->Equip(Model->MakeRequest(1), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
	TestTrue(TEXT("旧装备准确回到来源格"), Model->GetSnapshot().Slots[1].InstanceId == FirstId && Model->GetSnapshot().Equipment[0].InstanceId == SecondId);
	const auto Before = Model->GetSnapshot();
	TestEqual(TEXT("旧装备令牌拒绝"), Model->Unequip(Old), EWizardInventoryResult::StaleRequest);
	TestEqual(TEXT("错误槽位拒绝"), Model->Equip(Model->MakeRequest(2), EWizardEquipmentSlot::Hat), EWizardInventoryResult::WrongEquipmentSlot);
	TestEqual(TEXT("装备不能覆盖异类背包物品"), Model->Unequip(Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat), 2), EWizardInventoryResult::WrongEquipmentSlot);
	TestEqual(TEXT("装备请求不能误作背包移动"), Model->Move(Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat), 0), EWizardInventoryResult::InvalidSlot);
	TestTrue(TEXT("失败保持物品及版本"), WizardEquipmentTests::SameState(Before, Model->GetSnapshot()));
	TestEqual(TEXT("装备拖回同类占用背包格交换"), Model->Unequip(Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat), 1), EWizardInventoryResult::Success);
	TestTrue(TEXT("交换保留两件身份"), Model->GetSnapshot().Slots[1].InstanceId == SecondId && Model->GetSnapshot().Equipment[0].InstanceId == FirstId);
	TestEqual(TEXT("自动寻找空格卸下"), Model->Unequip(Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat)), EWizardInventoryResult::Success);
	TestTrue(TEXT("卸下后装备空且物品回到空格"), Model->GetSnapshot().Equipment[0].IsEmpty() && Model->GetSnapshot().Slots[0].InstanceId == FirstId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEquipmentCapacityTest, "Wizard.Equipment.FullInventoryAndReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardEquipmentCapacityTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Model = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* Hat = WizardEquipmentTests::Define(Model, TEXT("Hat"), EWizardEquipmentSlot::Hat);
	int32 Accepted = 0;
	Model->AddItems(Hat, 20, Accepted);
	Model->Equip(Model->MakeRequest(0), EWizardEquipmentSlot::Hat);
	Model->AddItems(Hat, 1, Accepted);
	const auto Before = Model->GetSnapshot();
	TestEqual(TEXT("满包卸下拒绝"), Model->Unequip(Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat)), EWizardInventoryResult::NoCapacity);
	TestTrue(TEXT("满包失败完全保留物品"), WizardEquipmentTests::SameState(Before, Model->GetSnapshot()));
	EWizardInventoryResult Reentry = EWizardInventoryResult::Success;
	const FDelegateHandle Handle = Model->OnChanged.AddLambda([&]()
	{
		Reentry = Model->Unequip(Model->MakeEquipmentRequest(EWizardEquipmentSlot::Hat));
	});
	const FGuid EquippedId = Before.Equipment[0].InstanceId;
	const FGuid IncomingId = Before.Slots[1].InstanceId;
	TestEqual(TEXT("满包仍能来源格替换"), Model->Equip(Model->MakeRequest(1), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
	Model->OnChanged.Remove(Handle);
	TestEqual(TEXT("属性/库存通知重入拒绝"), Reentry, EWizardInventoryResult::Busy);
	TestTrue(TEXT("满包替换保留双方身份"), Model->GetSnapshot().Slots[1].InstanceId == EquippedId && Model->GetSnapshot().Equipment[0].InstanceId == IncomingId);
	TestEqual(TEXT("只产生一次有效版本变更"), Model->GetSnapshot().Revision, Before.Revision + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEquipmentRestoreTest, "Wizard.Equipment.RestoreOwnershipValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardEquipmentRestoreTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Source = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* Hat = WizardEquipmentTests::Define(Source, TEXT("Hat"), EWizardEquipmentSlot::Hat);
	int32 Accepted = 0;
	Source->AddItems(Hat, 1, Accepted);
	Source->Equip(Source->MakeRequest(0), EWizardEquipmentSlot::Hat);
	const auto Valid = Source->GetSnapshot();
	UWizardInventoryComponent* Target = NewObject<UWizardInventoryComponent>();
	TestEqual(TEXT("恢复包含装备的统一快照"), Target->RestoreSnapshot(Valid), EWizardInventoryResult::Success);
	TestTrue(TEXT("恢复保留实例与配置"), Target->GetSnapshot().Equipment == Valid.Equipment);
	const auto Before = Target->GetSnapshot();
	auto Invalid = Valid;
	Invalid.Slots[0] = Invalid.Equipment[0];
	TestEqual(TEXT("拒绝背包与装备重复归属"), Target->RestoreSnapshot(Invalid), EWizardInventoryResult::InvalidSnapshot);
	Invalid = Valid;
	Invalid.Equipment.Swap(0, 1);
	TestEqual(TEXT("拒绝恢复到错误装备槽"), Target->RestoreSnapshot(Invalid), EWizardInventoryResult::InvalidSnapshot);
	Invalid = Valid;
	Invalid.Equipment.SetNum(3);
	TestEqual(TEXT("拒绝错误装备槽数量"), Target->RestoreSnapshot(Invalid), EWizardInventoryResult::InvalidSnapshot);
	TestTrue(TEXT("全部失败保持原有效状态"), WizardEquipmentTests::SameState(Before, Target->GetSnapshot()));
	FWizardEquipmentBonus Bonus;
	Bonus.Attribute = UWizardAttributeSet::GetAttackPowerAttribute();
	Bonus.Magnitude = 10.f;
	Hat->EquipmentBonuses.Add(Bonus);
	TestTrue(TEXT("合法战斗属性可数据配置"), Hat->IsConfigurationValid());
	Hat->EquipmentBonuses[0].Attribute = UWizardAttributeSet::GetIncomingDamageAttribute();
	TestFalse(TEXT("伤害元属性不能成为装备加成"), Hat->IsConfigurationValid());
	for (const FGameplayAttribute& Resource : { UWizardAttributeSet::GetHealthAttribute(), UWizardAttributeSet::GetManaAttribute(), UWizardAttributeSet::GetShieldAttribute() })
	{
		Hat->EquipmentBonuses[0].Attribute = Resource;
		TestFalse(TEXT("当前资源不接受装备加成，避免穿脱补充资源"), Hat->IsConfigurationValid());
	}
	return true;
}
#endif
