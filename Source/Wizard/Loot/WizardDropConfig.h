// Wizard 项目：每条配置独立判定概率，不为具体物品增加业务分支。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WizardDropConfig.generated.h"
class AWizardWorldPickup;
class UWizardItemDefinition;
class AWizardChallengeArena;

USTRUCT(BlueprintType)
struct FWizardDropEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UWizardItemDefinition> Definition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float Probability = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 MinQuantity = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 MaxQuantity = 1;
};

UCLASS(BlueprintType)
class WIZARD_API UWizardDropConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<class UTexture2D> GoldIcon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 MinGold = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) int32 MaxGold = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float ScatterRadius = 65.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FWizardDropEntry> Items;
	bool IsConfigurationValid() const;
	// 敌人负责死亡只调用一次；生成失败记录诊断，已生成掉落不重复重试。
	void SpawnDrops(UWorld* World, const FVector& Location, AWizardChallengeArena* Challenge) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
