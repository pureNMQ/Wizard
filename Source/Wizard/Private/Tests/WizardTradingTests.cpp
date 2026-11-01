// Wizard 项目：公开买卖接口保护金币/物品原子提交、数量守恒、旧请求及同步通知重入。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Trading/WizardShopCatalog.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "Engine/World.h"

namespace WizardTradingTests
{
	struct FPlayerFixture
	{
		UWorld* World;
		AWizardPlayerState* Player;
		FPlayerFixture()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			Player = World->SpawnActor<AWizardPlayerState>();
		}
		~FPlayerFixture() { World->DestroyWorld(false); }
	};
	UWizardItemDefinition* Define(UObject* Outer, FName Id, int32 Limit = 10, EWizardItemType Type = EWizardItemType::Consumable)
	{
		UWizardItemDefinition* Definition = NewObject<UWizardItemDefinition>(Outer);
		Definition->ItemId = Id;
		Definition->MaxStack = Limit;
		Definition->Type = Type;
		Definition->SellPrice = 3;
		return Definition;
	}
	UWizardShopCatalog* Catalog(UObject* Outer, UWizardItemDefinition* Definition, int32 Price = 5)
	{
		UWizardShopCatalog* Result = NewObject<UWizardShopCatalog>(Outer);
		FWizardShopProduct Product;
		Product.Definition = Definition;
		Product.BuyPrice = Price;
		Result->Products.Add(Product);
		return Result;
	}
	int32 Total(const UWizardInventoryComponent* Inventory, FName Id)
	{
		int32 Count = 0;
		for (const FWizardInventoryEntry& Entry : Inventory->GetSnapshot().Slots) if (Entry.DefinitionId == Id) Count += Entry.Quantity;
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardTradingAtomicTest, "Wizard.Trading.AtomicTransactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardTradingAtomicTest::RunTest(const FString& Parameters)
{
	using namespace WizardTradingTests;
	FPlayerFixture Fixture;
	AWizardPlayerState* Player = Fixture.Player;
	UWizardInventoryComponent* Inventory = Player->GetInventory();
	UWizardItemDefinition* Potion = Define(Player, TEXT("TestPotion"));
	UWizardShopCatalog* Shop = Catalog(Player, Potion);
	Player->TryAddGold(100);
	const int64 FirstRevision = Inventory->GetSnapshot().Revision;
	TestEqual(TEXT("购买上限受余额限制"), Inventory->GetPurchaseLimit(Shop, 0), 20);
	bool bObservePurchase = true;
	const FDelegateHandle InventoryHandle = Inventory->OnChanged.AddLambda([&]()
	{
		if (!bObservePurchase) return;
		TestEqual(TEXT("物品通知观察到最终余额"), Player->GetGold(), 85);
		TestEqual(TEXT("物品通知观察到最终数量"), Total(Inventory, Potion->ItemId), 3);
		TestEqual(TEXT("通知期间仍能读取购买上限"), Inventory->GetPurchaseLimit(Shop, 0), 17);
		TestFalse(TEXT("物品通知不能修改金币"), Player->TryAddGold(1));
	});
	const FDelegateHandle GoldHandle = Player->OnGoldChanged.AddLambda([&]()
	{
		if (!bObservePurchase) return;
		TestEqual(TEXT("金币通知不能再次购买"), Inventory->Buy(Shop, 0, 1, Inventory->GetSnapshot().Revision), EWizardInventoryResult::Busy);
		TestEqual(TEXT("金币通知不能移动物品"), Inventory->Move(Inventory->MakeRequest(0), 1), EWizardInventoryResult::Busy);
	});
	TestEqual(TEXT("全量购买成功"), Inventory->Buy(Shop, 0, 3, FirstRevision), EWizardInventoryResult::Success);
	bObservePurchase = false;
	Inventory->OnChanged.Remove(InventoryHandle);
	Player->OnGoldChanged.Remove(GoldHandle);
	TestEqual(TEXT("购买请求不能重放"), Inventory->Buy(Shop, 0, 3, FirstRevision), EWizardInventoryResult::StaleRequest);
	const FWizardInventoryRequest Sale = Inventory->MakeRequest(0);
	TestEqual(TEXT("部分出售成功"), Inventory->Sell(Sale, 2), EWizardInventoryResult::Success);
	TestEqual(TEXT("出售扣除准确数量"), Total(Inventory, Potion->ItemId), 1);
	TestEqual(TEXT("出售收入准确"), Player->GetGold(), 91);
	TestEqual(TEXT("出售请求不能重放"), Inventory->Sell(Sale, 2), EWizardInventoryResult::StaleRequest);
	// 素材没有目录条目也能按同一物品定义机制出售。
	UWizardItemDefinition* Material = Define(Player, TEXT("NewMaterial"), 10, EWizardItemType::Material);
	int32 Accepted = 0;
	Inventory->AddItems(Material, 4, Accepted);
	TestEqual(TEXT("新增素材无需出售专用分支"), Inventory->Sell(Inventory->MakeRequest(1), 4), EWizardInventoryResult::Success);
	TestEqual(TEXT("素材售完清空来源"), Total(Inventory, Material->ItemId), 0);
	TestEqual(TEXT("素材收入正确"), Player->GetGold(), 103);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardTradingFailureTest, "Wizard.Trading.FailureBoundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardTradingFailureTest::RunTest(const FString& Parameters)
{
	using namespace WizardTradingTests;
	FPlayerFixture Fixture;
	AWizardPlayerState* Player = Fixture.Player;
	UWizardInventoryComponent* Inventory = Player->GetInventory();
	UWizardItemDefinition* Potion = Define(Player, TEXT("Potion"));
	UWizardShopCatalog* Shop = Catalog(Player, Potion);
	const int64 EmptyRevision = Inventory->GetSnapshot().Revision;
	TestEqual(TEXT("无金币不购买"), Inventory->Buy(Shop, 0, 1, EmptyRevision), EWizardInventoryResult::InsufficientGold);
	Player->TryAddGold(1000);
	TestEqual(TEXT("零数量拒绝"), Inventory->Buy(Shop, 0, 0, EmptyRevision), EWizardInventoryResult::InvalidQuantity);
	TestEqual(TEXT("非法商品拒绝"), Inventory->Buy(Shop, 1, 1, EmptyRevision), EWizardInventoryResult::UnknownDefinition);
	Shop->Products[0].BuyPrice = -1;
	TestEqual(TEXT("负价拒绝"), Inventory->Buy(Shop, 0, 1, EmptyRevision), EWizardInventoryResult::InvalidPrice);
	Shop->Products[0].BuyPrice = MAX_int32;
	TestEqual(TEXT("购买乘价溢出拒绝"), Inventory->Buy(Shop, 0, 2, EmptyRevision), EWizardInventoryResult::GoldOverflow);
	Shop->Products[0].BuyPrice = 5;
	UWizardItemDefinition* Staff = Define(Player, TEXT("Staff"), 1, EWizardItemType::Equipment);
	int32 Accepted = 0;
	Inventory->AddItems(Potion, 9, Accepted);
	Inventory->AddItems(Staff, 19, Accepted);
	const FWizardInventorySnapshot Before = Inventory->GetSnapshot();
	TestEqual(TEXT("剩余堆叠空间为1"), Inventory->GetPurchaseLimit(Shop, 0), 1);
	TestEqual(TEXT("只能接收部分也整笔失败"), Inventory->Buy(Shop, 0, 2, Before.Revision), EWizardInventoryResult::NoCapacity);
	TestEqual(TEXT("失败购买余额不变"), Player->GetGold(), 1000);
	TestTrue(TEXT("失败购买物品与身份不变"), Inventory->GetSnapshot().Slots == Before.Slots);
	TestEqual(TEXT("失败不推进版本"), Inventory->GetSnapshot().Revision, Before.Revision);
	TestEqual(TEXT("超来源数量拒绝"), Inventory->Sell(Inventory->MakeRequest(0), 10), EWizardInventoryResult::InvalidQuantity);
	Potion->bTradable = false;
	TestEqual(TEXT("不可交易拒售"), Inventory->Sell(Inventory->MakeRequest(0), 1), EWizardInventoryResult::NotTradable);
	Potion->bTradable = true;
	Potion->SellPrice = 0;
	TestEqual(TEXT("非法售价拒绝"), Inventory->Sell(Inventory->MakeRequest(0), 1), EWizardInventoryResult::InvalidPrice);
	Potion->SellPrice = MAX_int32;
	TestEqual(TEXT("出售乘价溢出拒绝"), Inventory->Sell(Inventory->MakeRequest(0), 2), EWizardInventoryResult::GoldOverflow);
	Potion->SellPrice = 3;
	Player->TryAddGold(MAX_int32 - Player->GetGold());
	TestEqual(TEXT("余额溢出拒售"), Inventory->Sell(Inventory->MakeRequest(0), 1), EWizardInventoryResult::GoldOverflow);
	TestTrue(TEXT("所有失败保持物品"), Inventory->GetSnapshot().Slots == Before.Slots);
	Inventory->Equip(Inventory->MakeRequest(1), EWizardEquipmentSlot::Hat);
	TestEqual(TEXT("装备中的物品不可直接出售"), Inventory->Sell(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Hat), 1), EWizardInventoryResult::NotTradable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardTradingCatalogTest, "Wizard.Trading.ConfigurationAndNotificationReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardTradingCatalogTest::RunTest(const FString& Parameters)
{
	using namespace WizardTradingTests;
	FPlayerFixture Fixture;
	AWizardPlayerState* Player = Fixture.Player;
	UWizardInventoryComponent* Inventory = Player->GetInventory();
	UWizardItemDefinition* Potion = Define(Player, TEXT("Potion"));
	UWizardShopCatalog* Shop = Catalog(Player, Potion);
	TestTrue(TEXT("有效目录"), Shop->IsConfigurationValid());
	const FWizardShopProduct DuplicateProduct = Shop->Products[0];
	Shop->Products.Add(DuplicateProduct);
	TestFalse(TEXT("重复定义拒绝"), Shop->IsConfigurationValid());
	Shop->Products.SetNum(1);
	Shop->bInfiniteStock = false;
	TestFalse(TEXT("未支持库存规则拒绝"), Shop->IsConfigurationValid());
	Shop->bInfiniteStock = true;
	UWizardItemDefinition* Other = Define(Player, TEXT("Potion"));
	int32 Accepted = 0;
	Inventory->AddItems(Other, 1, Accepted);
	Player->TryAddGold(10);
	TestEqual(TEXT("同ID冲突定义不能购买混入"), Inventory->Buy(Shop, 0, 1, Inventory->GetSnapshot().Revision), EWizardInventoryResult::InvalidDefinition);
	TestEqual(TEXT("冲突不扣金币"), Player->GetGold(), 10);
	const FDelegateHandle Handle = Player->OnGoldChanged.AddLambda([&]()
	{
		TestEqual(TEXT("拾金币通知期间不能出售"), Inventory->Sell(Inventory->MakeRequest(0), 1), EWizardInventoryResult::Busy);
		TestEqual(TEXT("拾金币通知期间不能增加物品"), Inventory->AddItems(Potion, 1, Accepted), EWizardInventoryResult::Busy);
		TestEqual(TEXT("拾金币通知期间不能恢复库存"), Inventory->RestoreSnapshot(Inventory->GetSnapshot()), EWizardInventoryResult::Busy);
	});
	Player->TryAddGold(1);
	Player->OnGoldChanged.Remove(Handle);
	TestEqual(TEXT("只有原金币入账"), Player->GetGold(), 11);
	return true;
}
#endif
