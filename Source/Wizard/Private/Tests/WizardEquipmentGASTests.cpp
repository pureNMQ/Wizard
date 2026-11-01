// Wizard 项目：保护换装不免费治疗、无临时上限损失以及重建不重复加成的规格约束。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Equipment/WizardEquipmentComponent.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "WizardCharacter.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEquipmentGASTest, "Wizard.Equipment.GASBonusesAndHealthClamp", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardEquipmentGASTest::RunTest(const FString& Parameters)
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
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	UWizardInventoryComponent* Inventory = Player->GetInventory();
	auto DefineHat = [Inventory](FName Id, float MaxHealthBonus)
	{
		UWizardItemDefinition* Definition = NewObject<UWizardItemDefinition>(Inventory);
		Definition->ItemId = Id;
		Definition->Type = EWizardItemType::Equipment;
		Definition->MaxStack = 1;
		Definition->EquipmentSlot = EWizardEquipmentSlot::Hat;
		FWizardEquipmentBonus& Bonus = Definition->EquipmentBonuses.AddDefaulted_GetRef();
		Bonus.Attribute = UWizardAttributeSet::GetMaxHealthAttribute();
		Bonus.Magnitude = MaxHealthBonus;
		return Definition;
	};
	UWizardItemDefinition* First = DefineHat(TEXT("EquipmentHealthHat"), 50.f);
	UWizardItemDefinition* Larger = DefineHat(TEXT("EquipmentLargerHat"), 80.f);
	UWizardItemDefinition* Smaller = DefineHat(TEXT("EquipmentSmallerHat"), 20.f);
	int32 Accepted = 0;
	Inventory->AddItems(First, 1, Accepted);
	Inventory->AddItems(Larger, 1, Accepted);
	Inventory->AddItems(Smaller, 1, Accepted);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 80.f);
	TestEqual(TEXT("首次装备"), Inventory->Equip(Inventory->MakeRequest(0), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
	TestEqual(TEXT("上限增加"), Player->GetAttributes()->GetMaxHealth(), 150.f);
	TestEqual(TEXT("上限增加不治疗"), Player->GetAttributes()->GetHealth(), 80.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 140.f);
	TestEqual(TEXT("替换更高上限"), Inventory->Equip(Inventory->MakeRequest(1), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
	TestEqual(TEXT("新上限替代旧加成"), Player->GetAttributes()->GetMaxHealth(), 180.f);
	TestEqual(TEXT("替换无中间卸装损失"), Player->GetAttributes()->GetHealth(), 140.f);
	Player->InitializeAvatar(Avatar);
	Player->GetEquipment()->RefreshEquipment();
	TestEqual(TEXT("重复初始化和重建不叠加"), Player->GetAttributes()->GetMaxHealth(), 180.f);
	TestEqual(TEXT("替换较低上限"), Inventory->Equip(Inventory->MakeRequest(2), EWizardEquipmentSlot::Hat), EWizardInventoryResult::Success);
	TestEqual(TEXT("降低后只夹到最终上限"), Player->GetAttributes()->GetHealth(), 120.f);
	TestEqual(TEXT("卸装"), Inventory->Unequip(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Hat)), EWizardInventoryResult::Success);
	TestEqual(TEXT("卸装恢复基础上限"), Player->GetAttributes()->GetMaxHealth(), 100.f);
	TestEqual(TEXT("卸装生命夹取"), Player->GetAttributes()->GetHealth(), 100.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 60.f);
	Inventory->Equip(Inventory->MakeRequest(0), EWizardEquipmentSlot::Hat);
	Inventory->Unequip(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Hat));
	TestEqual(TEXT("反复穿脱不能免费治疗"), Player->GetAttributes()->GetHealth(), 60.f);
	// 负数也是数据允许的修正；替换时先添加/先移除都会存在临时错误上限的风险。
	UWizardItemDefinition* Negative = DefineHat(TEXT("EquipmentNegativeHat"), -20.f);
	UWizardItemDefinition* LessNegative = DefineHat(TEXT("EquipmentLessNegativeHat"), -10.f);
	Inventory->AddItems(Negative, 1, Accepted);
	Inventory->AddItems(LessNegative, 1, Accepted);
	const auto Find = [Inventory](const UWizardItemDefinition* Definition)
	{
		return Inventory->GetSnapshot().Slots.IndexOfByPredicate([Definition](const FWizardInventoryEntry& Entry) { return Entry.Definition == Definition; });
	};
	Inventory->Equip(Inventory->MakeRequest(Find(Negative)), EWizardEquipmentSlot::Hat);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 75.f);
	Inventory->Equip(Inventory->MakeRequest(Find(LessNegative)), EWizardEquipmentSlot::Hat);
	TestEqual(TEXT("负加成替换保留合法当前生命"), Player->GetAttributes()->GetHealth(), 75.f);
	TestEqual(TEXT("负加成精确替换"), Player->GetAttributes()->GetMaxHealth(), 90.f);
	const FWizardInventorySnapshot Saved = Inventory->GetSnapshot();
	Inventory->Unequip(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Hat));
	TestEqual(TEXT("装备快照恢复"), Inventory->RestoreSnapshot(Saved), EWizardInventoryResult::Success);
	AWizardCharacter* Replacement = World->SpawnActor<AWizardCharacter>();
	Player->InitializeAvatar(Replacement);
	TestEqual(TEXT("恢复及角色重建不叠加"), Player->GetAttributes()->GetMaxHealth(), 90.f);
	Inventory->Unequip(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Hat));
	UWizardItemDefinition* ResourceBoots = NewObject<UWizardItemDefinition>(Inventory);
	ResourceBoots->ItemId = TEXT("EquipmentResourceBoots");
	ResourceBoots->Type = EWizardItemType::Equipment;
	ResourceBoots->MaxStack = 1;
	ResourceBoots->EquipmentSlot = EWizardEquipmentSlot::Boots;
	auto AddBonus = [ResourceBoots](FGameplayAttribute Attribute, float Value)
	{
		FWizardEquipmentBonus& Bonus = ResourceBoots->EquipmentBonuses.AddDefaulted_GetRef();
		Bonus.Attribute = Attribute;
		Bonus.Magnitude = Value;
	};
	AddBonus(UWizardAttributeSet::GetMaxHealthAttribute(), 50.f);
	AddBonus(UWizardAttributeSet::GetMaxManaAttribute(), 30.f);
	AddBonus(UWizardAttributeSet::GetAttackPowerAttribute(), 3.f);
	AddBonus(UWizardAttributeSet::GetDefenseAttribute(), 4.f);
	AddBonus(UWizardAttributeSet::GetMoveSpeedAttribute(), 25.f);
	Inventory->AddItems(ResourceBoots, 1, Accepted);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 50.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 50.f);
	Inventory->Equip(Inventory->MakeRequest(Find(ResourceBoots)), EWizardEquipmentSlot::Boots);
	TestEqual(TEXT("上限装备不额外治疗"), Player->GetAttributes()->GetHealth(), 50.f);
	TestEqual(TEXT("上限装备不额外恢复法力"), Player->GetAttributes()->GetMana(), 50.f);
	TestEqual(TEXT("混合配置提高生命上限"), Player->GetAttributes()->GetMaxHealth(), 150.f);
	TestEqual(TEXT("混合配置提高法力上限"), Player->GetAttributes()->GetMaxMana(), 130.f);
	TestEqual(TEXT("装备移动速度应用到新角色"), Player->GetAttributes()->GetMoveSpeed(), 625.f);
	TestEqual(TEXT("角色移动组件同步装备移速"), Replacement->GetCharacterMovement()->MaxWalkSpeed, 625.f);
	TestEqual(TEXT("攻击加成"), Player->GetAttributes()->GetAttackPower(), 28.f);
	TestEqual(TEXT("防御加成"), Player->GetAttributes()->GetDefense(), 4.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 140.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 120.f);
	Inventory->Equip(Inventory->MakeRequest(Find(Negative)), EWizardEquipmentSlot::Hat);
	TestEqual(TEXT("多个装备共同决定最终生命上限"), Player->GetAttributes()->GetMaxHealth(), 130.f);
	TestEqual(TEXT("其他槽位降上限时只夹到最终上限"), Player->GetAttributes()->GetHealth(), 130.f);
	Inventory->Unequip(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Boots));
	TestEqual(TEXT("混合装备移除后生命夹取"), Player->GetAttributes()->GetHealth(), 80.f);
	TestEqual(TEXT("混合装备移除后法力夹取"), Player->GetAttributes()->GetMana(), 100.f);
	TestEqual(TEXT("移动加成精确移除"), Player->GetAttributes()->GetMoveSpeed(), 600.f);
	TestEqual(TEXT("角色移动组件同步卸装移速"), Replacement->GetCharacterMovement()->MaxWalkSpeed, 600.f);
	TestEqual(TEXT("攻击加成精确移除"), Player->GetAttributes()->GetAttackPower(), 25.f);
	TestEqual(TEXT("防御加成精确移除"), Player->GetAttributes()->GetDefense(), 0.f);
	Inventory->Unequip(Inventory->MakeEquipmentRequest(EWizardEquipmentSlot::Hat));
	TestEqual(TEXT("上限回升不免费治疗"), Player->GetAttributes()->GetHealth(), 80.f);
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif
