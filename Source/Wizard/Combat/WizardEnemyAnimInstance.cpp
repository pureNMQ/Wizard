#include "Combat/WizardEnemyAnimInstance.h"
#include "GameFramework/Pawn.h"
void UWizardEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const APawn* Pawn = TryGetPawnOwner();
	Speed = Pawn ? Pawn->GetVelocity().Size2D() : 0;
}
