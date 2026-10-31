// Wizard 项目：通过真实世界、弹丸碰撞和属性事件检查战斗链路。
// 此开发命令不模拟物理按键，实际 Enhanced Input / CommonUI 返回仍需单独验收。
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/WizardCombatConfig.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardProjectile.h"
#include "Containers/Ticker.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GAS/WizardAttributeSet.h"
#include "GAS/WizardBasicAttackAbility.h"
#include "GAS/WizardCombatTags.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "UI/WizardPlayerStatusWidget.h"
#include "UI/WizardUIRoot.h"
#include "UI/WizardPauseMenu.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "WizardCharacter.h"
#include "WizardPlayerController.h"
#include "WizardPlayerState.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardCombatSmoke, Log, All);

namespace WizardCombatSmoke
{
	struct FRun : public TSharedFromThis<FRun>
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AWizardPlayerController> Controller;
		TWeakObjectPtr<AWizardPlayerState> State;
		TWeakObjectPtr<AWizardEnemy> Target;
		TWeakObjectPtr<UWizardPlayerStatusWidget> HUD;
		TWeakObjectPtr<UClass> ConfiguredProjectileClass;
		TWeakObjectPtr<UAnimMontage> ConfiguredMontage;
		TWeakObjectPtr<UClass> ConfiguredAbilityClass;
		FDelegateHandle SpawnHandle;
		FDelegateHandle CooldownHandle;
		TWeakObjectPtr<UAnimInstance> SlowedAnim;
		float OriginalMontageRate = 1.f;
		float NormalMontageSeconds = 0.f;
		float OriginalPlayerHealth = 0.f;
		float Elapsed = 0.f;
		float TotalElapsed = 0.f;
		float Interval = 0.f;
		float InitialHealth = 0.f;
		float InitialMana = 0.f;
		float SingleDamage = 0.f;
		float Snapshot = 0.f;
		int32 Phase = -1;
		int32 SpawnCount = 0;
		int32 SpawnSnapshot = 0;
		int32 DuplicateEventCount = 0;
		bool bInjectDuplicate = false;
		bool bSlowCooldownWhileActive = false;
		bool bPlayerHealthChanged = false;
		bool bOriginalTick = true;
		bool bOriginalSuppressCues = false;
		bool bHasOriginalSuppressCues = false;

		float Health() const { return Target->GetAttributes()->GetHealth(); }
		UAnimInstance* Anim() const
		{
			const ACharacter* Pawn = Controller.IsValid() ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
			return Pawn && Pawn->GetMesh() ? Pawn->GetMesh()->GetAnimInstance() : nullptr;
		}
		const FGameplayAbilitySpec* AttackSpec() const
		{
			if (!State.IsValid()) { return nullptr; }
			for (const FGameplayAbilitySpec& Spec : State->GetAbilitySystemComponent()->GetActivatableAbilities())
			{
				if (Spec.Ability && Spec.Ability->GetClass() == ConfiguredAbilityClass.Get()) { return &Spec; }
			}
			return nullptr;
		}
		bool IsAttackActive() const { const FGameplayAbilitySpec* Spec = AttackSpec(); return Spec && Spec->IsActive(); }
		bool HasNoAttackTasks() const
		{
			const FGameplayAbilitySpec* Spec = AttackSpec();
			const UGameplayAbility* Instance = Spec ? Spec->GetPrimaryInstance() : nullptr;
			const FArrayProperty* Tasks = Instance ? FindFProperty<FArrayProperty>(Instance->GetClass(), TEXT("ActiveTasks")) : nullptr;
			return Tasks && FScriptArrayHelper(Tasks, Tasks->ContainerPtrToValuePtr<void>(Instance)).Num() == 0;
		}
		bool IsMontagePlaying() const { return Anim() && Anim()->Montage_IsPlaying(ConfiguredMontage.Get()); }
		bool IsReady() const
		{
			return !IsAttackActive() && !State->GetAbilitySystemComponent()->HasMatchingGameplayTag(WizardCombat::BasicAttackCooldownTag());
		}
		float ShotWindow() const { return NormalMontageSeconds + Interval + 0.7f; }
		void OnProjectileSpawned(AActor* Actor)
		{
			if (!Actor || Actor->GetWorld() != World.Get() || Actor->GetClass() != ConfiguredProjectileClass.Get()) { return; }
			++SpawnCount;
			if (bInjectDuplicate)
			{
				// 首次出手始终来自真实蒙太奇 Notify；只在已生成弹丸后注入重复事件，
				// 检查同一 Notify 回调仍在栈内时也不能重入生成第二发。
				bInjectDuplicate = false;
				FGameplayEventData Event;
				Event.EventTag = WizardCombat::BasicAttackFireTag();
				Event.Instigator = Controller->GetPawn();
				Event.Target = Controller->GetPawn();
				Event.OptionalObject = ConfiguredMontage.Get();
				for (int32 Duplicate = 0; Duplicate < 2; ++Duplicate)
				{
					++DuplicateEventCount;
					State->GetAbilitySystemComponent()->HandleGameplayEvent(Event.EventTag, &Event);
				}
			}
		}
		void BindObservers()
		{
			TWeakPtr<FRun> WeakSelf = AsShared();
			SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([WeakSelf](AActor* Actor)
			{
				if (const TSharedPtr<FRun> Test = WeakSelf.Pin()) { Test->OnProjectileSpawned(Actor); }
			}));
			CooldownHandle = State->GetAbilitySystemComponent()->RegisterGameplayTagEvent(WizardCombat::BasicAttackCooldownTag(), EGameplayTagEventType::NewOrRemoved).AddLambda([WeakSelf](FGameplayTag, int32 Count)
			{
				if (const TSharedPtr<FRun> Test = WeakSelf.Pin())
				{
					if (Test->Phase == 28 && Count == 0) { Test->bSlowCooldownWhileActive |= Test->IsAttackActive(); }
				}
			});
		}
		void RestoreCueSuppression()
		{
			if (Target.IsValid() && bHasOriginalSuppressCues)
			{
				Target->GetAbilitySystemComponent()->bSuppressGameplayCues = bOriginalSuppressCues;
			}
		}
		void Cleanup()
		{
			if (State.IsValid()) { State->StopBasicAttack(); }
			if (World.IsValid()) { World->RemoveOnActorSpawnedHandler(SpawnHandle); }
			if (State.IsValid())
			{
				State->GetAbilitySystemComponent()->RegisterGameplayTagEvent(WizardCombat::BasicAttackCooldownTag(), EGameplayTagEventType::NewOrRemoved).Remove(CooldownHandle);
				if (bPlayerHealthChanged) { State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), OriginalPlayerHealth); }
			}
			if (SlowedAnim.IsValid()) { SlowedAnim->Montage_SetPlayRate(ConfiguredMontage.Get(), OriginalMontageRate); }
			RestoreCueSuppression();
			if (Controller.IsValid())
			{
				Controller->SetMenuOpen(false);
				Controller->SetActorTickEnabled(bOriginalTick);
			}
			if (Target.IsValid()) { Target->Destroy(); }
		}
		bool Finish(bool bSuccess, const TCHAR* Reason)
		{
			Cleanup();
			if (bSuccess)
			{
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_SMOKE_PASS: %s"), Reason);
			}
			else
			{
				UE_LOG(LogWizardCombatSmoke, Error, TEXT("WIZARD_COMBAT_SMOKE_FAIL: phase=%d %s"), Phase, Reason);
			}
			if (World.IsValid() && World->WorldType == EWorldType::Game)
			{
				FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
			}
			return false;
		}
		void Next() { ++Phase; Elapsed = 0.f; }
		bool Tick(float Delta)
		{
			if (!World.IsValid() || !Controller.IsValid() || !State.IsValid() || !Target.IsValid() || !HUD.IsValid()
				|| !ConfiguredProjectileClass.IsValid() || !ConfiguredMontage.IsValid())
			{
				return Finish(false, TEXT("Required runtime object disappeared"));
			}
			Elapsed += Delta;
			TotalElapsed += Delta;
			if (TotalElapsed > 120.f) { return Finish(false, TEXT("Combat smoke exceeded its 120 second bound")); }
			switch (Phase)
			{
			case -1:
				if (Elapsed < 0.25f || !IsReady()) { break; }
				InitialHealth = Health();
				InitialMana = State->GetAttributes()->GetMana();
				SpawnSnapshot = SpawnCount;
				bInjectDuplicate = true;
				State->StartBasicAttack();
				if (SpawnCount != SpawnSnapshot || Health() != InitialHealth || !IsAttackActive() || !IsMontagePlaying())
				{
					return Finish(false, TEXT("Start must activate montage/tasks without firing before its authored Notify"));
				}
				State->ReleaseBasicAttack();
				if (!IsAttackActive() || !IsMontagePlaying()) { return Finish(false, TEXT("Immediate ordinary release cancelled the one requested shot")); }
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_WINDUP: SpawnDelta=0 Damage=0 ActiveAfterRelease=1 MontagePlaying=1"));
				Next();
				break;
			case 0:
				if (Elapsed < ShotWindow()) { break; }
				SingleDamage = InitialHealth - Health();
				if (SingleDamage <= 0.f
					|| !FMath::IsNearlyEqual(HUD->GetDisplayedPlayerHealth(), State->GetAttributes()->GetHealth())
					|| State->GetAttributes()->GetMana() != InitialMana || SpawnCount - SpawnSnapshot != 1
					|| DuplicateEventCount != 2 || IsAttackActive() || IsMontagePlaying() || !HasNoAttackTasks())
				{
					return Finish(false, TEXT("Released tap must fire exactly once from Notify, ignore duplicate events, update HUD and end montage/tasks"));
				}
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_NOTIFY: SpawnDelta=%d DuplicateEvents=%d ActiveAbility=0 ActiveTasks=0 MontagePlaying=0"), SpawnCount - SpawnSnapshot, DuplicateEventCount);
				Snapshot = Health();
				Next();
				break;
			case 1:
				if (Elapsed < Interval * 2.f + 0.2f) { break; }
				if (Health() != Snapshot) { return Finish(false, TEXT("Release did not stop shots")); }
				// 只在临时目标 ASC 禁用 Cue；伤害继续走同一 Ability/弹丸/蓝图 GE。
				Target->GetAbilitySystemComponent()->bSuppressGameplayCues = true;
				State->StartBasicAttack();
				State->ReleaseBasicAttack();
				Phase = 8;
				Elapsed = 0.f;
				break;
			case 8:
				if (Elapsed < ShotWindow()) { break; }
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_CUE_SUPPRESSED: Damage=%.3f ExpectedDamage=%.3f"), Snapshot - Health(), SingleDamage);
				if (!FMath::IsNearlyEqual(Snapshot - Health(), SingleDamage))
				{
					return Finish(false, TEXT("Suppressing GameplayCues must preserve damage"));
				}
				RestoreCueSuppression();
				Snapshot = Health();
				State->StartBasicAttack();
				Phase = 2;
				Elapsed = 0.f;
				break;
			case 2:
				if (Elapsed < FMath::Max(Interval, NormalMontageSeconds) * 4.f + 0.7f) { break; }
				{
					const int32 HitCount = FMath::RoundToInt((Snapshot - Health()) / SingleDamage);
					const int32 MinimumHits = FMath::Max(2, FMath::FloorToInt((Elapsed - NormalMontageSeconds - 0.2f) / (FMath::Max(Interval, NormalMontageSeconds) + 0.04f)));
					UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_CADENCE: Hits=%d ExpectedAtLeast=%d Seconds=%.3f Interval=%.3f"), HitCount, MinimumHits, Elapsed, Interval);
					if (HitCount < MinimumHits) { return Finish(false, TEXT("Hold cadence slower than configured interval")); }
				}
				Controller->SetMenuOpen(true);
				if (!World->IsPaused() || !Controller->IsMenuOpen()) { return Finish(false, TEXT("Menu did not pause world")); }
				Snapshot = Health();
				Next();
				break;
			case 3:
				// CoreTicker 随真实时间推进，世界暂停时也能关闭测试菜单。
				if (Elapsed < Interval * 2.f + 0.2f) { break; }
				if (Health() != Snapshot) { return Finish(false, TEXT("World advanced while menu open")); }
				Controller->SetMenuOpen(false);
				if (World->IsPaused()) { return Finish(false, TEXT("Menu did not restore world")); }
				Next();
				break;
			case 4:
				// 已发出的弹丸允许落地；之后再检测是否还在自动连射。
				if (Elapsed < Interval + 0.6f) { break; }
				Snapshot = Health();
				Next();
				break;
			case 5:
				if (Elapsed < Interval * 2.f + 0.2f) { break; }
				if (Health() != Snapshot) { return Finish(false, TEXT("Menu close resumed stale held input")); }
				{
					APawn* OldPawn = Controller->GetPawn();
					if (!OldPawn) { return Finish(false, TEXT("Old avatar missing")); }
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					AWizardCharacter* NewPawn = World->SpawnActor<AWizardCharacter>(OldPawn->GetClass(), OldPawn->GetActorTransform(), Params);
					if (!NewPawn) { return Finish(false, TEXT("Replacement avatar failed to spawn")); }
					State->StartBasicAttack();
					Controller->Possess(NewPawn);
					OldPawn->Destroy();
					const int32 AbilityCount = State->GetAbilitySystemComponent()->GetActivatableAbilities().Num();
					State->InitializeAvatar(NewPawn);
					State->InitializeAvatar(NewPawn);
					if (Controller->GetPlayerState<AWizardPlayerState>() != State.Get()
						|| State->GetAbilitySystemComponent()->GetAvatarActor() != NewPawn
						|| State->GetAbilitySystemComponent()->GetActivatableAbilities().Num() != AbilityCount)
					{
						return Finish(false, TEXT("Avatar ownership or idempotent ability grant failed"));
					}
				}
				Next();
				break;
			case 6:
				if (Elapsed < Interval + 0.6f) { break; }
				Snapshot = Health();
				Next();
				break;
			case 7:
				if (Elapsed < Interval * 2.f + 0.2f) { break; }
				if (Health() != Snapshot) { return Finish(false, TEXT("Avatar replacement retained attack timer")); }
				if (IsAttackActive() || IsMontagePlaying() || !HasNoAttackTasks()) { return Finish(false, TEXT("Avatar replacement left montage or AbilityTasks active")); }
				Phase = 20;
				Elapsed = 0.f;
				break;
			case 20:
				if (!IsReady()) { break; }
				SpawnSnapshot = SpawnCount;
				Snapshot = Health();
				State->StartBasicAttack();
				State->StopBasicAttack();
				if (IsAttackActive()) { return Finish(false, TEXT("Strong cancellation left ability active")); }
				Next();
				break;
			case 21:
				if (Elapsed < ShotWindow()) { break; }
				if (SpawnCount != SpawnSnapshot || Health() != Snapshot || IsMontagePlaying() || !HasNoAttackTasks())
				{
					return Finish(false, TEXT("Cancel before Notify produced a late projectile or retained montage/tasks"));
				}
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_CANCEL_WINDUP: SpawnDelta=0 ActiveTasks=0"));
				Next();
				break;
			case 22:
				if (!IsReady()) { break; }
				SpawnSnapshot = SpawnCount;
				Snapshot = Health();
				State->StartBasicAttack();
				Controller->SetMenuOpen(true);
				if (!World->IsPaused() || IsAttackActive() || !HasNoAttackTasks()) { return Finish(false, TEXT("Menu must cancel windup and its tasks before pausing")); }
				Next();
				break;
			case 23:
				if (Elapsed < 0.4f) { break; }
				if (SpawnCount != SpawnSnapshot || Health() != Snapshot) { return Finish(false, TEXT("Paused cancelled windup still fired")); }
				Controller->SetMenuOpen(false);
				Next();
				break;
			case 24:
				if (Elapsed < ShotWindow()) { break; }
				if (SpawnCount != SpawnSnapshot || Health() != Snapshot || IsAttackActive() || IsMontagePlaying() || !HasNoAttackTasks())
				{
					return Finish(false, TEXT("Menu close resumed a cancelled windup"));
				}
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_MENU_WINDUP: SpawnDelta=0 ActiveTasks=0"));
				Next();
				break;
			case 25:
				if (!IsReady()) { break; }
				SpawnSnapshot = SpawnCount;
				Snapshot = Health();
				State->StartBasicAttack();
				bPlayerHealthChanged = true;
				State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
				if (IsAttackActive()) { return Finish(false, TEXT("Death must cancel the active windup")); }
				Next();
				break;
			case 26:
				if (Elapsed < ShotWindow()) { break; }
				if (SpawnCount != SpawnSnapshot || Health() != Snapshot || IsMontagePlaying() || !HasNoAttackTasks())
				{
					return Finish(false, TEXT("Death before Notify produced a late projectile or retained tasks"));
				}
				State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), OriginalPlayerHealth);
				bPlayerHealthChanged = false;
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_DEATH_WINDUP: SpawnDelta=0 ActiveTasks=0 RestoredHealth=%.3f"), OriginalPlayerHealth);
				Phase = 31;
				Elapsed = 0.f;
				break;
			case 27:
				if (!IsReady()) { break; }
				SpawnSnapshot = SpawnCount;
				bSlowCooldownWhileActive = false;
				State->StartBasicAttack();
				if (!IsAttackActive() || !IsMontagePlaying()) { return Finish(false, TEXT("Slow-montage fixture did not start actual ability montage")); }
				SlowedAnim = Anim();
				OriginalMontageRate = Anim()->Montage_GetPlayRate(ConfiguredMontage.Get());
				// 只放慢本次运行实例，CDO、蒙太奇资产和蓝图 GE 均保持只读。
				Anim()->Montage_SetPlayRate(ConfiguredMontage.Get(), OriginalMontageRate * 0.1f);
				Next();
				break;
			case 28:
				if (SpawnCount - SpawnSnapshot < 2)
				{
					if (Elapsed > NormalMontageSeconds * 10.f + ShotWindow() + 3.f) { return Finish(false, TEXT("Cooldown expired during montage but completion failed to resume held fire")); }
					break;
				}
				State->ReleaseBasicAttack();
				if (!bSlowCooldownWhileActive) { return Finish(false, TEXT("Slow fixture did not observe cooldown ending before ability completion")); }
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_SLOW_MONTAGE: CooldownEndedWhileActive=1 SpawnDelta=%d Seconds=%.3f"), SpawnCount - SpawnSnapshot, Elapsed);
				SpawnSnapshot = SpawnCount;
				Next();
				break;
			case 29:
				if (Elapsed < ShotWindow()) { break; }
				if (SpawnCount != SpawnSnapshot || IsAttackActive() || IsMontagePlaying() || !HasNoAttackTasks())
				{
					return Finish(false, TEXT("Released slow-cadence fixture retained a shot, montage or task"));
				}
				Next();
				break;
			case 30:
				return Finish(true, TEXT("Native montage Notify single shot after release; duplicate event guard; GE/HUD; cooldown and montage continuation; cancel/menu/death/interruption windup; avatar replacement; no active montage/tasks"));
			case 31:
				if (!IsReady()) { break; }
				SpawnSnapshot = SpawnCount;
				Snapshot = Health();
				State->StartBasicAttack();
				if (!IsAttackActive() || !IsMontagePlaying()) { return Finish(false, TEXT("Interrupt fixture did not start actual montage")); }
				// 从动画实例打断，走 Task.OnInterrupted，而非测试直接取消 Ability。
				Anim()->Montage_Stop(0.f, ConfiguredMontage.Get());
				Next();
				break;
			case 32:
				if (Elapsed < ShotWindow()) { break; }
				if (SpawnCount != SpawnSnapshot || Health() != Snapshot || IsAttackActive() || IsMontagePlaying() || !HasNoAttackTasks())
				{
					return Finish(false, TEXT("Montage interruption retained held input or produced a late Notify projectile/task"));
				}
				UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_MONTAGE_INTERRUPTED: SpawnDelta=0 ActiveTasks=0"));
				Phase = 27;
				Elapsed = 0.f;
				break;
			}
			return true;
		}
	};

	void Run(UWorld* World)
	{
		// 不在编辑器世界运行，也不在 PIE 中请求退出编辑器。
		if (!World || (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)) { return; }
		AWizardPlayerController* PC = Cast<AWizardPlayerController>(World->GetFirstPlayerController());
		AWizardPlayerState* PS = PC ? PC->GetPlayerState<AWizardPlayerState>() : nullptr;
		AWizardCharacter* Pawn = PC ? Cast<AWizardCharacter>(PC->GetPawn()) : nullptr;
		TSharedRef<FRun> Test = MakeShared<FRun>();
		Test->World = World;
		Test->Controller = PC;
		Test->State = PS;
		if (!PC || !PS || !Pawn) { Test->Finish(false, TEXT("Wizard player objects missing")); return; }
		if (World->IsPaused() || PC->IsMenuOpen()) { Test->Finish(false, TEXT("Start smoke from active gameplay")); return; }
		for (TObjectIterator<UWizardPlayerStatusWidget> It; It; ++It)
		{
			if (It->GetWorld() == World && It->GetOwningPlayer() == PC && It->IsVisible() && It->GetParent() && PC->GetUIRoot() && PC->GetUIRoot()->IsInViewport()) { Test->HUD = *It; break; }
		}
		if (!Test->HUD.IsValid()) { Test->Finish(false, TEXT("Player HUD missing from root")); return; }
		// 使用真实 CommonUI stack；原有 SetMenuOpen 业务测试仍保留。
		UWizardUIRoot* Root = PC->GetUIRoot();
		bool bRootPassed = Root && !Root->HasOpenScreens();
		if (bRootPassed)
		{
			bRootPassed = Root->PushMenu(UWizardPauseMenu::StaticClass()) != nullptr;
			bRootPassed &= Root->PushModal(UWizardPauseMenu::StaticClass()) != nullptr;
			bRootPassed &= Root->PushMenu(UWizardPauseMenu::StaticClass()) == nullptr;
			bRootPassed &= PC->IsMenuOpen() && World->IsPaused();
			bRootPassed &= Root->CloseTop() && Root->HasOpenScreens() && World->IsPaused();
			bRootPassed &= Root->CloseTop() && !Root->HasOpenScreens() && !PC->IsMenuOpen() && !World->IsPaused();
			PC->SetPause(true);
			bRootPassed &= Root->PushModal(UWizardPauseMenu::StaticClass()) != nullptr;
			bRootPassed &= Root->CloseTop() && World->IsPaused();
			PC->SetPause(false);
		}
		if (!bRootPassed)
		{
			if (Root) { for (int32 CloseIndex = 0; CloseIndex < 4 && Root->CloseTop(); ++CloseIndex) {} }
			PC->SetPause(false);
			Test->Finish(false, TEXT("UIRoot nested stacks / modal priority / previous pause failed"));
			return;
		}
		UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_UIROOT_STACK_PASS"));
		Test->bOriginalTick = PC->IsActorTickEnabled();
		Test->OriginalPlayerHealth = PS->GetAttributes()->GetHealth();
		PC->SetActorTickEnabled(false);
		PS->StopBasicAttack();
		FVector Position = FVector::ZeroVector;
		bool bClearFixture = false;
		FCollisionQueryParams Query(SCENE_QUERY_STAT(WizardCombatSmoke), false, Pawn);
		// 只选择无阻挡方向，不移动或删除地图既有目标与道具。
		for (int32 Candidate = 0; Candidate < 8; ++Candidate)
		{
			const FRotator Facing(0.f, Pawn->GetActorRotation().Yaw + Candidate * 45.f, 0.f);
			const FVector End = Pawn->GetActorLocation() + Facing.Vector() * 250.f;
			const FVector Origin = Pawn->GetActorLocation() + Facing.RotateVector(PS->GetCombatConfig()->MuzzleOffset);
			FHitResult BlockingHit;
			const bool bBlockedPath = World->SweepSingleByChannel(BlockingHit, Origin, End + FVector(0, 0, 25), FQuat::Identity,
				ECC_WorldDynamic, FCollisionShape::MakeSphere(12.f), Query);
			const bool bBlockedTarget = World->OverlapBlockingTestByChannel(End, FQuat::Identity, ECC_Pawn,
				FCollisionShape::MakeCapsule(42.f, 88.f), Query);
			if (!bBlockedPath && !bBlockedTarget)
			{
				Pawn->SetActorRotation(Facing);
				Position = End;
				bClearFixture = true;
				break;
			}
		}
		if (!bClearFixture) { Test->Finish(false, TEXT("No clear 250cm target fixture around player; scene left unchanged")); return; }
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWizardEnemy* Target = World->SpawnActor<AWizardEnemy>(AWizardEnemy::StaticClass(), Position, FRotator::ZeroRotator, Params);
		Test->Target = Target;
		if (!Target) { Test->Finish(false, TEXT("Target failed to spawn")); return; }
		Test->bOriginalSuppressCues = Target->GetAbilitySystemComponent()->bSuppressGameplayCues;
		Test->bHasOriginalSuppressCues = true;
		Target->GetAbilitySystemComponent()->bSuppressGameplayCues = false;
		// 测试夹具采用高血量，连射验收不会提前杀死目标。
		Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 10000.f);
		Target->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 10000.f);
		PS->SetAimPoint(Position);
		const UWizardBasicAttackAbility* Ability = PS->GetCombatConfig()->BasicAttackAbilityClass
			? Cast<UWizardBasicAttackAbility>(PS->GetCombatConfig()->BasicAttackAbilityClass->GetDefaultObject()) : nullptr;
		const UGameplayEffect* Cooldown = Ability ? Ability->GetCooldownGameplayEffect() : nullptr;
		Test->ConfiguredAbilityClass = Ability ? Ability->GetClass() : nullptr;
		Test->ConfiguredMontage = Ability ? Ability->AttackMontage.Get() : nullptr;
		if (!Test->ConfiguredMontage.IsValid() || !Ability->IsAnimationConfigurationValid())
		{
			Test->Finish(false, TEXT("Configured montage must be available"));
			return;
		}
		Test->NormalMontageSeconds = Test->ConfiguredMontage->GetPlayLength() / Test->ConfiguredMontage->RateScale;
		Test->ConfiguredProjectileClass = Ability ? Ability->ProjectileClass.Get() : nullptr;
		if (!Test->ConfiguredProjectileClass.IsValid())
		{
			Test->Finish(false, TEXT("Configured projectile class must be available"));
			return;
		}
		if (!Cooldown || !Cooldown->DurationMagnitude.GetStaticMagnitudeIfPossible(1.0f, Test->Interval)
			|| !FMath::IsFinite(Test->Interval) || Test->Interval < 0.05f)
		{
			Test->Finish(false, TEXT("Configured cooldown GE must have a finite static duration >= 0.05s"));
			return;
		}
		UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_SMOKE_BEGIN: Interval=%.3f Target=%s ConfiguredClass=%s"),
			Test->Interval, *Position.ToString(), *Test->ConfiguredProjectileClass->GetPathName());
		UE_LOG(LogWizardCombatSmoke, Display, TEXT("WIZARD_COMBAT_MONTAGE_CONFIG: Montage=%s Seconds=%.3f"),
			*Test->ConfiguredMontage->GetPathName(), Test->NormalMontageSeconds);
		Test->BindObservers();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Test](float Delta) { return Test->Tick(Delta); }));
	}

	FAutoConsoleCommandWithWorld Command(TEXT("Wizard.CombatSmokeTest"),
		TEXT("Check real montage Notify, projectile/GE/HUD, tap/hold/release, cancellation and avatar replacement. Game exits after checking; PIE stays open."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Run));
}
#endif
