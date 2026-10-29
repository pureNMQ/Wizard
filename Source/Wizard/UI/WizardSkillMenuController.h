// Wizard 项目：炼金台MVC控制器，不依赖Widget或Slate。
#pragma once
#include "CoreMinimal.h"
#include "Skills/WizardSkillDefinition.h"
#include "WizardSkillMenuController.generated.h"

class UWizardSkillLoadoutComponent;
class UWizardSkillMenuController;

struct FWizardSkillDragRequest
{
	FName SkillId;
	int32 SourceSlot = INDEX_NONE;
	TWeakObjectPtr<UWizardSkillMenuController> Source;
};

struct FWizardSkillMenuEntry
{
	const UWizardSkillDefinition* Definition = nullptr;
	int32 Slot = INDEX_NONE;
	bool bAvailable = true;
};

DECLARE_MULTICAST_DELEGATE(FWizardSkillMenuChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FWizardSkillMenuFeedback, EWizardSkillResult);

UCLASS()
class WIZARD_API UWizardSkillMenuController : public UObject
{
	GENERATED_BODY()
public:
	// 每次页面激活创建独立实例；失败不建立订阅，关闭后拒绝旧拖拽。
	bool Open(UWizardSkillLoadoutComponent* InModel);
	void Close();
	void GetEntries(TArray<FWizardSkillMenuEntry>& Candidates, TArray<FWizardSkillMenuEntry>& Slots) const;
	bool CanDragSkill(FName Id, int32 SourceSlot) const;
	// 只消费本次页面会话的有效拖拽；业务失败通过反馈事件报告，失败不改模型。
	bool HandleDrop(const FWizardSkillDragRequest& Request, int32 TargetSlot);
	FWizardSkillMenuChanged OnChanged;
	FWizardSkillMenuFeedback OnFeedback;
protected:
	virtual void BeginDestroy() override;
private:
	void HandleModelChanged();
	TWeakObjectPtr<UWizardSkillLoadoutComponent> Model;
	FDelegateHandle ChangedHandle;
};
