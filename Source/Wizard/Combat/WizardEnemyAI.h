// Wizard 项目：行为树驱动追击和 GAS 攻击。
#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "BehaviorTree/BTTaskNode.h"
#include "WizardEnemyAI.generated.h"
UCLASS()
class WIZARD_API AWizardEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	void StartEnemyBehavior();
protected:
	virtual void OnPossess(APawn* InPawn) override;
};
UCLASS()
class WIZARD_API UWizardBTTask_PursueTarget : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UWizardBTTask_PursueTarget();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
UCLASS()
class WIZARD_API UWizardBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UWizardBTTask_Attack();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
