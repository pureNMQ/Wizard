// Wizard 项目：装备状态属于物品模型，本组件仅将其适配为可移除的 GAS 效果与角色表现。
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "WizardEquipmentComponent.generated.h"

class UWizardInventoryComponent;
class UWizardItemDefinition;
class AWizardCharacter;

UCLASS(ClassGroup=(Wizard))
class WIZARD_API UWizardEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardEquipmentComponent();
	// 重复初始化不叠加；更换 Avatar 只迁移表现，PlayerState 上的有效加成继续保留。
	void InitializeAvatar(APawn* Avatar);
	// 读档及重建共用入口；按定义差异修复效果，不通过全部卸装造成临时上限下降。
	UFUNCTION(BlueprintCallable, Category="Equipment") void RefreshEquipment();
	void ClearAvatar(AWizardCharacter* ExpectedAvatar = nullptr);
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void RefreshPresentation();
	TWeakObjectPtr<UWizardInventoryComponent> Inventory;
	TWeakObjectPtr<AWizardCharacter> Character;
	FDelegateHandle InventoryHandle;
	TArray<FActiveGameplayEffectHandle> EffectHandles;
	UPROPERTY(Transient) TArray<TObjectPtr<const UWizardItemDefinition>> AppliedDefinitions;
	bool bRefreshing = false;
};
