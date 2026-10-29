#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WizardPlayerController.generated.h"
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class UUserWidget;
class UWizardPauseMenu;
class UWizardUIRoot;
UCLASS()
class AWizardPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AWizardPlayerController();
	virtual void PlayerTick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	// CommonUI 激活/返回统一调用，恢复时必须重新按下攻击。
	void SetMenuOpen(bool bOpen);
	void SetRootScreensOpen(bool bOpen);
	UWizardUIRoot* GetUIRoot() const { return UIRoot; }
	UFUNCTION(BlueprintPure) bool IsMenuOpen() const { return bMenuOpen; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void SetupInputComponent() override;
	UPROPERTY(EditDefaultsOnly, Category=Input) TObjectPtr<UInputMappingContext> CombatMappingContext;
	UPROPERTY(EditDefaultsOnly, Category=Input) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(EditDefaultsOnly, Category=Input) TObjectPtr<UInputAction> AttackAction;
	UPROPERTY(EditDefaultsOnly, Category=Input) TObjectPtr<UInputAction> MenuAction;
	UPROPERTY(EditDefaultsOnly, Category=UI) TSubclassOf<UWizardUIRoot> RootClass;
	UPROPERTY(EditDefaultsOnly, Category=UI) TSubclassOf<UUserWidget> HealthWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category=UI) TSubclassOf<UWizardPauseMenu> PauseMenuClass;
private:
	void Move(const FInputActionValue& Value);
	void StartAttack();
	void ReleaseAttack();
	void StopAttack();
	void OpenMenu();
	void UpdateAim();
	bool CanUseWorldInput() const;
	void RefreshMenuState();
	bool bLegacyMenuOpen = false;
	bool bRootScreensOpen = false;
	bool bMenuOpen = false;
	bool bWasPaused = false;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> HealthWidget;
	UPROPERTY(Transient) TObjectPtr<UWizardUIRoot> UIRoot;
};
