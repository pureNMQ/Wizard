#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "Skills/WizardSkillDefinition.h"
#include "WizardSkillLoadoutComponent.generated.h"
class AWizardPlayerState;
struct FActiveGameplayEffect;
struct FOnAttributeChangeData;
DECLARE_MULTICAST_DELEGATE(FWizardLoadoutChanged);

UCLASS(ClassGroup=(Wizard), meta=(BlueprintSpawnableComponent))
class WIZARD_API UWizardSkillLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardSkillLoadoutComponent();
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UWizardSkillCatalog> Catalog;
	void InitializeAvatar();
	UFUNCTION(BlueprintCallable) EWizardSkillResult ActivateSlot(int32 Slot);
	UFUNCTION(BlueprintCallable) EWizardSkillResult EquipSkill(FName Id, int32 Slot);
	// 仅在有效Alchemy交互范围内清空槽位；保留已授予能力和技能冷却。
	UFUNCTION(BlueprintCallable) EWizardSkillResult UnequipSkill(int32 Slot);
	UFUNCTION(BlueprintCallable) EWizardSkillResult SwapSlots(int32 Source, int32 Target);
	UFUNCTION(BlueprintPure) FWizardLoadoutSnapshot GetSnapshot() const { return Loadout; }
	// 整体校验后原子恢复，不广播未发生的变更；供T08读档使用，不属于UI编辑入口。
	EWizardSkillResult RestoreSnapshot(const FWizardLoadoutSnapshot& Snapshot);
	const UWizardSkillDefinition* GetSlot(int32 Slot) const;
	const UWizardSkillDefinition* Find(FName Id) const;
	float GetCooldownRemaining(const UWizardSkillDefinition* Definition) const;
	bool CanEditLoadout() const;
	EWizardSkillResult CheckSkill(const UWizardSkillDefinition* Definition, const FVector& Aim, FVector& OutTarget, bool bCheckAvailability = true) const;
	void CancelCasting();
	void ClearAvatarEffects();
	void ApplyShield(float Amount, float Duration, const UWizardSkillDefinition* Definition = nullptr);
	FWizardLoadoutChanged OnLoadoutChanged;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	AWizardPlayerState* GetState() const;
	void OnEffectRemoved(const FActiveGameplayEffect& Effect);
	void OnShieldChanged(const FOnAttributeChangeData& Data);
	UPROPERTY() FWizardLoadoutSnapshot Loadout;
	TMap<FName, FGameplayAbilitySpecHandle> Handles;
	FActiveGameplayEffectHandle ShieldHandle;
	FDelegateHandle EffectRemovedHandle, ShieldChangedHandle;
};
