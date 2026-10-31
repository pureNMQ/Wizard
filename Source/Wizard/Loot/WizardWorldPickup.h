// Wizard 项目：地面对象独占剩余数量，可靠交接后才允许自动拾取。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "WizardWorldPickup.generated.h"

class UWizardItemDefinition;
class AWizardChallengeArena;
class USphereComponent;
class UWidgetComponent;
class UTexture2D;

UCLASS(Abstract, Blueprintable)
class WIZARD_API UWizardPickupIcon : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetIcon(UTexture2D* Texture);
protected:
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<class UImage> IconImage;
};

UCLASS(Blueprintable)
class WIZARD_API AWizardWorldPickup : public AActor
{
	GENERATED_BODY()
public:
	AWizardWorldPickup();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup", meta=(ClampMin="1")) float PickupRadius = 160.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup") FVector2D IconSize = FVector2D(40.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup") TSubclassOf<UWizardPickupIcon> IconWidgetClass;
	// 返回的对象尚不接收拾取；调用者可靠提交来源后再激活，失败须销毁。
	static AWizardWorldPickup* SpawnItem(UWorld* World, const UWizardItemDefinition* Definition, int32 Quantity, const FVector& Location,
		AWizardChallengeArena* Challenge = nullptr);
	static AWizardWorldPickup* SpawnGold(UWorld* World, UTexture2D* Icon, int32 Quantity, const FVector& Location, AWizardChallengeArena* Challenge = nullptr);
	void ActivatePickup();
	bool HasValidConfiguration() const;
	// 接收者同步返回实际接收量；数量归本对象提交，回调重入被拒绝。
	bool TransferTo(TFunctionRef<int32(int32)> Receiver);
	UFUNCTION(BlueprintPure) int32 GetQuantity() const { return RemainingQuantity; }
	UFUNCTION(BlueprintPure) const UWizardItemDefinition* GetDefinition() const { return ItemDefinition; }
	UFUNCTION(BlueprintPure) bool IsGold() const { return bGold; }
	bool BelongsToChallenge(const AWizardChallengeArena* Challenge) const;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	static AWizardWorldPickup* SpawnPrepared(UWorld* World, const UWizardItemDefinition* Definition, UTexture2D* Icon, bool bIsGold, int32 Quantity,
		const FVector& Location, AWizardChallengeArena* Challenge);
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> PickupSphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> IconComponent;
	UPROPERTY(Transient) TObjectPtr<const UWizardItemDefinition> ItemDefinition;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> IconTexture;
	UPROPERTY(Transient) TWeakObjectPtr<AWizardChallengeArena> SourceChallenge;
	int32 RemainingQuantity = 0;
	bool bGold = false;
	bool bActive = false;
	bool bCollecting = false;
};
