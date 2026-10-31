// Wizard 项目：背包图标、可交互悬浮详情与数量Modal视图；布局由蓝图负责。
#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "Input/UIActionBindingHandle.h"
#include "UI/WizardInventoryMenuController.h"
#include "WizardInventoryWidgets.generated.h"
class UImage;
class UTextBlock;
class UBorder;
class UButton;
class UEditableTextBox;
class USlider;
class UUniformGridPanel;
class UInputAction;
class UWizardIconDragPreview;
class UWizardInventoryTile;
DECLARE_MULTICAST_DELEGATE_ThreeParams(FWizardInventoryInspect, int32, UWizardInventoryTile*, bool);
DECLARE_MULTICAST_DELEGATE(FWizardInventoryViewAction);

UCLASS()
class WIZARD_API UWizardInventoryDrag : public UDragDropOperation
{
	GENERATED_BODY()
public:
	FWizardInventoryDragRequest Request;
};
UCLASS()
class WIZARD_API UWizardInventoryTile : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	UWizardInventoryTile(const FObjectInitializer& Initializer);
	void Configure(const FWizardInventoryEntry& Entry, int32 Index, UWizardInventoryMenuController* Controller);
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardIconDragPreview> DragPreviewClass;
	FWizardInventoryInspect OnInspect;
	FWizardInventoryViewAction OnBeginDrag;
protected:
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> Icon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> QuantityText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> SelectionBorder;
private:
	TWeakObjectPtr<UWizardInventoryMenuController> MenuController;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> IconTexture;
	FWizardInventoryDragRequest PendingDrag;
	int32 SlotIndex = INDEX_NONE;
	bool bCanDrag = false;
};
UCLASS()
class WIZARD_API UWizardInventoryPopover : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void Configure(const UWizardItemDefinition* Definition, int32 Quantity);
	FWizardInventoryViewAction OnUse, OnSplit, OnDiscard;
	UWidget* GetFirstAction() const;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailsText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> UseButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> SplitButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> DiscardButton;
private:
	UFUNCTION() void HandleUse();
	UFUNCTION() void HandleSplit();
	UFUNCTION() void HandleDiscard();
};
UCLASS()
class WIZARD_API UWizardInventoryQuantityModal : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UWizardInventoryQuantityModal();
	void Configure(UWizardInventoryMenuController* Controller, const FWizardInventoryQuantityRequest& Request, int32 Maximum);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Heading;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Feedback;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> QuantityInput;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<USlider> QuantitySlider;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ConfirmButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CancelButton;
private:
	friend class FWizardInventoryQuantityControlsTest;
	UFUNCTION() void Confirm();
	UFUNCTION() void HandleQuantityTextChanged(const FText& Text);
	UFUNCTION() void HandleQuantityTextCommitted(const FText& Text, ETextCommit::Type Method);
	UFUNCTION() void HandleSliderChanged(float Value);
	void SetQuantity(int32 Value);
	UFUNCTION() void Cancel();
	TWeakObjectPtr<UWizardInventoryMenuController> MenuController;
	FWizardInventoryQuantityRequest Pending;
	int32 MaximumQuantity = 0;
	int32 CurrentQuantity = 1;
	bool bSynchronizingQuantity = false;
};
UCLASS()
class WIZARD_API UWizardInventoryMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UWizardInventoryMenu();
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardInventoryTile> TileClass;
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardInventoryQuantityModal> QuantityModalClass;
	UPROPERTY(EditDefaultsOnly) TObjectPtr<UInputAction> InventoryAction;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual bool NativeOnHandleBackAction() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UUniformGridPanel> InventoryGrid;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Feedback;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UWizardInventoryPopover> DetailsPopover;
private:
	void Refresh();
	void HandleFeedback(EWizardInventoryResult Result);
	void CloseSession();
	void HandleToggle();
	void Inspect(int32 Index, UWizardInventoryTile* Source, bool bKeyboard);
	void HideDetails();
	void OpenQuantity(bool bDiscard);
	void HandleUse();
	void HandleSplit();
	void HandleDiscard();
	UPROPERTY(Transient) TObjectPtr<UWizardInventoryMenuController> MenuController;
	UPROPERTY(Transient) TArray<TObjectPtr<UWizardInventoryTile>> Tiles;
	TWeakObjectPtr<UWizardInventoryTile> HoverSource;
	float OutsideTime = 0.f;
	bool bKeyboardDetails = false;
	FUIActionBindingHandle ToggleHandle;
};
