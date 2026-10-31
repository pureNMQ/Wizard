#include "Loot/WizardDropConfig.h"
#include "Loot/WizardWorldPickup.h"
#include "Loot/WizardLootSettings.h"
#include "Inventory/WizardItemDefinition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UWizardDropConfig::IsConfigurationValid() const
{
	if (MinGold < 0 || MaxGold < MinGold
		|| !FMath::IsFinite(ScatterRadius) || ScatterRadius < 0.f) return false;
	for (const FWizardDropEntry& Entry : Items)
	{
		if (!Entry.Definition || !Entry.Definition->IsConfigurationValid()
			|| !FMath::IsFinite(Entry.Probability) || Entry.Probability < 0.f || Entry.Probability > 1.f
			|| Entry.MinQuantity <= 0 || Entry.MaxQuantity < Entry.MinQuantity) return false;
	}
	return true;
}

void UWizardDropConfig::SpawnDrops(UWorld* World, const FVector& Location, AWizardChallengeArena* Challenge) const
{
	if (!World || !IsConfigurationValid())
	{
		UE_LOG(LogTemp, Error, TEXT("敌人掉落配置无效：%s"), *GetPathName());
		return;
	}
	const auto Scatter = [this, &Location]()
	{
		const float Angle = FMath::FRand() * 2.f * PI;
		const float Radius = FMath::Sqrt(FMath::FRand()) * ScatterRadius;
		return Location + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
	};
	const auto Activate = [this](AWizardWorldPickup* Pickup)
	{
		if (Pickup) Pickup->ActivatePickup();
		else UE_LOG(LogTemp, Error, TEXT("掉落生成失败（拾取类或世界状态无效）：%s"), *GetPathName());
	};
	const int32 Gold = static_cast<int32>(FMath::RandRange(static_cast<int64>(MinGold), static_cast<int64>(MaxGold)));
	if (Gold > 0) Activate(AWizardWorldPickup::SpawnGold(World, UWizardLootSettings::ResolveIcon(GoldIcon), Gold, Scatter(), Challenge));
	for (const FWizardDropEntry& Entry : Items)
	{
		if (Entry.Probability <= 0.f || (Entry.Probability < 1.f && FMath::FRand() >= Entry.Probability)) continue;
		const int32 Quantity = static_cast<int32>(FMath::RandRange(static_cast<int64>(Entry.MinQuantity), static_cast<int64>(Entry.MaxQuantity)));
		Activate(AWizardWorldPickup::SpawnItem(World, Entry.Definition, Quantity, Scatter(), Challenge));
	}
}

#if WITH_EDITOR
EDataValidationResult UWizardDropConfig::IsDataValid(FDataValidationContext& Context) const
{
	if (!IsConfigurationValid())
	{
		Context.AddError(FText::FromString(TEXT("掉落配置需要合法数量范围及概率，物品业务定义必须有效；图标缺失使用占位表现。")));
		return EDataValidationResult::Invalid;
	}
	return Super::IsDataValid(Context) == EDataValidationResult::Invalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
