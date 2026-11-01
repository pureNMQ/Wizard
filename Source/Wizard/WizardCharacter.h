// 保留模板来源，战斗扩展由 Wizard 项目实现。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "WizardCharacter.generated.h"

UCLASS(Blueprintable)
class AWizardCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AWizardCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	// 复用角色蓝图的握持组件，保留其 Socket 与相对变换；空装备隐藏法杖。
	void SetEquipmentStaff(class UStaticMesh* InMesh);
	UPROPERTY(EditDefaultsOnly, Category="Equipment") FName StaffComponentName = TEXT("MagicStaff");
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	TObjectPtr<class UCameraComponent> TopDownCameraComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera, meta=(AllowPrivateAccess="true"))
	TObjectPtr<class USpringArmComponent> CameraBoom;
};
