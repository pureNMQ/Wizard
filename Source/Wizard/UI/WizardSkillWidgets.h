// Wizard 项目：MVC视图仅负责CommonUI生命周期、显示和指针事件转发；布局来自Widget Blueprint。
#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "UI/WizardSkillMenuController.h"
#include "UI/WizardIconDragPreview.h"
#include "WizardSkillWidgets.generated.h"
class UImage;
class UTextBlock;
class UPanelWidget;
class UWizardSkillLoadoutComponent;
class UScrollBox;

UCLASS()
class WIZARD_API UWizardSkillDrag : public UDragDropOperation
{
	GENERATED_BODY()
public:
	FWizardSkillDragRequest Request;
};

UCLASS()
class WIZARD_API UWizardSkillTooltip : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void Configure(const UWizardSkillDefinition* Definition);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
};

UCLASS()
class WIZARD_API UWizardSkillTile : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void Configure(const UWizardSkillDefinition* InDefinition, int32 InSlot, UWizardSkillMenuController* InController);
	void SetStatus(const FString& Status, bool bAvailable);
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardSkillTooltip> TooltipClass;
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardIconDragPreview> DragPreviewClass;
	// 图标中央优先显示冷却，其次显示法力不足；就绪及菜单卡片隐藏状态层。
	void SetHUDState(float Remaining, bool bEnoughMana);
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> Icon;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Title;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> CooldownText;
private:
	UPROPERTY() TObjectPtr<const UWizardSkillDefinition> Definition;
	TWeakObjectPtr<UWizardSkillMenuController> MenuController;
	int32 SlotIndex = INDEX_NONE;
};

UCLASS()
class WIZARD_API UWizardSkillMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UWizardSkillMenu();
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardSkillTile> TileClass;
protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnHandleBackAction() override;
	virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UPanelWidget> Candidates;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UScrollBox> LibraryScroll;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UPanelWidget> LoadoutSlots;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Feedback;
private:
	void Refresh();
	void HandleFeedback(EWizardSkillResult Result);
	void Unbind();
	UPROPERTY(Transient) TObjectPtr<UWizardSkillMenuController> MenuController;
};

UCLASS()
class WIZARD_API UWizardSkillBar : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly) TSubclassOf<UWizardSkillTile> TileClass;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UPanelWidget> SkillSlots;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> Feedback;
private:
	void RebuildSlots();
	void HandleFeedback(EWizardSkillResult Result);
	TWeakObjectPtr<UWizardSkillLoadoutComponent> Skills;
	UPROPERTY() TArray<TObjectPtr<UWizardSkillTile>> Tiles;
	FDelegateHandle ChangedHandle, FeedbackHandle;
	float RefreshTime = 0.f;
};

namespace WizardSkills { WIZARD_API FText ResultText(EWizardSkillResult Result); }
