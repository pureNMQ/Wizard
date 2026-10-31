#include "Combat/WizardEnemyAI.h"
#include "Combat/WizardEnemy.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
void AWizardEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (InPawn->HasActorBegunPlay()) { StartEnemyBehavior(); }
}
void AWizardEnemyAIController::StartEnemyBehavior()
{
	AWizardEnemy* Enemy = Cast<AWizardEnemy>(GetPawn());
	if (Enemy && !Enemy->IsDead() && Enemy->IsConfigurationValid())
	{
		RunBehaviorTree(Enemy->BehaviorTree);
	}
}
UWizardBTTask_PursueTarget::UWizardBTTask_PursueTarget()
{
	NodeName = TEXT("Pursue living player into attack range");
	bNotifyTick = true;
}
EBTNodeResult::Type UWizardBTTask_PursueTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}
void UWizardBTTask_PursueTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	AWizardEnemy* Enemy = AI ? Cast<AWizardEnemy>(AI->GetPawn()) : nullptr;
	APawn* Target = UGameplayStatics::GetPlayerPawn(&OwnerComp, 0);
	if (!Enemy || Enemy->IsDead() || !Enemy->IsConfigurationValid() || !IsValid(Target))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (Enemy->CanAttackTarget(Target))
	{
		AI->StopMovement();
		AI->SetFocus(Target);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	AI->SetFocus(Target);
	// 引擎路径跟踪负责追随移动目标；已有移动请求无需每帧重建。
	if (AI->GetMoveStatus() != EPathFollowingStatus::Moving)
	{
		FAIMoveRequest MoveRequest(Target);
		MoveRequest.SetAcceptanceRadius(FMath::Max(30.f, Enemy->AttackRange * .65f));
		// 攻击按中心距离判定；双方胶囊半径不能扩大到达范围，否则近战会停在射程外。
		MoveRequest.SetReachTestIncludesAgentRadius(false);
		MoveRequest.SetReachTestIncludesGoalRadius(false);
		MoveRequest.SetUsePathfinding(true);
		MoveRequest.SetAllowPartialPath(true);
		MoveRequest.SetNavigationFilter(AI->GetDefaultNavigationFilterClass());
		MoveRequest.SetCanStrafe(false);
		AI->MoveTo(MoveRequest);
	}
}
EBTNodeResult::Type UWizardBTTask_PursueTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (AAIController* AI = OwnerComp.GetAIOwner()) { AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay); }
	return EBTNodeResult::Aborted;
}
UWizardBTTask_Attack::UWizardBTTask_Attack() { NodeName = TEXT("Request GAS enemy attack"); }
EBTNodeResult::Type UWizardBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AWizardEnemy* Enemy = OwnerComp.GetAIOwner() ? Cast<AWizardEnemy>(OwnerComp.GetAIOwner()->GetPawn()) : nullptr;
	return Enemy && Enemy->RequestAttack(UGameplayStatics::GetPlayerPawn(&OwnerComp, 0)) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}
