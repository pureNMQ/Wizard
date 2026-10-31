// Wizard 项目：通过真实重叠事件保护拾取守恒、单次尝试、重入和换角生命周期。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Loot/WizardWorldPickup.h"
#include "Loot/WizardLootSettings.h"
#include "Loot/WizardDropConfig.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/WorldSettings.h"

namespace WizardPickupTests
{
	struct FFixture
	{
		UWorld* World = nullptr;
		AWizardPlayerState* Player = nullptr;
		ACharacter* Avatar = nullptr;
		UWizardInventoryComponent* Inventory = nullptr;
		UWizardItemDefinition* Ore = nullptr;
		FFixture()
		{
			const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
			if (!World) return;
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			World->SetGameInstance(NewObject<UGameInstance>(GEngine));
			Player = World->SpawnActor<AWizardPlayerState>();
			Avatar = SpawnAvatar(FVector(0,0,100));
			if (!Player || !Avatar) return;
			Inventory = Player->GetInventory();
			Ore = NewObject<UWizardItemDefinition>(Inventory);
			Ore->ItemId = TEXT("PickupTestOre");
			Ore->MaxStack = 10;
			// 故意不配置图标或目录，验证表现缺失不影响真实拾取事务。
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
			Avatar->SetPlayerState(Player);
			Player->InitializeAvatar(Avatar);
		}
		~FFixture()
		{
			if (World) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
		}
		ACharacter* SpawnAvatar(const FVector& Location)
		{
			FActorSpawnParameters Spawn;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<ACharacter>(Location, FRotator::ZeroRotator, Spawn);
		}
		bool IsReady() const { return Player && Avatar && Inventory && Ore && Player->GetPawn() == Avatar; }
		int32 Total() const
		{
			int32 Quantity = 0;
			for (const FWizardInventoryEntry& Entry : Inventory->GetSnapshot().Slots) Quantity += Entry.Quantity;
			return Quantity;
		}
		AWizardWorldPickup* SpawnItem(int32 Quantity)
		{
			return AWizardWorldPickup::SpawnItem(World, Ore, Quantity, Avatar->GetActorLocation());
		}
		void Reenter(AWizardWorldPickup* Pickup, ACharacter* Character = nullptr)
		{
			ACharacter* Target = Character ? Character : Avatar;
			Target->SetActorLocation(Pickup->GetActorLocation() + FVector(Pickup->PickupRadius + 100.f,0,0));
			Target->SetActorLocation(Pickup->GetActorLocation());
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardPickupPartialTest, "Wizard.Pickup.PartialCapacityAndReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardPickupPartialTest::RunTest(const FString& Parameters)
{
	WizardPickupTests::FFixture Fixture;
	if (!TestTrue(TEXT("真实拾取世界可用"), Fixture.IsReady())) return false;
	int32 Accepted = 0;
	Fixture.Inventory->AddItems(Fixture.Ore, 195, Accepted);
	AWizardWorldPickup* Pickup = Fixture.SpawnItem(12);
	if (!TestNotNull(TEXT("准备12个地面素材"), Pickup)) return false;
	TestEqual(TEXT("未激活前不发生拾取"), Fixture.Total(), 195);
	bool bReentryAccepted = true;
	const FDelegateHandle Handle = Fixture.Inventory->OnChanged.AddLambda([&]() { bReentryAccepted = Pickup->TransferTo([](int32 Offered) { return Offered; }); });
	Pickup->ActivatePickup();
	Fixture.Inventory->OnChanged.Remove(Handle);
	TestFalse(TEXT("背包事件重入不能重复接收同一掉落"), bReentryAccepted);
	TestEqual(TEXT("重叠开始只接收5个容量"), Fixture.Total(), 200);
	TestEqual(TEXT("地面准确留下7个"), Pickup->GetQuantity(), 7);
	Fixture.Reenter(Pickup);
	TestEqual(TEXT("满背包重新进入仍保留余量"), Pickup->GetQuantity(), 7);
	FWizardInventoryDropTicket Ticket;
	Fixture.Inventory->PrepareDrop(Fixture.Inventory->MakeRequest(0), 10, Ticket);
	int32 Outgoing = 0;
	Fixture.Inventory->CommitDrop(Ticket, [&](const FWizardInventoryEntry& Entry) { Outgoing = Entry.Quantity; return true; });
	TestEqual(TEXT("腾出容量时不在原地自动重试"), Pickup->GetQuantity(), 7);
	Fixture.Player->InitializeAvatar(Fixture.Avatar);
	TestEqual(TEXT("重复绑定不补发拾取"), Pickup->GetQuantity(), 7);
	Fixture.Reenter(Pickup);
	TestEqual(TEXT("重新进入才接收全部余量"), Pickup->GetQuantity(), 0);
	TestEqual(TEXT("接收和移出全程守恒"), Fixture.Total() + Outgoing, 207);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardPickupGoldTest, "Wizard.Pickup.ExactGoldAndReentry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardPickupGoldTest::RunTest(const FString& Parameters)
{
	WizardPickupTests::FFixture Fixture;
	if (!TestTrue(TEXT("真实拾取世界可用"), Fixture.IsReady())) return false;
	int32 Accepted = 0;
	Fixture.Inventory->AddItems(Fixture.Ore, 200, Accepted);
	const FWizardInventorySnapshot Before = Fixture.Inventory->GetSnapshot();
	AWizardWorldPickup* Gold = AWizardWorldPickup::SpawnGold(Fixture.World, nullptr, 25, Fixture.Avatar->GetActorLocation());
	if (!TestNotNull(TEXT("金币缺少图标仍可生成"), Gold)) return false;
	bool bGoldReentry = true;
	bool bPickupReentry = true;
	const FDelegateHandle Handle = Fixture.Player->OnGoldChanged.AddLambda([&]()
	{
		bGoldReentry = Fixture.Player->TryAddGold(1);
		bPickupReentry = Gold->TransferTo([](int32 Offered) { return Offered; });
	});
	Gold->ActivatePickup();
	Fixture.Player->OnGoldChanged.Remove(Handle);
	TestEqual(TEXT("满背包重叠接收25金币"), Fixture.Player->GetGold(), 25);
	TestFalse(TEXT("金币通知重入不能额外入账"), bGoldReentry);
	TestFalse(TEXT("金币接收回调重入不能复制"), bPickupReentry);
	TestTrue(TEXT("金币完全不改背包"), Fixture.Inventory->GetSnapshot().Slots == Before.Slots);
	TestTrue(TEXT("可精确到达整数余额上限"), Fixture.Player->TryAddGold(MAX_int32 - 25));
	AWizardWorldPickup* Overflow = AWizardWorldPickup::SpawnGold(Fixture.World, nullptr, 1, Fixture.Avatar->GetActorLocation());
	if (!TestNotNull(TEXT("生成不能入账的金币"), Overflow)) return false;
	Overflow->ActivatePickup();
	TestEqual(TEXT("入账溢出金币留在地面"), Overflow->GetQuantity(), 1);
	Fixture.Reenter(Overflow);
	TestEqual(TEXT("重复进入不改变溢出余额"), Fixture.Player->GetGold(), MAX_int32);
	TestFalse(TEXT("负数不能扣改金币"), Fixture.Player->TryAddGold(-1));
	TestFalse(TEXT("零金币无有效变更"), Fixture.Player->TryAddGold(0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardPickupDiscardTest, "Wizard.Pickup.DiscardCommitAndLeaveRange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardPickupDiscardTest::RunTest(const FString& Parameters)
{
	WizardPickupTests::FFixture Fixture;
	if (!TestTrue(TEXT("真实拾取世界可用"), Fixture.IsReady())) return false;
	int32 Accepted = 0;
	Fixture.Inventory->AddItems(Fixture.Ore, 8, Accepted);
	const FWizardInventoryRequest Request = Fixture.Inventory->MakeRequest(0);
	UWizardLootSettings* Settings = GetMutableDefault<UWizardLootSettings>();
	const auto GroundClass = Settings->PickupClass;
	Settings->PickupClass.Reset();
	TestEqual(TEXT("生成类缺失导致丢弃失败"), Fixture.Inventory->Discard(Request, 3), EWizardInventoryResult::DropUnavailable);
	Settings->PickupClass = GroundClass;
	TestEqual(TEXT("生成失败保留8个来源物品"), Fixture.Total(), 8);
	TestEqual(TEXT("无图标可靠生成后提交丢弃"), Fixture.Inventory->Discard(Request, 3), EWizardInventoryResult::Success);
	AWizardWorldPickup* Pickup = nullptr;
	for (TActorIterator<AWizardWorldPickup> It(Fixture.World); It; ++It) if (IsValid(*It)) Pickup = *It;
	if (!TestNotNull(TEXT("已提交的丢弃对象"), Pickup)) return false;
	TestEqual(TEXT("来源只扣3个"), Fixture.Total(), 5);
	TestEqual(TEXT("初始重叠不得立即捡回"), Pickup->GetQuantity(), 3);
	TestEqual(TEXT("旧请求不能重复丢弃"), Fixture.Inventory->Discard(Request, 3), EWizardInventoryResult::StaleRequest);
	Fixture.Reenter(Pickup);
	TestEqual(TEXT("离开重入后拾回全部物品"), Fixture.Total(), 8);
	TestEqual(TEXT("地面余量归零"), Pickup->GetQuantity(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardPickupDefinitionAndAvatarTest, "Wizard.Pickup.DefinitionsIconsAndAvatarLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardPickupDefinitionAndAvatarTest::RunTest(const FString& Parameters)
{
	WizardPickupTests::FFixture Fixture;
	if (!TestTrue(TEXT("真实拾取世界可用"), Fixture.IsReady())) return false;
	TestNotNull(TEXT("缺失图标得到统一占位纹理"), Fixture.Ore->GetIconTexture());
	UWizardDropConfig* Drops = NewObject<UWizardDropConfig>();
	Drops->MinGold = Drops->MaxGold = 1;
	Drops->ScatterRadius = 0.f;
	FWizardDropEntry Entry;
	Entry.Definition = Fixture.Ore;
	Entry.MinQuantity = Entry.MaxQuantity = 2;
	Drops->Items.Add(Entry);
	TestTrue(TEXT("无物品和金币图标仍为有效掉落表"), Drops->IsConfigurationValid());
	Drops->SpawnDrops(Fixture.World, Fixture.Avatar->GetActorLocation(), nullptr);
	TestEqual(TEXT("未登记的物品在初始重叠时进入背包"), Fixture.Total(), 2);
	TestEqual(TEXT("无图标金币正常入账"), Fixture.Player->GetGold(), 1);
	Fixture.Inventory->Discard(Fixture.Inventory->MakeRequest(0), 1);
	AWizardWorldPickup* Pickup = nullptr;
	for (TActorIterator<AWizardWorldPickup> It(Fixture.World); It; ++It) if (IsValid(*It) && It->GetQuantity() > 0) Pickup = *It;
	if (!TestNotNull(TEXT("主动丢弃的地面对象"), Pickup)) return false;
	ACharacter* NextAvatar = Fixture.SpawnAvatar(Pickup->GetActorLocation());
	if (!TestNotNull(TEXT("新角色"), NextAvatar)) return false;
	NextAvatar->SetPlayerState(Fixture.Player);
	Fixture.Player->InitializeAvatar(NextAvatar);
	Fixture.Reenter(Pickup);
	TestEqual(TEXT("旧角色出入不能驱动新角色拾取"), Pickup->GetQuantity(), 1);
	Fixture.Reenter(Pickup, NextAvatar);
	TestEqual(TEXT("新角色离开重入恢复拾取"), Fixture.Total(), 2);
	TestEqual(TEXT("换角色和缺失图标全程数量守恒"), Pickup->GetQuantity(), 0);
	return true;
}
#endif
