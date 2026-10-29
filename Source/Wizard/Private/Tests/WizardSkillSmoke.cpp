// Wizard 项目：真实地图/蒙太奇/粒子/GE的开发运行检查；不替代实际按键验收。
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#include "Skills/WizardSkillLoadoutComponent.h"
#include "Skills/WizardSkillEffects.h"
#include "Skills/WizardAlchemyStation.h"
#include "UI/WizardSkillWidgets.h"
#include "UI/WizardUIRoot.h"
#include "WizardPlayerState.h"
#include "WizardPlayerController.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardCombatConfig.h"
#include "Combat/WizardProjectile.h"
#include "WizardCharacter.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardSkillsSmoke, Log, All);
namespace WizardSkillsSmoke
{
	struct FRun : TSharedFromThis<FRun>
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AWizardPlayerController> Controller;
		TWeakObjectPtr<AWizardPlayerState> State;
		TWeakObjectPtr<AWizardEnemy> Enemy;
		FWizardLoadoutSnapshot Snapshot;
		FVector OriginalLocation, TargetLocation, BeforeBlink;
		float OldMana = 0.f, OldHealth = 0.f;
		bool bOldTick = true;
		int32 Index = 0;
		bool bWaiting = false;
		int32 CancelIndex = 0;
		int32 SpawnCount = 0;
		int32 BeforeCancelSpawns = 0;
		FDelegateHandle SpawnHandle;
		TWeakObjectPtr<AWizardCharacter> TemporaryAvatar;
		double Started = 0, PhaseTime = 0;
		const TArray<FName> Ids = {TEXT("Fireball"),TEXT("IceArrow"),TEXT("Lightning"),TEXT("Blink"),TEXT("Shield"),TEXT("Heal")};
		void Finish(bool bSuccess, const FString& Reason)
		{
			if (World.IsValid()) World->RemoveOnActorSpawnedHandler(SpawnHandle);
			UE_LOG(LogWizardSkillsSmoke, Display, TEXT("WIZARD_SKILLS_SMOKE_%s: %s"), bSuccess ? TEXT("PASS") : TEXT("FAIL"), *Reason);
			if (State.IsValid())
			{
				if (TemporaryAvatar.IsValid() && Controller.IsValid()) State->InitializeAvatar(Controller->GetPawn());
				State->GetSkills()->CancelCasting();
				State->GetSkills()->ClearAvatarEffects();
				State->GetSkills()->RestoreSnapshot(Snapshot);
				State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), OldMana);
				State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), OldHealth);
			}
			if (Controller.IsValid())
			{
				if (Controller->GetUIRoot()) Controller->GetUIRoot()->CloseTop();
				Controller->SetActorTickEnabled(bOldTick);
				if (Controller->GetPawn()) Controller->GetPawn()->SetActorLocation(OriginalLocation);
			}
			if (Enemy.IsValid()) Enemy->Destroy();
			if (TemporaryAvatar.IsValid()) TemporaryAvatar->Destroy();
			if (World.IsValid() && World->WorldType != EWorldType::PIE) FPlatformMisc::RequestExitWithStatus(false, bSuccess ? 0 : 1);
		}
		bool Tick(float DeltaSeconds)
		{
			if (!State.IsValid() || !Controller.IsValid() || !World.IsValid()) { Finish(false, TEXT("World or player lost")); return false; }
			if (FPlatformTime::Seconds() - Started > 60) { Finish(false, TEXT("World time did not finish within timeout")); return false; }
			UWizardSkillLoadoutComponent* Skills = State->GetSkills();
			UAbilitySystemComponent* ASC = State->GetAbilitySystemComponent();
			if (Index == Ids.Num() && CancelIndex < 4)
			{
				if (!bWaiting)
				{
					Skills->RestoreSnapshot(Skills->Catalog->InitialLoadout);
					ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(Skills->Find(TEXT("Fireball"))->CooldownTag));
					ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 100.f);
					ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 100.f);
					State->SetAimPoint(Controller->GetPawn()->GetActorLocation() + FVector(500,0,0));
					BeforeCancelSpawns = SpawnCount;
					if (Skills->ActivateSlot(0) != EWizardSkillResult::Success) { Finish(false, TEXT("Cancel fixture activation failed")); return false; }
					if (CancelIndex == 0) Skills->CancelCasting();
					else if (CancelIndex == 1) { Controller->SetMenuOpen(true); Controller->SetMenuOpen(false); }
					else if (CancelIndex == 2) ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
					else
					{
						FActorSpawnParameters Params;
						Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						TemporaryAvatar = World->SpawnActor<AWizardCharacter>(AWizardCharacter::StaticClass(), Controller->GetPawn()->GetActorLocation() + FVector(0,0,1000), FRotator::ZeroRotator, Params);
						if (!TemporaryAvatar.IsValid()) { Finish(false, TEXT("Replacement Avatar fixture failed")); return false; }
						State->InitializeAvatar(TemporaryAvatar.Get());
					}
					PhaseTime = World->GetTimeSeconds();
					bWaiting = true;
					return true;
				}
				if (World->GetTimeSeconds() - PhaseTime < .5f) return true;
				if (ASC->HasMatchingGameplayTag(WizardSkills::CastingTag()) || SpawnCount != BeforeCancelSpawns
					|| !FMath::IsNearlyEqual(State->GetAttributes()->GetMana(), 88.f)) { Finish(false, TEXT("Cancelled skill leaked a projectile/task or refunded commit")); return false; }
				if (TemporaryAvatar.IsValid())
				{
					State->InitializeAvatar(Controller->GetPawn());
					TemporaryAvatar->Destroy();
					TemporaryAvatar.Reset();
				}
				ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 100.f);
				UE_LOG(LogWizardSkillsSmoke, Display, TEXT("WIZARD_SKILL_CANCEL: Path=%d SpawnDelta=0 NoRefund=1 Casting=0"), CancelIndex);
				++CancelIndex;
				bWaiting = false;
				return true;
			}
			if (Index >= Ids.Num())
			{
				AWizardAlchemyStation* Station = nullptr;
				for (TActorIterator<AWizardAlchemyStation> It(World.Get()); It; ++It) { Station = *It; break; }
				if (!Station || !Station->AlchemyActor) { Finish(false, TEXT("Alchemy station reference missing")); return false; }
				APawn* Pawn = Controller->GetPawn();
				FVector Near = Station->AlchemyActor->GetActorLocation() + FVector(180,0,105);
				Pawn->SetActorLocation(Near);
				if (!Station->CanInteract(State.Get())) { Finish(false, TEXT("Alchemy range/LOS fixture rejected")); return false; }
				const auto Before = Skills->GetSnapshot();
				if (Skills->SwapSlots(0,1) != EWizardSkillResult::Success || Skills->GetSnapshot().Slots[0] != Before.Slots[1]) { Finish(false, TEXT("Swap failed")); return false; }
				if (Skills->EquipSkill(Skills->GetSnapshot().Slots[1], 0) != EWizardSkillResult::DuplicateSkill) { Finish(false, TEXT("Duplicate accepted")); return false; }
				UClass* MenuClass = LoadClass<UWizardSkillMenu>(nullptr, TEXT("/Game/Wizard/UI/WBP_SkillMenu.WBP_SkillMenu_C"));
				if (!Controller->GetUIRoot()->PushMenu(MenuClass) || !World->IsPaused()) { Finish(false, TEXT("Skill menu did not pause")); return false; }
				Controller->GetUIRoot()->CloseTop();
				if (World->IsPaused()) { Finish(false, TEXT("Skill menu did not restore")); return false; }
				Finish(true, TEXT("Six real montage skills, four cancellation paths, cost/cooldown, Alchemy/swap/duplicate and menu pause passed"));
				return false;
			}
			const UWizardSkillDefinition* Definition = Skills->Find(Ids[Index]);
			if (!bWaiting)
			{
				FWizardLoadoutSnapshot TestLoadout;
				TestLoadout.Slots.Add(Ids[Index]);
				for (FName Id : Ids) { if (Id != Ids[Index] && TestLoadout.Slots.Num() < 4) TestLoadout.Slots.Add(Id); }
				Skills->RestoreSnapshot(TestLoadout);
				ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 100.f);
				ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), Index == 5 ? 70.f : 100.f);
				if (Index == 3 && Enemy.IsValid()) Enemy->Destroy();
				if (Enemy.IsValid()) Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 1000.f);
				State->SetAimPoint(TargetLocation);
				BeforeBlink = Controller->GetPawn()->GetActorLocation();
				const EWizardSkillResult Result = Skills->ActivateSlot(0);
				if (Result != EWizardSkillResult::Success) { Finish(false, FString::Printf(TEXT("%s activation failed: %d"), *Ids[Index].ToString(), int32(Result))); return false; }
				if (!FMath::IsNearlyEqual(State->GetAttributes()->GetMana(), 100.f - Definition->ManaCost)) { Finish(false, TEXT("Cost not committed once")); return false; }
				PhaseTime = World->GetTimeSeconds();
				bWaiting = true;
				return true;
			}
			if (World->GetTimeSeconds() - PhaseTime < 1.0f) return true;
			bool bPassed = !ASC->HasMatchingGameplayTag(WizardSkills::CastingTag());
			if (Index < 3) bPassed &= Enemy.IsValid() && FMath::IsNearlyEqual(Enemy->GetAttributes()->GetHealth(), 1000.f - 25.f * Definition->Power);
			if (Index == 1) bPassed &= Enemy.IsValid() && Enemy->GetAttributes()->GetMoveSpeed() < 600.f;
			if (Index == 3) bPassed &= FVector::Dist2D(BeforeBlink, Controller->GetPawn()->GetActorLocation()) > 100.f;
			if (Index == 4) bPassed &= FMath::IsNearlyEqual(State->GetAttributes()->GetShield(), Definition->Power);
			if (Index == 5) bPassed &= FMath::IsNearlyEqual(State->GetAttributes()->GetHealth(), 100.f);
			UE_LOG(LogWizardSkillsSmoke, Display, TEXT("WIZARD_SKILL_CHECK: %s Passed=%d Mana=%.1f Cooldown=%.2f"), *Ids[Index].ToString(), bPassed, State->GetAttributes()->GetMana(), Skills->GetCooldownRemaining(Definition));
			if (!bPassed) { Finish(false, FString::Printf(TEXT("%s did not produce expected result after real montage"), *Ids[Index].ToString())); return false; }
			++Index;
			bWaiting = false;
			return true;
		}
	};
	void Start(const TArray<FString>& Args, UWorld* World)
	{
		AWizardPlayerController* PC = World ? Cast<AWizardPlayerController>(World->GetFirstPlayerController()) : nullptr;
		AWizardPlayerState* PS = PC ? PC->GetPlayerState<AWizardPlayerState>() : nullptr;
		if (!PS || !PC->GetPawn() || !PS->GetSkills()->Catalog || !PS->GetSkills()->Catalog->IsConfigurationValid())
		{
			UE_LOG(LogWizardSkillsSmoke, Error, TEXT("WIZARD_SKILLS_SMOKE_FAIL: Player or catalog invalid"));
			return;
		}
		TSharedRef<FRun> Run = MakeShared<FRun>();
		Run->World = World; Run->Controller = PC; Run->State = PS;
		Run->Started = FPlatformTime::Seconds();
		Run->Snapshot = PS->GetSkills()->GetSnapshot();
		Run->OriginalLocation = PC->GetPawn()->GetActorLocation();
		Run->OldMana = PS->GetAttributes()->GetMana(); Run->OldHealth = PS->GetAttributes()->GetHealth();
		TWeakPtr<FRun> WeakRun = Run;
		Run->SpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateLambda([WeakRun](AActor* Actor)
		{
			if (Actor && Actor->IsA<AWizardProjectile>()) { if (auto Active = WeakRun.Pin()) ++Active->SpawnCount; }
		}));
		Run->bOldTick = PC->IsActorTickEnabled();
		PC->SetActorTickEnabled(false);
		PS->StopBasicAttack();
		const FVector Position = PC->GetPawn()->GetActorLocation();
		FCollisionQueryParams Query(SCENE_QUERY_STAT(SkillSmoke), false, PC->GetPawn());
		bool bFound = false;
		for (int32 Candidate = 0; Candidate < 8; ++Candidate)
		{
			const FRotator Facing(0, Candidate * 45.f, 0);
			const FVector Target = Position + Facing.Vector() * 250.f;
			FHitResult Hit;
			if (!World->SweepSingleByChannel(Hit, Position, Target, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(42,96), Query))
			{
				PC->GetPawn()->SetActorRotation(Facing);
				Run->TargetLocation = Target;
				bFound = true;
				break;
			}
		}
		if (!bFound) { Run->Finish(false, TEXT("No clear fixture around player")); return; }
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWizardEnemy* Enemy = World->SpawnActor<AWizardEnemy>(AWizardEnemy::StaticClass(), Run->TargetLocation, FRotator::ZeroRotator, Params);
		if (!Enemy) { Run->Finish(false, TEXT("No target")); return; }
		Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 1000.f);
		Run->Enemy = Enemy;
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt) { return Run->Tick(Dt); }));
	}
	FAutoConsoleCommandWithWorldAndArgs Command(TEXT("Wizard.SkillsSmokeTest"), TEXT("验证六技能和Alchemy搭配；在独立游戏中结束后退出。"), FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Start));
}
#endif
