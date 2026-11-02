// Wizard 项目：保护真实落盘往返、失败不覆盖、损坏恢复与跨域恢复原子性。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Save/WizardSaveComponent.h"
#include "Save/WizardSaveGame.h"
#include "WizardPlayerState.h"
#include "WizardCharacter.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "Trading/WizardShopCatalog.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace WizardSaveTests
{
	struct FSlot
	{
		FString Name = TEXT("WizardTest-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		FString Path = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), Name + TEXT(".sav"));
		~FSlot()
		{
			// 仅清理本用例随机槽名，不能触碰正式玩家进度。
			TArray<FString> Files;
			IFileManager::Get().FindFiles(Files, *(Path + TEXT("*")), true, false);
			for (const FString& File : Files) IFileManager::Get().Delete(*FPaths::Combine(FPaths::GetPath(Path), File));
		}
	};
	struct FFixture
	{
		UWorld* World;
		AWizardPlayerState* Player;
		AWizardCharacter* Avatar;
		FFixture()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			Player = World->SpawnActor<AWizardPlayerState>();
			Avatar = World->SpawnActor<AWizardCharacter>();
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
			Player->InitializeAvatar(Avatar);
		}
		~FFixture() { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
	};
	UWizardSaveGame* Snapshot(AWizardPlayerState* Player)
	{
		UWizardSaveGame* Save = NewObject<UWizardSaveGame>();
		Save->Gold = Player->GetGold();
		Save->Inventory = Player->GetInventory()->GetSnapshot();
		Save->Inventory.Revision = 0;
		Save->Loadout = Player->GetSkills()->GetSnapshot();
		return Save;
	}
	void Corrupt(const FString& Path)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *Path);
		if (!Bytes.IsEmpty()) Bytes.Last() ^= 0xFF;
		FFileHelper::SaveArrayToFile(Bytes, *Path);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSaveRoundTripTest, "Wizard.Save.TransactionsAndDiskRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSaveRoundTripTest::RunTest(const FString& Parameters)
{
	using namespace WizardSaveTests;
	FSlot Slot;
	FWizardInventorySnapshot Expected;
	float ExpectedMaxHealth = 0.f;
	int32 ExpectedGold = 0;
	{
		FFixture Source;
		AWizardPlayerState* Player = Source.Player;
		UWizardSaveComponent* Service = Player->GetSaveService();
		TestTrue(TEXT("无存档可开始新会话"), Service->StartSession(Slot.Name));
		TestFalse(TEXT("初始无变化不写文件"), IFileManager::Get().FileExists(*Slot.Path));
		Player->TryAddGold(1000);
		UWizardItemDefinition* Potion = LoadObject<UWizardItemDefinition>(nullptr, TEXT("/Game/Wizard/Data/Items/DA_HealthPotion.DA_HealthPotion"));
		UWizardItemDefinition* Hat = LoadObject<UWizardItemDefinition>(nullptr, TEXT("/Game/Wizard/Data/Items/DA_SageHat.DA_SageHat"));
		if (!TestNotNull(TEXT("正式药水定义"), Potion) || !TestNotNull(TEXT("正式装备定义"), Hat)) return false;
		UWizardShopCatalog* Shop = NewObject<UWizardShopCatalog>();
		FWizardShopProduct Product;
		Product.Definition = Potion;
		Product.BuyPrice = 5;
		Shop->Products.Add(Product);
		UWizardInventoryComponent* Items = Player->GetInventory();
		TestEqual(TEXT("真实购买事务"), Items->Buy(Shop, 0, 4, Items->GetSnapshot().Revision), EWizardInventoryResult::Success);
		TestEqual(TEXT("拆分"), Items->Split(Items->MakeRequest(0), 1, 1), EWizardInventoryResult::Success);
		TestEqual(TEXT("出售"), Items->Sell(Items->MakeRequest(1), 1), EWizardInventoryResult::Success);
		int32 Accepted = 0;
		Items->AddItems(Hat, 1, Accepted);
		TestEqual(TEXT("换装"), Items->Equip(Items->MakeRequest(1), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
		FWizardLoadoutSnapshot Empty;
		Empty.Slots.SetNum(4);
		Player->GetSkills()->RestoreSnapshot(Empty);
		Expected = Items->GetSnapshot();
		ExpectedGold = Player->GetGold();
		ExpectedMaxHealth = Player->GetAttributes()->GetMaxHealth();
		TArray<uint8> Before, After, BackupBefore, BackupAfter;
		WizardSaveStorage::Read(Slot.Path, Before);
		WizardSaveStorage::Read(Slot.Path + TEXT(".bak"), BackupBefore);
		UWizardSaveGame* Disk = WizardSaveStorage::Decode(Before);
		if (!TestNotNull(TEXT("事务返回时已成功落盘"), Disk)) return false;
		TestEqual(TEXT("余额与物品为同笔交易的最终值"), Disk->Gold, ExpectedGold);
		TestEqual(TEXT("落盘药水数量"), Disk->Inventory.Slots[0].Quantity, 3);
		TestNull(TEXT("磁盘不保存运行时定义引用"), Disk->Inventory.Slots[0].Definition.Get());
		TestEqual(TEXT("失败数量请求"), Items->Split(Items->MakeRequest(0), 99, 2), EWizardInventoryResult::InvalidQuantity);
		Player->InitializeAvatar(Source.Avatar);
		WizardSaveStorage::Read(Slot.Path, After);
		WizardSaveStorage::Read(Slot.Path + TEXT(".bak"), BackupAfter);
		TestTrue(TEXT("失败操作与重复初始化不改变落盘快照"), Before == After);
		TestTrue(TEXT("无有效变化不会挤掉上一成功备份"), BackupBefore == BackupAfter);
		TestEqual(TEXT("重复初始化不覆盖空技能槽"), Player->GetSkills()->GetSnapshot().Slots, Empty.Slots);
	}
	{
		FFixture Restored;
		TestTrue(TEXT("新PlayerState从磁盘恢复"), Restored.Player->GetSaveService()->StartSession(Slot.Name));
		const auto Actual = Restored.Player->GetInventory()->GetSnapshot();
		TestEqual(TEXT("恢复金币"), Restored.Player->GetGold(), ExpectedGold);
		TestTrue(TEXT("数量、位置、身份、定义及装备准确恢复"), Actual.Slots == Expected.Slots && Actual.Equipment == Expected.Equipment);
		TestEqual(TEXT("装备加成只重建一次"), Restored.Player->GetAttributes()->GetMaxHealth(), ExpectedMaxHealth);
		TestEqual(TEXT("生命恢复到装备后的上限"), Restored.Player->GetAttributes()->GetHealth(), ExpectedMaxHealth);
		TestEqual(TEXT("法力恢复"), Restored.Player->GetAttributes()->GetMana(), Restored.Player->GetAttributes()->GetMaxMana());
		for (FName Id : Restored.Player->GetSkills()->GetSnapshot().Slots) TestTrue(TEXT("全空技能搭配恢复"), Id.IsNone());
		Restored.Player->InitializeAvatar(Restored.Avatar);
		TestEqual(TEXT("恢复后重复初始化不叠加装备"), Restored.Player->GetAttributes()->GetMaxHealth(), ExpectedMaxHealth);
		FWizardLoadoutSnapshot Mixed = Restored.Player->GetSkills()->Catalog->InitialLoadout;
		Mixed.Slots.Swap(0, 1);
		Mixed.Slots[3] = NAME_None;
		Restored.Player->GetSkills()->RestoreSnapshot(Mixed);
		TArray<uint8> Bytes;
		WizardSaveStorage::Read(Slot.Path, Bytes);
		UWizardSaveGame* Disk = WizardSaveStorage::Decode(Bytes);
		if (!TestNotNull(TEXT("混合技能快照可读"), Disk)) return false;
		TestTrue(TEXT("技能ID与空槽的位置准确落盘"), Disk->Loadout.Slots == Mixed.Slots);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSaveAtomicRestoreTest, "Wizard.Save.InvalidSnapshotIsAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSaveAtomicRestoreTest::RunTest(const FString& Parameters)
{
	using namespace WizardSaveTests;
	FFixture Fixture;
	Fixture.Player->TryAddGold(17);
	UWizardSaveGame* Save = Snapshot(Fixture.Player);
	auto Reject = [&](const FWizardInventorySnapshot& Items, const FWizardLoadoutSnapshot& Skills, int32 Gold)
	{
		TestFalse(TEXT("非法快照拒绝"), Fixture.Player->RestorePersistentState(Gold, Items, Skills));
		TestEqual(TEXT("拒绝后余额不变"), Fixture.Player->GetGold(), 17);
		TestTrue(TEXT("拒绝后物品不变"), Fixture.Player->GetInventory()->GetSnapshot().Slots == Save->Inventory.Slots);
		TestTrue(TEXT("拒绝后技能不变"), Fixture.Player->GetSkills()->GetSnapshot().Slots == Save->Loadout.Slots);
	};
	Reject(Save->Inventory, Save->Loadout, -1);
	FWizardLoadoutSnapshot BadSkills = Save->Loadout;
	BadSkills.Slots[0] = TEXT("MissingSkill");
	Reject(Save->Inventory, BadSkills, 99);
	FWizardInventorySnapshot BadItems = Save->Inventory;
	BadItems.Slots[0].DefinitionId = TEXT("MissingItem");
	BadItems.Slots[0].InstanceId = FGuid::NewGuid();
	BadItems.Slots[0].Quantity = 1;
	Reject(BadItems, Save->Loadout, 99);
	UWizardItemDefinition* Potion = LoadObject<UWizardItemDefinition>(nullptr, TEXT("/Game/Wizard/Data/Items/DA_HealthPotion.DA_HealthPotion"));
	if (!TestNotNull(TEXT("物品定义"), Potion)) return false;
	BadItems.Slots[0].Definition = Potion;
	BadItems.Slots[0].DefinitionId = Potion->ItemId;
	BadItems.Slots[0].Quantity = -1;
	Reject(BadItems, Save->Loadout, 99);
	BadItems.Slots[0].Quantity = 1;
	BadItems.Slots[1] = BadItems.Slots[0];
	Reject(BadItems, Save->Loadout, 99);
	bool bObserved = false;
	const FDelegateHandle Handle = Fixture.Player->OnGoldChanged.AddLambda([&]()
	{
		bObserved = true;
		TestTrue(TEXT("通知时技能已一起提交"), Fixture.Player->GetSkills()->GetSnapshot().Slots == Save->Loadout.Slots);
		TestFalse(TEXT("通知不能重入修改金币"), Fixture.Player->TryAddGold(1));
		TestEqual(TEXT("通知不能重入修改技能"), Fixture.Player->GetSkills()->RestoreSnapshot(Save->Loadout), EWizardSkillResult::Busy);
	});
	TestTrue(TEXT("合法快照整体恢复"), Fixture.Player->RestorePersistentState(22, Save->Inventory, Save->Loadout));
	Fixture.Player->OnGoldChanged.Remove(Handle);
	TestTrue(TEXT("成功广播最终状态"), bObserved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSaveFailureTest, "Wizard.Save.WriteFailureAndRetry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSaveFailureTest::RunTest(const FString& Parameters)
{
	using namespace WizardSaveTests;
	FSlot Slot;
	FFixture Fixture;
	UWizardSaveComponent* Service = Fixture.Player->GetSaveService();
	Service->StartSession(Slot.Name);
	Fixture.Player->TryAddGold(10);
	TArray<uint8> Before, After;
	WizardSaveStorage::Read(Slot.Path, Before);
	// 以同名目录阻止临时文件创建，模拟真实文件系统写失败。
	IFileManager::Get().MakeDirectory(*(Slot.Path + TEXT(".tmp")));
	Fixture.Player->TryAddGold(5);
	TestEqual(TEXT("失败不伪报成功"), Service->GetStatus(), EWizardSaveStatus::WriteFailed);
	TestFalse(TEXT("失败提供实际反馈"), Service->GetNotice().IsEmpty());
	WizardSaveStorage::Read(Slot.Path, After);
	TestTrue(TEXT("写失败保留旧主档"), Before == After);
	IFileManager::Get().DeleteDirectory(*(Slot.Path + TEXT(".tmp")), false, false);
	TestTrue(TEXT("解除阻碍后重试"), Service->SaveNow());
	WizardSaveStorage::Read(Slot.Path, After);
	UWizardSaveGame* Disk = WizardSaveStorage::Decode(After);
	if (!TestNotNull(TEXT("重试存档可读"), Disk)) return false;
	TestEqual(TEXT("重试保存最新余额"), Disk->Gold, 15);
	TestEqual(TEXT("成功状态"), Service->GetStatus(), EWizardSaveStatus::Saved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSaveRecoveryTest, "Wizard.Save.CorruptionAndBackupRecovery", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSaveRecoveryTest::RunTest(const FString& Parameters)
{
	using namespace WizardSaveTests;
	FSlot Slot;
	{
		FFixture Source;
		Source.Player->GetSaveService()->StartSession(Slot.Name);
		Source.Player->TryAddGold(10);
		Source.Player->TryAddGold(20);
	}
	Corrupt(Slot.Path);
	{
		FFixture Restored;
		TestTrue(TEXT("主档损坏可以恢复备份"), Restored.Player->GetSaveService()->StartSession(Slot.Name));
		TestEqual(TEXT("恢复上一成功余额"), Restored.Player->GetGold(), 10);
		TestEqual(TEXT("明确报告使用备份"), Restored.Player->GetSaveService()->GetStatus(), EWizardSaveStatus::RecoveredBackup);
		TArray<FString> Rejected;
		IFileManager::Get().FindFiles(Rejected, *(Slot.Path + TEXT(".rejected-*")), true, false);
		TestEqual(TEXT("损坏原件保留诊断依据"), Rejected.Num(), 1);
		Restored.Player->TryAddGold(1);
	}
	{
		FFixture Again;
		TestTrue(TEXT("恢复后新进度可继续读档"), Again.Player->GetSaveService()->StartSession(Slot.Name));
		TestEqual(TEXT("新进度未被旧备份覆盖"), Again.Player->GetGold(), 11);
	}
	IFileManager::Get().Delete(*Slot.Path);
	{
		FFixture MissingPrimary;
		TestTrue(TEXT("替换文件期间中断留下备份仍可恢复"), MissingPrimary.Player->GetSaveService()->StartSession(Slot.Name));
		TestEqual(TEXT("主档缺失时恢复最后备份"), MissingPrimary.Player->GetGold(), 10);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSaveBlockedTest, "Wizard.Save.InvalidFilesArePreserved", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSaveBlockedTest::RunTest(const FString& Parameters)
{
	using namespace WizardSaveTests;
	FFixture Source;
	for (int32 Case = 0; Case < 4; ++Case)
	{
		FSlot Slot;
		UWizardSaveGame* Save = Snapshot(Source.Player);
		if (Case == 0) Save->SchemaVersion = 999;
		if (Case == 1) Save->Inventory.Slots[0].Quantity = -1;
		TArray<uint8> Original;
		WizardSaveStorage::Encode(Save, Original);
		if (Case == 2) Original.SetNum(Original.Num() / 2);
		if (Case == 3) Original.Last() ^= 0xFF;
		WizardSaveStorage::Write(Slot.Path, Original);
		{
			FFixture Target;
			TestFalse(TEXT("不兼容/非法/截断/损坏文件阻塞自动覆盖"), Target.Player->GetSaveService()->StartSession(Slot.Name));
			TestEqual(TEXT("明确区分加载失败与新玩家"), Target.Player->GetSaveService()->GetStatus(), EWizardSaveStatus::LoadBlocked);
			Target.Player->TryAddGold(100);
			TestFalse(TEXT("不能覆盖不可恢复的原件"), Target.Player->GetSaveService()->SaveNow());
		}
		TArray<uint8> After;
		FFileHelper::LoadFileToArray(After, *Slot.Path);
		TestTrue(TEXT("原文件逐字节保持"), Original == After);
	}
	return true;
}
#endif
