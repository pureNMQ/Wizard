// Wizard 项目：商店双栏视图；布局、图标与样式由Widget Blueprint配置。
#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
#include "UI/WizardShopMenuController.h"
#include "WizardShopWidgets.generated.h"
class AWizardShopActor;
class AWizardPlayerState;
class UWizardInventoryTile;
class UWizardInventoryPopover;
class UWizardInventoryQuantityModal;
class UWizardShopMenu;
class UVerticalBox;
class UUniformGridPanel;
class UButton;
class UImage;
class UTextBlock;
class UBorder;
UCLASS()
class WIZARD_API UWizardShopProductRow : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void Configure(UWizardShopMenu* Menu, int32 ProductIndex, const UWizardItemDefinition* Definition, int32 Price);
	void SetSelected(int32 ProductIndex);
	void FocusSelection();
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> Icon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> PriceText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SelectButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UBorder> SelectionBorder;
private:
	UFUNCTION() void Select();
	TWeakObjectPtr<UWizardShopMenu> OwnerMenu;
	int32 Index = INDEX_NONE;
};
UCLASS()
class WIZARD_API UWizardShopMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UWizardShopMenu();
	void Configure(AWizardShopActor* Shop);
	void SelectProduct(int32 Index);
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardShopProductRow> ProductRowClass;
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardInventoryTile> TileClass;
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardInventoryQuantityModal> QuantityModalClass;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UVerticalBox> ProductsBox;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UUniformGridPanel> InventoryGrid;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ProductDetails;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> GoldText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Feedback;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWizardInventoryPopover> DetailsPopover;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BuyButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CloseButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> AllButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> EquipmentButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ConsumablesButton;
private:
	UFUNCTION() void Buy();
	UFUNCTION() void CloseMenu();
	UFUNCTION() void ShowAll();
	UFUNCTION() void ShowEquipment();
	UFUNCTION() void ShowConsumables();
	void Sell();
	void Refresh();
	void RefreshProducts();
	void FocusProducts();
	void RefreshGold();
	void ShowFeedback(EWizardInventoryResult Result);
	void Inspect(int32 Index, UWizardInventoryTile* Source, bool bKeyboard);
	void HideDetails();
	void OpenQuantity(const FWizardShopQuantityRequest& Request, int32 Maximum);
	void CloseSession();
	UPROPERTY(Transient) TObjectPtr<UWizardShopMenuController> MenuController;
	UPROPERTY(Transient) TArray<TObjectPtr<UWizardInventoryTile>> Tiles;
	TWeakObjectPtr<AWizardShopActor> ShopSource;
	TWeakObjectPtr<AWizardPlayerState> StateSource;
	TWeakObjectPtr<UWizardInventoryTile> HoverSource;
	FWizardInventoryRequest SelectedItem;
	FDelegateHandle GoldHandle;
	int32 ProductIndex = INDEX_NONE;
	int32 Category = 0;
	float OutsideTime = 0.f;
	bool bKeyboardDetails = false;
};
