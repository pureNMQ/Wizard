#include "UI/WizardCombatAssetTools.h"
#if WITH_EDITOR
#include "UI/WizardHealthWidget.h"
#include "UI/WizardPauseMenu.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "CommonUITypes.h"
#include "CommonInputBaseTypes.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif

bool UWizardCombatAssetTools::BuildCombatAssets()
{
#if WITH_EDITOR
	auto Save = [](UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset,
			*FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	};
	auto MakeAction = [&](const TCHAR* Name, EInputActionValueType Type)
	{
		const FString Path = FString(TEXT("/Game/Wizard/Input/")) + Name;
		UInputAction* Action = LoadObject<UInputAction>(nullptr, *(Path + TEXT(".") + Name), nullptr, LOAD_NoWarn);
		if (!Action)
		{
			Action = NewObject<UInputAction>(CreatePackage(*Path), Name, RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Action);
		}
		Action->ValueType = Type;
		Save(Action);
		return Action;
	};
	UInputAction* Move = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	UInputAction* Attack = MakeAction(TEXT("IA_Attack"), EInputActionValueType::Boolean);
	UInputAction* Menu = MakeAction(TEXT("IA_Menu"), EInputActionValueType::Boolean);
	const TCHAR* ContextPath = TEXT("/Game/Wizard/Input/IMC_Combat");
	UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Wizard/Input/IMC_Combat.IMC_Combat"), nullptr, LOAD_NoWarn);
	if (!Context)
	{
		Context = NewObject<UInputMappingContext>(CreatePackage(ContextPath), TEXT("IMC_Combat"), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Context);
	}
	Context->UnmapAll();
	auto AxisKey = [&](FKey Key, bool bY, bool bNegate)
	{
		FEnhancedActionKeyMapping& Mapping = Context->MapKey(Move, Key);
		if (bNegate) Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
		if (bY)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Context);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			Mapping.Modifiers.Add(Swizzle);
		}
	};
	AxisKey(EKeys::W, true, false);
	AxisKey(EKeys::S, true, true);
	AxisKey(EKeys::D, false, false);
	AxisKey(EKeys::A, false, true);
	Context->MapKey(Attack, EKeys::LeftMouseButton);
	Context->MapKey(Menu, EKeys::Escape);
	Save(Context);

	auto MakeWidget = [&](const TCHAR* Name, UClass* Parent, bool bMenu)
	{
		const FString Path = FString(TEXT("/Game/Wizard/UI/")) + Name;
		// 不覆盖已有人手修改的布局；删除资产后才重新生成默认布局。
		if (LoadObject<UWidgetBlueprint>(nullptr, *(Path + TEXT(".") + Name), nullptr, LOAD_NoWarn)) return true;
		UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(Parent,
			CreatePackage(*Path), Name, BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
		if (!Blueprint) return false;
		UWidgetTree* Tree = Blueprint->WidgetTree;
		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
		Tree->RootWidget = Canvas;
		UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
		Border->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.06f, 0.94f));
		Border->SetPadding(FMargin(20.f));
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Border);
		Slot->SetPosition(bMenu ? FVector2D(400, 220) : FVector2D(28, 28));
		Slot->SetSize(bMenu ? FVector2D(500, 210) : FVector2D(350, 145));
		UVerticalBox* Box = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Contents"));
		Border->SetContent(Box);
		auto Label = [&](const TCHAR* LabelName, const TCHAR* Initial, int32 Size)
		{
			UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), LabelName);
			Text->SetText(FText::FromString(Initial));
			FSlateFontInfo Font = Text->GetFont();
			Font.Size = Size;
			Text->SetFont(Font);
			Box->AddChildToVerticalBox(Text);
		};
		if (bMenu)
		{
			Label(TEXT("Title"), TEXT("PAUSED"), 30);
			Label(TEXT("Instructions"), TEXT("Esc: return to combat\nRelease attack before resuming"), 20);
		}
		else
		{
			Canvas->SetVisibility(ESlateVisibility::HitTestInvisible);
			Label(TEXT("PlayerHealth"), TEXT("Player HP"), 22);
			Label(TEXT("TargetHealth"), TEXT("Target HP: aim at an enemy"), 22);
			Label(TEXT("Controls"), TEXT("WASD Move  |  LMB Attack  |  Esc Pause"), 14);
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		FAssetRegistryModule::AssetCreated(Blueprint);
		return Save(Blueprint);
	};
	const TCHAR* TablePath = TEXT("/Game/Wizard/UI/DT_UIActions");
	UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/Wizard/UI/DT_UIActions.DT_UIActions"), nullptr, LOAD_NoWarn);
	if (!Table)
	{
		Table = NewObject<UDataTable>(CreatePackage(TablePath), TEXT("DT_UIActions"), RF_Public | RF_Standalone);
		Table->RowStruct = FCommonInputActionDataBase::StaticStruct();
		Table->CreateTableFromJSONString(TEXT("[{\"Name\":\"Back\",\"DisplayName\":\"Back\",\"KeyboardInputTypeInfo\":{\"Key\":{\"KeyName\":\"Escape\"}}},{\"Name\":\"Click\",\"DisplayName\":\"Select\",\"KeyboardInputTypeInfo\":{\"Key\":{\"KeyName\":\"LeftMouseButton\"}}}]"));
		FAssetRegistryModule::AssetCreated(Table);
		Save(Table);
	}
	const TCHAR* InputPath = TEXT("/Game/Wizard/UI/BP_UIInputData");
	UBlueprint* InputBlueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Wizard/UI/BP_UIInputData.BP_UIInputData"), nullptr, LOAD_NoWarn);
	if (!InputBlueprint)
	{
		InputBlueprint = FKismetEditorUtilities::CreateBlueprint(UCommonUIInputData::StaticClass(), CreatePackage(InputPath), TEXT("BP_UIInputData"), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
		FKismetEditorUtilities::CompileBlueprint(InputBlueprint);
		UCommonUIInputData* Data = CastChecked<UCommonUIInputData>(InputBlueprint->GeneratedClass->GetDefaultObject());
		Data->DefaultBackAction.DataTable = Table;
		Data->DefaultBackAction.RowName = TEXT("Back");
		Data->DefaultClickAction.DataTable = Table;
		Data->DefaultClickAction.RowName = TEXT("Click");
		FAssetRegistryModule::AssetCreated(InputBlueprint);
		Save(InputBlueprint);
	}
	return MakeWidget(TEXT("WBP_CombatHUD"), UWizardHealthWidget::StaticClass(), false)
		&& MakeWidget(TEXT("WBP_PauseMenu"), UWizardPauseMenu::StaticClass(), true);
#else
	return false;
#endif
}
