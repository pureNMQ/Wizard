#include "Save/WizardSaveComponent.h"
#include "Save/WizardSaveGame.h"
#include "WizardPlayerState.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

UWizardSaveComponent::UWizardSaveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AWizardPlayerState* UWizardSaveComponent::GetPlayer() const { return Cast<AWizardPlayerState>(GetOwner()); }

UWizardSaveGame* UWizardSaveComponent::Capture() const
{
	const AWizardPlayerState* Player = GetPlayer();
	if (!Player || !Player->HasAuthority() || Player->GetSkills()->ValidateSnapshot(Player->GetSkills()->GetSnapshot()) != EWizardSkillResult::Success) return nullptr;
	UWizardSaveGame* Snapshot = NewObject<UWizardSaveGame>();
	Snapshot->Gold = Player->GetGold();
	Snapshot->Inventory = Player->GetInventory()->GetSnapshot();
	Snapshot->Inventory.Revision = 0;
	Snapshot->Loadout = Player->GetSkills()->GetSnapshot();
	return Snapshot;
}

bool UWizardSaveComponent::TryRestore(const TArray<uint8>& Bytes)
{
	UWizardSaveGame* Snapshot = WizardSaveStorage::Decode(Bytes);
	if (!Snapshot)
	{
		UE_LOG(LogTemp, Warning, TEXT("存档解码失败：封套长度/CRC或SaveGame类型无效。槽位=%s"), *SavePath);
		return false;
	}
	if (Snapshot->SchemaVersion != UWizardSaveGame::CurrentVersion)
	{
		UE_LOG(LogTemp, Warning, TEXT("存档版本不兼容：实际%d，支持%d。槽位=%s"), Snapshot->SchemaVersion, UWizardSaveGame::CurrentVersion, *SavePath);
		return false;
	}
	if (!GetPlayer()->RestorePersistentState(Snapshot->Gold, Snapshot->Inventory, Snapshot->Loadout))
	{
		UE_LOG(LogTemp, Warning, TEXT("存档业务校验失败：检查金币、格数、定义ID、数量、实例身份或技能搭配。槽位=%s"), *SavePath);
		return false;
	}
	return true;
}

bool UWizardSaveComponent::StartSession(const FString& SlotName)
{
	if (bStarting) return false;
	if (Status != EWizardSaveStatus::NotStarted) return !bLoadBlocked;
	AWizardPlayerState* Player = GetPlayer();
	if (!Player || !Player->HasAuthority() || SlotName.IsEmpty() || SlotName.Contains(TEXT(".."))
		|| SlotName.Contains(TEXT("/")) || SlotName.Contains(TEXT("\\")) || FPaths::GetCleanFilename(SlotName) != SlotName) return false;
	TGuardValue<bool> Guard(bStarting, true);
	SavePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), SlotName + TEXT(".sav"));
	IFileManager& Files = IFileManager::Get();
	const bool bPrimaryExists = Files.FileExists(*SavePath);
	const bool bBackupExists = Files.FileExists(*(SavePath + TEXT(".bak")));
	bool bRestored = false;
	TArray<uint8> Bytes;
	if (bPrimaryExists && WizardSaveStorage::Read(SavePath, Bytes)) bRestored = TryRestore(Bytes);
	if (bRestored)
	{
		// 重新序列化运行快照作为去重基线，忽略旧进程的冲突检测版本。
		WizardSaveStorage::Encode(Capture(), LastSaved);
		SetStatus(EWizardSaveStatus::Ready);
	}
	else if (bPrimaryExists || bBackupExists)
	{
		Bytes.Reset();
		const bool bBackupRestored = bBackupExists && WizardSaveStorage::Read(SavePath + TEXT(".bak"), Bytes) && TryRestore(Bytes);
		// 损坏主档移出槽位再允许保存，避免将其复制到唯一有效备份上。
		const FString Rejected = SavePath + TEXT(".rejected-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		const bool bPreserved = !bPrimaryExists || (bBackupRestored && Files.Move(*Rejected, *SavePath, false, false));
		bLoadBlocked = !bBackupRestored || !bPreserved;
		if (bLoadBlocked)
		{
			SetStatus(EWizardSaveStatus::LoadBlocked, FText::FromString(TEXT("存档加载失败，原文件已保留；本次进度不会保存。")));
		}
		else
		{
			WizardSaveStorage::Encode(Capture(), LastSaved);
			SetStatus(EWizardSaveStatus::RecoveredBackup, FText::FromString(TEXT("存档异常，已恢复上一次备份。")));
		}
	}
	else SetStatus(EWizardSaveStatus::Ready);
	InventoryHandle = Player->GetInventory()->OnChanged.AddUObject(this, &ThisClass::HandleChanged);
	GoldHandle = Player->OnGoldChanged.AddUObject(this, &ThisClass::HandleChanged);
	LoadoutHandle = Player->GetSkills()->OnLoadoutChanged.AddUObject(this, &ThisClass::HandleChanged);
	UE_LOG(LogTemp, Display, TEXT("存档会话：%s，状态=%d"), *SavePath, static_cast<int32>(Status));
	bStarting = false;
	OnStatusChanged.Broadcast();
	return !bLoadBlocked;
}

void UWizardSaveComponent::HandleChanged()
{
	if (!bStarting) SaveNow();
}

bool UWizardSaveComponent::SaveNow()
{
	if (Status == EWizardSaveStatus::NotStarted || bStarting || bSaving || bLoadBlocked) return false;
	bool bSuccess = false;
	FText Failure;
	{
		TGuardValue<bool> Guard(bSaving, true);
		TArray<uint8> Bytes;
		if (!WizardSaveStorage::Encode(Capture(), Bytes)) Failure = FText::FromString(TEXT("存档数据生成失败，进度尚未保存。"));
		else
		{
			if (Bytes == LastSaved && Status != EWizardSaveStatus::WriteFailed) return true;
			const double Started = FPlatformTime::Seconds();
			bSuccess = WizardSaveStorage::Write(SavePath, Bytes);
			if (bSuccess)
			{
				LastSaved = MoveTemp(Bytes);
				UE_LOG(LogTemp, Display, TEXT("存档完成：%s，%d字节，%.2fms"), *SavePath, LastSaved.Num(), (FPlatformTime::Seconds() - Started) * 1000.);
			}
			else Failure = FText::FromString(TEXT("保存失败，请检查磁盘空间和目录权限；进度尚未保存。"));
		}
	}
	// 完成文件操作并释放写锁后才通知，观察者产生的新有效变更会另存最新快照。
	SetStatus(bSuccess ? EWizardSaveStatus::Saved : EWizardSaveStatus::WriteFailed, Failure);
	return bSuccess;
}

void UWizardSaveComponent::SetStatus(EWizardSaveStatus Value, const FText& Message)
{
	Status = Value;
	Notice = Message;
	if (!Notice.IsEmpty()) UE_LOG(LogTemp, Warning, TEXT("存档：%s；文件=%s"), *Notice.ToString(), *SavePath);
	if (!bStarting) OnStatusChanged.Broadcast();
}

void UWizardSaveComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	// 同步写入不存在尚未完成的请求；失败过的最后快照在正常退出时再尝试一次。
	if (Status == EWizardSaveStatus::WriteFailed) SaveNow();
	if (AWizardPlayerState* Player = GetPlayer())
	{
		Player->GetInventory()->OnChanged.Remove(InventoryHandle);
		Player->OnGoldChanged.Remove(GoldHandle);
		Player->GetSkills()->OnLoadoutChanged.Remove(LoadoutHandle);
	}
	Super::EndPlay(Reason);
}
