// Wizard 项目：通过公开事务检查守恒、旧请求、恢复和真实GAS药水边界。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "WizardCharacter.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace WizardInventoryTests
{
	UWizardItemDefinition* Define(UObject* Outer, FName Id, int32 Limit, EWizardItemType Type = EWizardItemType::Material)
	{
		UWizardItemDefinition* Definition = NewObject<UWizardItemDefinition>(Outer);
		Definition->ItemId = Id;
		Definition->MaxStack = Limit;
		Definition->Type = Type;
		return Definition;
	}
	int32 Total(const FWizardInventorySnapshot& Snapshot, FName Id)
	{
		int32 Quantity = 0;
		for (const FWizardInventoryEntry& Entry : Snapshot.Slots) if (Entry.DefinitionId == Id) Quantity += Entry.Quantity;
		return Quantity;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInventoryTransactionsTest, "Wizard.Inventory.TransactionsAndRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInventoryTransactionsTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Inventory = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* Ore = WizardInventoryTests::Define(Inventory, TEXT("Ore"), 10);
	UWizardItemDefinition* Staff = WizardInventoryTests::Define(Inventory, TEXT("Staff"), 1, EWizardItemType::Equipment);
	int32 Events = 0;
	Inventory->OnChanged.AddLambda([&Events]() { ++Events; });
	int32 Accepted = 0;
	TestEqual(TEXT("20格固定"), Inventory->GetSnapshot().Slots.Num(), 20);
	TestEqual(TEXT("加入15素材"), Inventory->AddItems(Ore, 15, Accepted), EWizardInventoryResult::Success);
	TestEqual(TEXT("完整接收"), Accepted, 15);
	TestEqual(TEXT("一次广播"), Events, 1);
	const FWizardInventoryRequest OldRequest = Inventory->MakeRequest(1);
	TestEqual(TEXT("拆2个到空格"), Inventory->Split(Inventory->MakeRequest(0), 2, 2), EWizardInventoryResult::Success);
	TestEqual(TEXT("旧版本拒绝"), Inventory->Move(OldRequest, 3), EWizardInventoryResult::StaleRequest);
	TestEqual(TEXT("同类合并溢出"), Inventory->Move(Inventory->MakeRequest(0), 1), EWizardInventoryResult::Success);
	TestEqual(TEXT("目标达到上限"), Inventory->GetSnapshot().Slots[1].Quantity, 10);
	TestEqual(TEXT("来源保留溢出"), Inventory->GetSnapshot().Slots[0].Quantity, 3);
	TestEqual(TEXT("合并拆堆守恒"), WizardInventoryTests::Total(Inventory->GetSnapshot(), TEXT("Ore")), 15);
	TestEqual(TEXT("拒绝零拆分"), Inventory->Split(Inventory->MakeRequest(0), 0, 3), EWizardInventoryResult::InvalidQuantity);
	TestEqual(TEXT("拒绝占用目标"), Inventory->Split(Inventory->MakeRequest(0), 1, 1), EWizardInventoryResult::NoCapacity);
	TestEqual(TEXT("拒绝越界目标"), Inventory->Move(Inventory->MakeRequest(0), 20), EWizardInventoryResult::InvalidSlot);
	FWizardInventoryRequest WrongId = Inventory->MakeRequest(0);
	WrongId.InstanceId = FGuid::NewGuid();
	TestEqual(TEXT("拒绝错误身份"), Inventory->Move(WrongId, 3), EWizardInventoryResult::StaleRequest);
	TestEqual(TEXT("添加装备"), Inventory->AddItems(Staff, 2, Accepted), EWizardInventoryResult::Success);
	TestEqual(TEXT("装备单件单格"), Inventory->GetSnapshot().Slots[3].Quantity, 1);
	const FGuid StaffId = Inventory->GetSnapshot().Slots[3].InstanceId;
	const FGuid OreId = Inventory->GetSnapshot().Slots[0].InstanceId;
	Inventory->Move(Inventory->MakeRequest(0), 3);
	TestTrue(TEXT("交换保留双方身份"), Inventory->GetSnapshot().Slots[0].InstanceId == StaffId && Inventory->GetSnapshot().Slots[3].InstanceId == OreId);
	const FWizardInventorySnapshot Before = Inventory->GetSnapshot();
	const int32 BeforeEvents = Events;
	TestEqual(TEXT("未知定义失败"), Inventory->AddItems(nullptr, 2, Accepted), EWizardInventoryResult::UnknownDefinition);
	TestEqual(TEXT("负数失败"), Inventory->AddItems(Ore, -1, Accepted), EWizardInventoryResult::InvalidQuantity);
	FWizardInventorySnapshot Bad = Before;
	Bad.Slots[1].InstanceId = Bad.Slots[0].InstanceId;
	TestEqual(TEXT("重复ID拒绝恢复"), Inventory->RestoreSnapshot(Bad), EWizardInventoryResult::InvalidSnapshot);
	Bad = Before;
	Bad.Slots[0].Quantity = 2;
	TestEqual(TEXT("超上限装备拒绝恢复"), Inventory->RestoreSnapshot(Bad), EWizardInventoryResult::InvalidSnapshot);
	Bad = Before;
	Bad.Slots[1].DefinitionId = TEXT("Unknown");
	TestEqual(TEXT("未知定义拒绝恢复"), Inventory->RestoreSnapshot(Bad), EWizardInventoryResult::InvalidSnapshot);
	Inventory->RestoreSnapshot(Before);
	TestEqual(TEXT("失败和无变化不广播"), Events, BeforeEvents);
	TestTrue(TEXT("失败来源完全不变"), Inventory->GetSnapshot().Slots == Before.Slots);
	FWizardInventoryDropTicket Ticket;
	TestEqual(TEXT("准备部分丢弃"), Inventory->PrepareDrop(Inventory->MakeRequest(3), 1, Ticket), EWizardInventoryResult::Success);
	TestEqual(TEXT("接收失败不扣"), Inventory->CommitDrop(Ticket, [](const FWizardInventoryEntry&) { return false; }), EWizardInventoryResult::DropUnavailable);
	TestTrue(TEXT("失败丢弃保持快照"), Inventory->GetSnapshot().Slots == Before.Slots);
	TestEqual(TEXT("可靠接收后扣量"), Inventory->CommitDrop(Ticket, [&](const FWizardInventoryEntry& Entry)
	{
		TestTrue(TEXT("部分丢弃分配新身份"), Entry.InstanceId != Ticket.Entry.InstanceId);
		TestEqual(TEXT("回调重入被拒绝"), Inventory->Move(Inventory->MakeRequest(3), 5), EWizardInventoryResult::Busy);
		return true;
	}), EWizardInventoryResult::Success);
	TestEqual(TEXT("丢弃不可重放"), Inventory->CommitDrop(Ticket, [](const FWizardInventoryEntry&) { return true; }), EWizardInventoryResult::StaleRequest);
	Inventory->AddItems(Staff, MAX_int32, Accepted);
	TestEqual(TEXT("只能接收剩余15格"), Accepted, 15);
	const int64 FullRevision = Inventory->GetSnapshot().Revision;
	TestEqual(TEXT("满背包失败"), Inventory->AddItems(Staff, 1, Accepted), EWizardInventoryResult::NoCapacity);
	TestEqual(TEXT("满背包失败不变更版本"), Inventory->GetSnapshot().Revision, FullRevision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInventoryConfigurationTest, "Wizard.Inventory.ConfigurationValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInventoryConfigurationTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Inventory = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* Ore = WizardInventoryTests::Define(Inventory, TEXT("Ore"), 10);
	UWizardItemDefinition* Staff = WizardInventoryTests::Define(Inventory, TEXT("Staff"), 2, EWizardItemType::Equipment);
	TestFalse(TEXT("装备配置堆叠上限2无效"), Staff->IsConfigurationValid());
	int32 Accepted = 0;
	TestEqual(TEXT("错误定义拒绝接收"), Inventory->AddItems(Staff, 1, Accepted), EWizardInventoryResult::InvalidDefinition);
	TestEqual(TEXT("错误配置不接收物品"), Accepted, 0);
	TestEqual(TEXT("未登记且无图标的有效定义可以接收"), Inventory->AddItems(Ore, 15, Accepted), EWizardInventoryResult::Success);
	UWizardItemDefinition* ConflictingOre = WizardInventoryTests::Define(Inventory, Ore->ItemId, 10);
	const auto Before = Inventory->GetSnapshot();
	TestEqual(TEXT("同ID不同定义不能混入已有堆叠"), Inventory->AddItems(ConflictingOre, 1, Accepted), EWizardInventoryResult::InvalidDefinition);
	TestTrue(TEXT("冲突定义不改变库存"), Before.Slots == Inventory->GetSnapshot().Slots);
	Staff->MaxStack = 1;
	TestEqual(TEXT("修正定义后可以正常接收"), Inventory->AddItems(Staff, 1, Accepted), EWizardInventoryResult::Success);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInventoryPotionTest, "Wizard.Inventory.PotionsAndReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInventoryPotionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AWizardPlayerState* Player = World->SpawnActor<AWizardPlayerState>();
	AWizardCharacter* Avatar = World->SpawnActor<AWizardCharacter>();
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	Player->InitializeAvatar(Avatar);
	const FWizardInventorySnapshot EmptyInventory = Player->GetInventory()->GetSnapshot();
	TestFalse(TEXT("新玩家背包为空"), EmptyInventory.Slots.ContainsByPredicate([](const FWizardInventoryEntry& Entry) { return !Entry.IsEmpty(); }));
	Player->InitializeAvatar(Avatar);
	TestTrue(TEXT("重复绑定角色仍为空背包"), Player->GetInventory()->GetSnapshot().Slots == EmptyInventory.Slots);
	UWizardInventoryComponent* Inventory = NewObject<UWizardInventoryComponent>(Player);
	UWizardItemDefinition* Health = WizardInventoryTests::Define(Inventory, TEXT("HealthPotion"), 10, EWizardItemType::Consumable);
	Health->PotionResource = EWizardPotionResource::Health;
	Health->PotionMagnitude = 25.f;
	UWizardItemDefinition* Mana = WizardInventoryTests::Define(Inventory, TEXT("ManaPotion"), 10, EWizardItemType::Consumable);
	Mana->PotionResource = EWizardPotionResource::Mana;
	Mana->PotionMagnitude = 30.f;
	int32 Accepted = 0;
	Inventory->AddItems(Health, 3, Accepted);
	Inventory->AddItems(Mana, 2, Accepted);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	const int64 Before = Inventory->GetSnapshot().Revision;
	TestEqual(TEXT("满血失败"), Inventory->Use(Inventory->MakeRequest(0)), EWizardInventoryResult::FullResource);
	TestEqual(TEXT("满蓝失败"), Inventory->Use(Inventory->MakeRequest(1)), EWizardInventoryResult::FullResource);
	TestEqual(TEXT("满值不变更版本"), Inventory->GetSnapshot().Revision, Before);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 90.f);
	EWizardInventoryResult Reentry = EWizardInventoryResult::Success;
	const FDelegateHandle Handle = ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddLambda([&](const FOnAttributeChangeData&)
	{
		Reentry = Inventory->Use(Inventory->MakeRequest(0));
	});
	TestEqual(TEXT("真实GE恢复生命"), Inventory->Use(Inventory->MakeRequest(0)), EWizardInventoryResult::Success);
	ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(Handle);
	TestEqual(TEXT("属性上限夹取"), ASC->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()), 100.f);
	TestEqual(TEXT("属性回调重入拒绝"), Reentry, EWizardInventoryResult::Busy);
	TestEqual(TEXT("只扣一瓶"), Inventory->GetSnapshot().Slots[0].Quantity, 2);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 5.f);
	TestEqual(TEXT("真实GE恢复法力"), Inventory->Use(Inventory->MakeRequest(1)), EWizardInventoryResult::Success);
	TestEqual(TEXT("法力加30"), ASC->GetNumericAttribute(UWizardAttributeSet::GetManaAttribute()), 35.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	TestEqual(TEXT("死亡拒绝喝药"), Inventory->Use(Inventory->MakeRequest(0)), EWizardInventoryResult::Dead);
	TestEqual(TEXT("死亡不扣药"), Inventory->GetSnapshot().Slots[0].Quantity, 2);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInventoryDefinitionRestoreTest, "Wizard.Inventory.DefinitionIdRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInventoryDefinitionRestoreTest::RunTest(const FString& Parameters)
{
	const UWizardItemDefinition* Ore = LoadObject<UWizardItemDefinition>(nullptr, TEXT("/Game/Wizard/Data/Items/DA_ArcaneOre.DA_ArcaneOre"));
	if (!TestNotNull(TEXT("交付物品定义"), Ore)) return false;
	UWizardInventoryComponent* Source = NewObject<UWizardInventoryComponent>();
	int32 Accepted = 0;
	Source->AddItems(Ore, 1, Accepted);
	FWizardInventorySnapshot Snapshot = Source->GetSnapshot();
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FObjectAndNameAsStringProxyArchive Save(Writer, false);
	Save.ArIsSaveGame = true;
	FWizardInventorySnapshot::StaticStruct()->SerializeItem(Save, &Snapshot, nullptr);
	FWizardInventorySnapshot Loaded;
	FMemoryReader Reader(Bytes);
	FObjectAndNameAsStringProxyArchive Load(Reader, true);
	Load.ArIsSaveGame = true;
	FWizardInventorySnapshot::StaticStruct()->SerializeItem(Load, &Loaded, nullptr);
	if (!TestEqual(TEXT("保存20格快照"), Loaded.Slots.Num(), UWizardInventoryComponent::Capacity)) return false;
	TestNull(TEXT("快照不保存运行时定义指针"), Loaded.Slots[0].Definition.Get());
	TestEqual(TEXT("快照保留稳定物品ID"), Loaded.Slots[0].DefinitionId, Ore->ItemId);
	UWizardInventoryComponent* Target = NewObject<UWizardInventoryComponent>();
	TestEqual(TEXT("无需手工目录，通过自动扫描恢复定义"), Target->RestoreSnapshot(Loaded), EWizardInventoryResult::Success);
	TestTrue(TEXT("恢复为真实定义引用"), Target->GetSnapshot().Slots[0].Definition == Ore);
	TestEqual(TEXT("恢复数量正确"), WizardInventoryTests::Total(Target->GetSnapshot(), Ore->ItemId), 1);
	Loaded.Slots[0].DefinitionId = TEXT("MissingDefinition");
	TestEqual(TEXT("未知ID拒绝恢复且保留已有物品"), Target->RestoreSnapshot(Loaded), EWizardInventoryResult::InvalidSnapshot);
	TestEqual(TEXT("失败不丢现有库存"), WizardInventoryTests::Total(Target->GetSnapshot(), Ore->ItemId), 1);
	return true;
}
#endif
