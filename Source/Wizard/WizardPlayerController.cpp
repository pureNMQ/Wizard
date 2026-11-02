#include "WizardPlayerController.h"
#include "Combat/WizardCombatFeedbackComponent.h"
#include "WizardPlayerState.h"
#include "Save/WizardSaveComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/WizardPauseMenu.h"
#include "UI/WizardUIRoot.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


#include "GAS/WizardAttributeSet.h"
#include "UObject/ConstructorHelpers.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "UI/WizardSkillWidgets.h"
#include "UI/WizardInventoryWidgets.h"
#include "Interaction/WizardInteractionComponent.h"

AWizardPlayerController::AWizardPlayerController()
{
	CombatFeedback = CreateDefaultSubobject<UWizardCombatFeedbackComponent>(TEXT("CombatFeedback"));
	bShowMouseCursor = true;
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Context(TEXT("/Game/Wizard/Input/IMC_Combat"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveInput(TEXT("/Game/Wizard/Input/IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> AttackInput(TEXT("/Game/Wizard/Input/IA_Attack"));
	static ConstructorHelpers::FObjectFinder<UInputAction> MenuInput(TEXT("/Game/Wizard/Input/IA_Menu"));
	CombatMappingContext = Context.Object;
	MoveAction = MoveInput.Object;
	AttackAction = AttackInput.Object;
	MenuAction = MenuInput.Object;
	static ConstructorHelpers::FClassFinder<UUserWidget> HUDClass(TEXT("/Game/Wizard/UI/WBP_CombatHUD"));
	static ConstructorHelpers::FClassFinder<UWizardPauseMenu> MenuClass(TEXT("/Game/Wizard/UI/WBP_PauseMenu"));
	static ConstructorHelpers::FClassFinder<UWizardUIRoot> RootWidgetClass(TEXT("/Game/Wizard/UI/WBP_UIRoot"));
	RootClass = RootWidgetClass.Class;
	HealthWidgetClass = HUDClass.Class;
	PauseMenuClass = MenuClass.Class;
	static ConstructorHelpers::FClassFinder<UWizardSkillMenu> SkillsMenu(TEXT("/Game/Wizard/UI/WBP_SkillMenu"));
	SkillMenuClass = SkillsMenu.Class;
	InventoryMenuClass = LoadClass<UWizardInventoryMenu>(nullptr, TEXT("/Game/Wizard/UI/WBP_InventoryMenu.WBP_InventoryMenu_C"), nullptr, LOAD_NoWarn);
	InventoryAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Wizard/Input/IA_Inventory.IA_Inventory"), nullptr, LOAD_NoWarn);
	static ConstructorHelpers::FObjectFinder<UInputAction> InteractInput(TEXT("/Game/Wizard/Input/IA_Interact"));
	InteractAction = InteractInput.Object;
	const TCHAR* SkillPaths[] = { TEXT("/Game/Wizard/Input/IA_Skill1"), TEXT("/Game/Wizard/Input/IA_Skill2"), TEXT("/Game/Wizard/Input/IA_Skill3"), TEXT("/Game/Wizard/Input/IA_Skill4") };
	for (const TCHAR* Path : SkillPaths)
	{
		ConstructorHelpers::FObjectFinder<UInputAction> SkillInput(Path);
		SkillActions.Add(SkillInput.Object);
	}
}

void AWizardPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (CombatMappingContext) Subsystem->AddMappingContext(CombatMappingContext, 0);
	}
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	if (RootClass)
	{
		UIRoot = CreateWidget<UWizardUIRoot>(this, RootClass);
		if (UIRoot) UIRoot->AddToViewport();
	}
	if (UIRoot && HealthWidgetClass)
	{
		HealthWidget = CreateWidget<UUserWidget>(this, HealthWidgetClass);
		UIRoot->SetHUD(HealthWidget);
	}
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>())
	{
		SaveStatusHandle = State->GetSaveService()->OnStatusChanged.AddUObject(this, &ThisClass::ShowSaveNotice);
		ShowSaveNotice();
	}
}

void AWizardPlayerController::ShowSaveNotice()
{
	const AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>();
	if (!UIRoot || !State) return;
	const FText Message = State->GetSaveService()->GetNotice();
	if (!Message.IsEmpty()) UIRoot->ShowChallengeAnnouncement(Message, FLinearColor(1.f, .3f, .15f), 10.f);
}

void AWizardPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction) Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
		if (AttackAction)
		{
			Input->BindAction(AttackAction, ETriggerEvent::Started, this, &ThisClass::StartAttack);
			Input->BindAction(AttackAction, ETriggerEvent::Completed, this, &ThisClass::ReleaseAttack);
			Input->BindAction(AttackAction, ETriggerEvent::Canceled, this, &ThisClass::StopAttack);
		}
		if (MenuAction) Input->BindAction(MenuAction, ETriggerEvent::Started, this, &ThisClass::OpenMenu);
		for (int32 Slot = 0; Slot < SkillActions.Num(); ++Slot)
		{
			if (!SkillActions[Slot]) continue;
			Input->BindAction(SkillActions[Slot], ETriggerEvent::Started, this, &ThisClass::UseSkill, Slot);
			Input->BindAction(SkillActions[Slot], ETriggerEvent::Canceled, this, &ThisClass::CancelSkillInput);
		}
		if (InventoryAction) Input->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
		if (InteractAction) Input->BindAction(InteractAction, ETriggerEvent::Started, this, &ThisClass::Interact);
	}
}

bool AWizardPlayerController::CanUseWorldInput() const
{
	const AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>();
	return !bMenuOpen && !IsPaused() && GetPawn() && State
		&& State->GetAttributes()->GetHealth() > 0.f;
}

void AWizardPlayerController::Move(const FInputActionValue& Value)
{
	if (!CanUseWorldInput()) return;
	const FVector2D Axis = Value.Get<FVector2D>();
	// 相机固定在世界 X 方向；W 为屏幕向上，D 为屏幕向右。
	GetPawn()->AddMovementInput(FVector::ForwardVector, Axis.Y);
	GetPawn()->AddMovementInput(FVector::RightVector, Axis.X);
}

void AWizardPlayerController::StartAttack()
{
	if (CanUseWorldInput())
	{
		UpdateAim();
		GetPlayerState<AWizardPlayerState>()->StartBasicAttack();
	}
}
void AWizardPlayerController::ReleaseAttack()
{
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>()) State->ReleaseBasicAttack();
}
void AWizardPlayerController::StopAttack()
{
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>()) State->StopBasicAttack();
	CancelSkillInput();
}
void AWizardPlayerController::CancelSkillInput()
{
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>()) State->GetSkills()->CancelCasting();
}
void AWizardPlayerController::UseSkill(int32 Slot)
{
	if (!CanUseWorldInput()) return;
	UpdateAim();
	OnSkillFeedback.Broadcast(GetPlayerState<AWizardPlayerState>()->GetSkills()->ActivateSlot(Slot));
}
void AWizardPlayerController::Interact()
{
	if (CanUseWorldInput()) GetPlayerState<AWizardPlayerState>()->GetInteraction()->TryInteract();
}
bool AWizardPlayerController::OpenSkillMenu()
{
	return CanUseWorldInput() && UIRoot && SkillMenuClass && UIRoot->PushMenu(SkillMenuClass);
}
bool AWizardPlayerController::OpenInventory()
{
	return CanUseWorldInput() && UIRoot && InventoryMenuClass && UIRoot->PushMenu(InventoryMenuClass);
}
void AWizardPlayerController::ToggleInventory()
{
	// 菜单内同一动作交给CommonUI路由；世界输入只负责打开。
	if (!bMenuOpen) OpenInventory();
}
void AWizardPlayerController::PlayerTick(float DeltaSeconds)
{
	Super::PlayerTick(DeltaSeconds);
	if (CanUseWorldInput()) UpdateAim();
	if (!CanUseWorldInput()) StopAttack();
}
void AWizardPlayerController::UpdateAim()
{
	if (!GetPawn()) return;
	// 以角色高度的平面反投射，避免光标落在高物体时产生竖直射击。
	FVector Origin, Direction;
	if (DeprojectMousePositionToWorld(Origin, Direction) && !FMath::IsNearlyZero(Direction.Z))
	{
		const float Distance = (GetPawn()->GetActorLocation().Z - Origin.Z) / Direction.Z;
		if (Distance > 0.f)
		{
			const FVector Aim = Origin + Direction * Distance;
			const FVector Facing = Aim - GetPawn()->GetActorLocation();
			if (!Facing.IsNearlyZero()) GetPawn()->SetActorRotation(FRotator(0, Facing.Rotation().Yaw, 0));
			GetPlayerState<AWizardPlayerState>()->SetAimPoint(Aim);
		}
	}
}
void AWizardPlayerController::OpenMenu()
{
	if (!UIRoot || bMenuOpen || !PauseMenuClass) return;
	UIRoot->PushModal(PauseMenuClass);
}
void AWizardPlayerController::SetMenuOpen(bool bOpen)
{
	bLegacyMenuOpen = bOpen;
	RefreshMenuState();
}
void AWizardPlayerController::SetRootScreensOpen(bool bOpen)
{
	bRootScreensOpen = bOpen;
	RefreshMenuState();
}
void AWizardPlayerController::RefreshMenuState()
{
	const bool bOpen = bLegacyMenuOpen || bRootScreensOpen;
	if (bMenuOpen == bOpen) return;
	StopAttack();
	bMenuOpen = bOpen;
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>()) State->GetInteraction()->SetSuppressed(bOpen);
	FlushPressedKeys();
	if (ACharacter* ControlledCharacter = Cast<ACharacter>(GetPawn())) ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
	if (bOpen)
	{
		bWasPaused = IsPaused();
		CombatFeedback->ResetFeedback();
		SetPause(true);
	}
	else
	{
		SetPause(bWasPaused);
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			// 等按键松开后才重新处理，防止菜单关闭的点击恢复连射。
			FModifyContextOptions Options;
			Options.bIgnoreAllPressedKeysUntilRelease = true;
			Subsystem->RequestRebuildControlMappings(Options);
		}
	}
}
void AWizardPlayerController::OnPossess(APawn* InPawn)
{
	StopAttack();
	Super::OnPossess(InPawn);
}
void AWizardPlayerController::OnUnPossess()
{
	CombatFeedback->ResetFeedback();
	StopAttack();
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>()) State->GetInteraction()->BindAvatar(nullptr);
	Super::OnUnPossess();
}
void AWizardPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (AWizardPlayerState* State = GetPlayerState<AWizardPlayerState>())
	{
		State->GetSaveService()->OnStatusChanged.Remove(SaveStatusHandle);
	}
	StopAttack();
	bLegacyMenuOpen = false;
	if (UIRoot) UIRoot->RemoveFromParent();
	SetRootScreensOpen(false);
	UIRoot = nullptr;
	if (HealthWidget) HealthWidget->RemoveFromParent();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (CombatMappingContext) Subsystem->RemoveMappingContext(CombatMappingContext);
	}
	Super::EndPlay(Reason);
}
