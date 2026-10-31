#include "Loot/WizardWorldPickup.h"
#include "Inventory/WizardItemDefinition.h"
#include "Loot/WizardLootSettings.h"
#include "Challenge/WizardChallengeArena.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/Image.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"

void UWizardPickupIcon::SetIcon(UTexture2D* Texture)
{
	if (IconImage) IconImage->SetBrushFromTexture(Texture);
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

AWizardWorldPickup::AWizardWorldPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);
	PickupSphere->InitSphereRadius(160.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
	PickupSphere->SetCanEverAffectNavigation(false);
	IconComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupIcon"));
	IconComponent->SetupAttachment(PickupSphere);
	IconComponent->SetWidgetSpace(EWidgetSpace::Screen);
	IconComponent->SetRelativeLocation(FVector(0, 0, 32));
	IconComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	IconComponent->SetVisibility(false);
}

bool AWizardWorldPickup::HasValidConfiguration() const
{
	constexpr EClassFlags InvalidFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;
	return !GetClass()->HasAnyClassFlags(InvalidFlags) && FMath::IsFinite(PickupRadius) && PickupRadius > 0.f;
}

AWizardWorldPickup* AWizardWorldPickup::SpawnItem(UWorld* World, const UWizardItemDefinition* Definition, int32 Quantity, const FVector& Location,
	AWizardChallengeArena* Challenge)
{
	if (!Definition || !Definition->IsConfigurationValid()) return nullptr;
	UTexture2D* Icon = Definition->GetIconTexture();
	return SpawnPrepared(World, Definition, Icon, false, Quantity, Location, Challenge);
}

AWizardWorldPickup* AWizardWorldPickup::SpawnGold(UWorld* World, UTexture2D* Icon, int32 Quantity, const FVector& Location, AWizardChallengeArena* Challenge)
{
	return SpawnPrepared(World, nullptr, Icon ? Icon : UWizardLootSettings::ResolveIcon({}), true, Quantity, Location, Challenge);
}

AWizardWorldPickup* AWizardWorldPickup::SpawnPrepared(UWorld* World, const UWizardItemDefinition* Definition, UTexture2D* Icon, bool bIsGold, int32 Quantity,
	const FVector& Location, AWizardChallengeArena* Challenge)
{
	const auto PickupClass = GetDefault<UWizardLootSettings>()->PickupClass.LoadSynchronous();
	if (!World || !PickupClass || Quantity <= 0 || Location.ContainsNaN()) return nullptr;
	const AWizardWorldPickup* Defaults = PickupClass->GetDefaultObject<AWizardWorldPickup>();
	if (!Defaults->HasValidConfiguration()) return nullptr;
	FVector GroundLocation = Location;
	FHitResult Ground;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(WizardDropGround), false);
	// 从脚下开始，避免把角色胶囊或敌人的顶部误认为地面。
	if (World->LineTraceSingleByObjectType(Ground, Location + FVector(0,0,20), Location - FVector(0,0,1000), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
	{
		GroundLocation.Z = Ground.ImpactPoint.Z + 8.f;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardWorldPickup* Pickup = World->SpawnActor<AWizardWorldPickup>(PickupClass, GroundLocation, FRotator::ZeroRotator, Params);
	if (!IsValid(Pickup)) return nullptr;
	Pickup->ItemDefinition = Definition;
	Pickup->IconTexture = Icon;
	Pickup->RemainingQuantity = Quantity;
	Pickup->bGold = bIsGold;
	Pickup->SourceChallenge = Challenge;
	// 蓝图通过组件标签声明装备光柱；类型来自物品定义，金币和普通物品仅保留底部柔光。
	const bool bShowEquipmentBeam = !bIsGold && Definition && Definition->Type == EWizardItemType::Equipment;
	TInlineComponentArray<USceneComponent*> PresentationComponents(Pickup);
	for (USceneComponent* Component : PresentationComponents)
	{
		if (Component->ComponentHasTag(TEXT("EquipmentBeam"))) Component->SetVisibility(bShowEquipmentBeam, true);
	}
	Pickup->PickupSphere->SetSphereRadius(Pickup->PickupRadius);
	Pickup->IconComponent->SetWidgetClass(Pickup->IconWidgetClass);
	const FVector2D Size = Pickup->IconSize;
	Pickup->IconComponent->SetDrawSize(!Size.ContainsNaN() && Size.X > 0.f && Size.Y > 0.f ? Size : FVector2D(32.f));
	// 表现初始化尽力完成；Widget或图标资源失败不能阻断物品生成和事务。
	if (FSlateApplication::IsInitialized() && World->IsGameWorld() && !IsRunningDedicatedServer())
	{
		Pickup->IconComponent->InitWidget();
		if (UWizardPickupIcon* Widget = Cast<UWizardPickupIcon>(Pickup->IconComponent->GetUserWidgetObject())) Widget->SetIcon(Icon);
	}

	return Pickup;
}

void AWizardWorldPickup::ActivatePickup()
{
	if (bActive || !HasAuthority() || RemainingQuantity <= 0) return;
	bActive = true;
	IconComponent->SetVisibility(true);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

bool AWizardWorldPickup::TransferTo(TFunctionRef<int32(int32)> Receiver)
{
	if (!bActive || bCollecting || !HasAuthority() || RemainingQuantity <= 0) return false;
	TGuardValue<bool> Guard(bCollecting, true);
	const int32 Accepted = Receiver(RemainingQuantity);
	checkf(Accepted >= 0 && Accepted <= RemainingQuantity, TEXT("接收方返回非法数量"));
	if (Accepted <= 0 || Accepted > RemainingQuantity) return false;
	RemainingQuantity -= Accepted;
	if (RemainingQuantity == 0)
	{
		bActive = false;
		Destroy();
	}
	return true;
}

bool AWizardWorldPickup::BelongsToChallenge(const AWizardChallengeArena* Challenge) const
{
	return Challenge && SourceChallenge.Get() == Challenge;
}

void AWizardWorldPickup::EndPlay(const EEndPlayReason::Type Reason)
{
	bActive = false;
	Super::EndPlay(Reason);
}
