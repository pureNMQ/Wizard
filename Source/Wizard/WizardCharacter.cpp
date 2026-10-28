// Copyright Epic Games, Inc. All Rights Reserved.
#include "WizardCharacter.h"
#include "WizardPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

AWizardCharacter::AWizardCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;
	PrimaryActorTick.bCanEverTick = false;
}

UAbilitySystemComponent* AWizardCharacter::GetAbilitySystemComponent() const
{
	const AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>();
	return State ? State->GetAbilitySystemComponent() : nullptr;
}

void AWizardCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>())
	{
		State->InitializeAvatar(this);
	}
	// 现有蓝图可能保存了模板朝向设置，占有时明确切换为鼠标瞄准。
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AWizardCharacter::UnPossessed()
{
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>())
	{
		State->StopBasicAttack();
	}
	Super::UnPossessed();
}

void AWizardCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>())
	{
		State->StopBasicAttack();
	}
	Super::EndPlay(Reason);
}
