// Wizard 项目：只监听已提交状态；同步小快照无待写队列，生命周期结束前没有异步尾项。
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WizardSaveComponent.generated.h"

class AWizardPlayerState;
class UWizardSaveGame;
UENUM(BlueprintType)
enum class EWizardSaveStatus : uint8 { NotStarted, Ready, Saved, WriteFailed, LoadBlocked, RecoveredBackup };
DECLARE_MULTICAST_DELEGATE(FWizardSaveStatusChanged);

UCLASS()
class WIZARD_API UWizardSaveComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardSaveComponent();
	// 在首次占有和能力初始化后调用一次；测试使用独立槽名，不能读写玩家正式进度。
	bool StartSession(const FString& SlotName = TEXT("WizardProgress"));
	// 写失败可显式重试；阻塞加载时保持原件，拒绝覆盖。
	bool SaveNow();
	UFUNCTION(BlueprintPure, Category="Save") EWizardSaveStatus GetStatus() const { return Status; }
	UFUNCTION(BlueprintPure, Category="Save") FText GetNotice() const { return Notice; }
	FString GetSavePath() const { return SavePath; }
	FWizardSaveStatusChanged OnStatusChanged;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	AWizardPlayerState* GetPlayer() const;
	UWizardSaveGame* Capture() const;
	bool TryRestore(const TArray<uint8>& Bytes);
	void HandleChanged();
	void SetStatus(EWizardSaveStatus Value, const FText& Message = FText::GetEmpty());
	FDelegateHandle InventoryHandle, GoldHandle, LoadoutHandle;
	FString SavePath;
	TArray<uint8> LastSaved;
	EWizardSaveStatus Status = EWizardSaveStatus::NotStarted;
	FText Notice;
	bool bStarting = false;
	bool bSaving = false;
	bool bLoadBlocked = false;
};
