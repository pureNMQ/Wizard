#include "GAS/WizardAttributeSet.h"
#include "Combat/WizardCombatPresentation.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UWizardAttributeSet::UWizardAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitMaxMana(100.0f);
	InitMana(100.0f);
	InitAttackPower(25.0f);
	InitDefense(0.0f);
	InitMoveSpeed(600.f);
	InitShield(0.f);
	InitIncomingDamage(0.0f);
}

void UWizardAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxHealthAttribute() || Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
	else if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetAttackPowerAttribute() || Attribute == GetDefenseAttribute() || Attribute == GetShieldAttribute() ||
	         Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void UWizardAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	if (Attribute == GetMoveSpeedAttribute())
	{
		if (ACharacter* Character = Cast<ACharacter>(GetOwningAbilitySystemComponent()->GetAvatarActor()))
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.f, NewValue);
		}
	}
	// 持续效果和基础值变化同样必须夹取；上限回升时不补血。
	if (Attribute == GetMaxHealthAttribute() && GetHealth() > NewValue)
	{
		SetHealth(NewValue);
	}
	else if (Attribute == GetMaxManaAttribute() && GetMana() > NewValue)
	{
		SetMana(NewValue);
	}
}

bool UWizardAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	HealthBeforeEffect = GetHealth();
	return Super::PreGameplayEffectExecute(Data);
}

void UWizardAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float Damage = FMath::Max(0.0f, GetIncomingDamage());
		SetIncomingDamage(0.0f);
		const float BeforeHealth = GetHealth();
		const float BeforeShield = GetShield();
		const float Absorbed = FMath::Min(BeforeShield, Damage);
		FWizardCombatResult Result;
		Result.Target = GetOwningAbilitySystemComponent()->GetAvatarActor();
		Result.Source = Data.EffectSpec.GetContext().GetOriginalInstigator();
		Result.Presentation = WizardCombat::ResolvePresentation(Data.EffectSpec.GetContext().GetSourceObject());
		Result.Location = Result.Target ? Result.Target->GetActorLocation() : FVector::ZeroVector;
		if (const FHitResult* Hit = Data.EffectSpec.GetContext().GetHitResult())
			Result.Location = Hit->ImpactPoint;
		Result.HealthLost = FMath::Min(BeforeHealth, Damage - Absorbed);
		Result.Absorbed = Absorbed;
		Result.bKilled = BeforeHealth > 0 && Result.HealthLost >= BeforeHealth;
		Result.bShieldBroken = BeforeShield > 0 && Absorbed >= BeforeShield;
		// SetHealth会同步触发死亡/清场；先发布本次已经确定的结算结果，表现不得回写属性。
		if (BeforeHealth > 0)
		{
			OnCombatResolved.Broadcast(Result);
			WizardCombat::Report(Result);
		}
		SetShield(FMath::Max(0.f, GetShield() - Absorbed));
		SetHealth(FMath::Clamp(GetHealth() - (Damage - Absorbed), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute() || Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && Data.EffectSpec.GetContext().GetSourceObject() &&
	    WizardCombat::ResolvePresentation(Data.EffectSpec.GetContext().GetSourceObject()) && GetHealth() > HealthBeforeEffect)
	{
		FWizardCombatResult Result;
		Result.Target = GetOwningAbilitySystemComponent()->GetAvatarActor();
		Result.Source = Data.EffectSpec.GetContext().GetOriginalInstigator();
		Result.Location = Result.Target ? Result.Target->GetActorLocation() : FVector::ZeroVector;
		Result.Healed = GetHealth() - HealthBeforeEffect;
		OnCombatResolved.Broadcast(Result);
		WizardCombat::Report(Result);
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute() || Data.EvaluatedData.Attribute == GetMaxManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	}
}
