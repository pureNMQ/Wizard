// Wizard 项目：数量弹窗持有冻结会话，防止取消、重放或离开商店后买卖。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GAS/WizardAttributeSet.h"
#include "Interaction/WizardInteractionTarget.h"
#include "Trading/WizardShopActor.h"
#include "Trading/WizardShopCatalog.h"
#include "UI/WizardShopMenuController.h"
#include "UI/WizardShopWidgets.h"
#include "WizardCharacter.h"
#include "WizardPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardShopMenuSessionTest, "Wizard.Trading.MenuSession", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardShopMenuSessionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AWizardPlayerState* State = World->SpawnActor<AWizardPlayerState>();
	UWizardInventoryComponent* Inventory = State->GetInventory();
	UWizardItemDefinition* Potion = NewObject<UWizardItemDefinition>(State);
	Potion->ItemId = TEXT("ShopSessionPotion"); Potion->Type = EWizardItemType::Consumable;
	Potion->PotionResource = EWizardPotionResource::Health; Potion->PotionMagnitude = 20.f;
	Potion->MaxStack = 10; Potion->SellPrice = 2;
	UWizardShopCatalog* Catalog = NewObject<UWizardShopCatalog>(State);
	FWizardShopProduct Product; Product.Definition = Potion; Product.BuyPrice = 5;
	Catalog->Products.Add(Product);
	State->TryAddGold(100);
	int32 Accepted = 0;
	Inventory->AddItems(Potion, 15, Accepted);
	UWizardShopMenuController* Menu = NewObject<UWizardShopMenuController>(State);
	bool bPermission = false;
	TestFalse(TEXT("无场景交互权限不能打开商店"), Menu->Open(Inventory, Catalog, [&]() { return bPermission; }));
	bPermission = true;
	TestTrue(TEXT("有效权限打开会话"), Menu->Open(Inventory, Catalog, [&]() { return bPermission; }));
	EWizardInventoryResult Feedback = EWizardInventoryResult::Success;
	Menu->OnFeedback.AddLambda([&](EWizardInventoryResult Result) { Feedback = Result; });
	FWizardShopQuantityRequest Sale;
	int32 Maximum = 0;
	TestTrue(TEXT("准备出售冻结物品"), Menu->PrepareSale(0, Sale, Maximum));
	TestEqual(TEXT("出售范围来自选中堆叠"), Maximum, 10);
	const auto Before = Inventory->GetSnapshot();
	TestEqual(TEXT("仅打开或取消数量弹窗不扣金币"), State->GetGold(), 100);
	TestTrue(TEXT("仅准备请求不移动物品"), Before.Slots == Inventory->GetSnapshot().Slots);
	bPermission = false;
	Menu->SubmitQuantity(Sale, 3);
	TestEqual(TEXT("确认时重验商店权限"), Feedback, EWizardInventoryResult::ShopUnavailable);
	TestEqual(TEXT("权限撤销不改金币"), State->GetGold(), 100);
	TestTrue(TEXT("权限撤销不改物品"), Before.Slots == Inventory->GetSnapshot().Slots);
	bPermission = true;
	FWizardShopQuantityRequest OtherSelection;
	Menu->PrepareSale(1, OtherSelection, Maximum);
	Menu->SubmitQuantity(Sale, 3);
	TestEqual(TEXT("改选后仍出售原来源"), Inventory->GetSnapshot().Slots[0].Quantity, 7);
	TestEqual(TEXT("后来选择的堆叠不受影响"), Inventory->GetSnapshot().Slots[1].Quantity, 5);
	TestEqual(TEXT("出售余额准确"), State->GetGold(), 106);
	Menu->SubmitQuantity(Sale, 3);
	TestEqual(TEXT("已成功请求不能重放"), Feedback, EWizardInventoryResult::StaleRequest);
	TestEqual(TEXT("重放不重复入账"), State->GetGold(), 106);
	FWizardShopQuantityRequest Purchase;
	TestTrue(TEXT("购买弹窗冻结报价"), Menu->PreparePurchase(0, Purchase, Maximum));
	Catalog->Products[0].BuyPrice = 6;
	Menu->SubmitQuantity(Purchase, 2);
	TestEqual(TEXT("报价变化拒绝旧确认"), Feedback, EWizardInventoryResult::StaleRequest);
	TestEqual(TEXT("旧报价不扣金币"), State->GetGold(), 106);
	Menu->PreparePurchase(0, Purchase, Maximum);
	Menu->SubmitQuantity(Purchase, 0);
	TestEqual(TEXT("零数量失败"), Feedback, EWizardInventoryResult::InvalidQuantity);
	TestEqual(TEXT("非法数量不扣金币"), State->GetGold(), 106);
	Menu->SubmitQuantity(Purchase, 2);
	TestEqual(TEXT("有效购买提交"), Feedback, EWizardInventoryResult::Success);
	TestEqual(TEXT("按实际目录价格扣费"), State->GetGold(), 94);
	FWizardShopQuantityRequest PreviousSession;
	Menu->PrepareSale(0, PreviousSession, Maximum);
	int32 Changed = 0;
	Menu->OnChanged.AddLambda([&]() { ++Changed; });
	Menu->Close();
	Inventory->AddItems(Potion, 1, Accepted);
	TestEqual(TEXT("关闭后解除物品事件"), Changed, 0);
	Menu->Open(Inventory, Catalog, [&]() { return bPermission; });
	Menu->OnFeedback.AddLambda([&](EWizardInventoryResult Result) { Feedback = Result; });
	Menu->SubmitQuantity(PreviousSession, 1);
	TestEqual(TEXT("重开商店拒旧会话"), Feedback, EWizardInventoryResult::StaleRequest);
	TestEqual(TEXT("旧会话出售不入账"), State->GetGold(), 94);
	Menu->Close();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

// 事务会话调用真正的场景权限，防止菜单打开后以陈旧交互状态继续交易。
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardShopInteractionTest, "Wizard.Trading.InteractionPermissions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardShopInteractionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	AWizardPlayerState* State = World->SpawnActor<AWizardPlayerState>();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardCharacter* Avatar = World->SpawnActor<AWizardCharacter>(AWizardCharacter::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator, Params);
	AWizardShopActor* Shop = World->SpawnActor<AWizardShopActor>();
	auto BoxActor = [&](const FVector& Location, const FVector& Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	Shop->SetActorLocation(FVector(150, 0, 100));
	Shop->ShopMenuClass = UWizardShopMenu::StaticClass();
	UWizardShopCatalog* Catalog = NewObject<UWizardShopCatalog>(Shop);
	UWizardItemDefinition* Potion = NewObject<UWizardItemDefinition>(Catalog);
	Potion->ItemId = TEXT("InteractionPotion"); Potion->Type = EWizardItemType::Consumable;
	Potion->PotionResource = EWizardPotionResource::Health; Potion->PotionMagnitude = 20.f;
	FWizardShopProduct Product; Product.Definition = Potion; Product.BuyPrice = 5;
	Catalog->Products.Add(Product);
	Shop->Catalog = Catalog;
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	State->InitializeAvatar(Avatar);
	TestTrue(TEXT("营地存活玩家在商人NPC范围内可交易"), Shop->CanTrade(State));
	TestFalse(TEXT("空玩家不可交易"), Shop->CanTrade(nullptr));
	State->SetInChallenge(true);
	TestFalse(TEXT("挑战期间拒绝营地交易"), Shop->CanTrade(State));
	State->SetInChallenge(false);
	Avatar->SetActorLocation(FVector(-200, 0, 100));
	TestFalse(TEXT("超出商人NPC中心交互距离拒绝交易"), Shop->CanTrade(State));
	Avatar->SetActorLocation(FVector(0, 0, 100));
	Shop->CampBounds->SetBoxExtent(FVector(50, 50, 500));
	Avatar->SetActorLocation(FVector(210, 0, 100));
	TestFalse(TEXT("范围内但离开营地拒绝交易"), Shop->CanTrade(State));
	Avatar->SetActorLocation(FVector(0, 0, 100));
	Shop->CampBounds->SetBoxExtent(FVector(1500, 1500, 500));
	Shop->InteractionTarget->SetInteractionEnabled(false);
	TestFalse(TEXT("目标禁用拒绝交易"), Shop->CanTrade(State));
	Shop->InteractionTarget->SetInteractionEnabled(true);
	TestTrue(TEXT("目标重新启用可交易"), Shop->CanTrade(State));
	Shop->SetActorLocation(FVector(1000, 0, 100));
	TestFalse(TEXT("商人NPC移开后交互目标随自身离开"), Shop->CanTrade(State));
	Shop->SetActorLocation(FVector(150, 0, 100));
	TestTrue(TEXT("商人NPC返回后交互恢复"), Shop->CanTrade(State));
	Catalog->bInfiniteStock = false;
	TestFalse(TEXT("不支持的库存配置拒绝交易"), Shop->CanTrade(State));
	Catalog->bInfiniteStock = true;
	Shop->Catalog = nullptr;
	TestFalse(TEXT("缺少目录拒绝交易"), Shop->CanTrade(State));
	Shop->Catalog = Catalog;
	Shop->ShopMenuClass = nullptr;
	TestFalse(TEXT("缺少界面配置拒绝交互"), Shop->CanTrade(State));
	Shop->ShopMenuClass = UWizardShopMenu::StaticClass();
	AActor* Wall = BoxActor(FVector(75, 0, 100), FVector(5, 50, 100));
	TestFalse(TEXT("目标被遮挡拒绝交易"), Shop->CanTrade(State));
	Wall->Destroy();
	TestTrue(TEXT("遮挡移除后提交重验可用"), Shop->CanTrade(State));
	State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	TestFalse(TEXT("死亡玩家拒绝交易"), Shop->CanTrade(State));
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
