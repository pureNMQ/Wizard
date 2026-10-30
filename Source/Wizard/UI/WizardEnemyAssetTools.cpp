#include "UI/WizardEnemyAssetTools.h"
#if WITH_EDITOR
#include "UI/WizardEnemyHealthBar.h"
#include "Combat/WizardEnemyAnimInstance.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/Skeleton.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_Slot.h"
#include "AnimationGraphSchema.h"
#include "Factories/AnimBlueprintFactory.h"
#include "K2Node_VariableGet.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

namespace
{
	bool SaveEnemyAsset(UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset,
			*FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	}

	bool BuildEnemyAnimations(const FString& Kind)
	{
		const FString Source = TEXT("/Game/MonsterForSurvivalGame/Animation/Polyart/") + Kind + TEXT("/") + Kind;
		USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, *(TEXT("/Game/MonsterForSurvivalGame/Mesh/Polyart/") + Kind + TEXT("_Skeleton." ) + Kind + TEXT("_Skeleton")));
		UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, *(Source + TEXT("_IdleBattle_ANIM." ) + Kind + TEXT("_IdleBattle_ANIM")));
		UAnimSequence* Run = LoadObject<UAnimSequence>(nullptr, *(Source + TEXT("_Run_ANIM." ) + Kind + TEXT("_Run_ANIM")));
		UAnimSequence* Attack = LoadObject<UAnimSequence>(nullptr, *(Source + TEXT("_Attack01_ANIM." ) + Kind + TEXT("_Attack01_ANIM")));
		if (!Skeleton || !Idle || !Run || !Attack || Idle->GetSkeleton() != Skeleton || Run->GetSkeleton() != Skeleton || Attack->GetSkeleton() != Skeleton) return false;
		const FString Folder = TEXT("/Game/Wizard/Enemies/Animation/");
		const FString BlendName = TEXT("BS_") + Kind;
		UBlendSpace1D* Blend = LoadObject<UBlendSpace1D>(nullptr, *(Folder + BlendName + TEXT(".") + BlendName), nullptr, LOAD_NoWarn);
		if (!Blend)
		{
			Blend = NewObject<UBlendSpace1D>(CreatePackage(*(Folder + BlendName)), *BlendName, RF_Public | RF_Standalone);
			Blend->SetSkeleton(Skeleton);
			// 轴定义为资源配置，运行时由动画蓝图读取角色速度。
			FBlendParameter& Axis = const_cast<FBlendParameter&>(Blend->GetBlendParameter(0));
			Axis.DisplayName = TEXT("Speed");
			Axis.Min = 0.f;
			Axis.Max = 400.f;
			Axis.GridNum = 4;
			Blend->AddSample(Idle, FVector::ZeroVector);
			Blend->AddSample(Run, FVector(400.f, 0.f, 0.f));
			Blend->ValidateSampleData();
			Blend->ResampleData();
			FAssetRegistryModule::AssetCreated(Blend);
			if (!SaveEnemyAsset(Blend)) return false;
		}
		const FString MontageName = TEXT("AM_") + Kind + TEXT("Attack");
		if (!LoadObject<UAnimMontage>(nullptr, *(Folder + MontageName + TEXT(".") + MontageName), nullptr, LOAD_NoWarn))
		{
			UAnimMontage* Temp = UAnimMontage::CreateSlotAnimationAsDynamicMontage(Attack, TEXT("DefaultSlot"), 0.08f, 0.12f);
			if (!Temp) return false;
			UAnimMontage* Montage = DuplicateObject<UAnimMontage>(Temp, CreatePackage(*(Folder + MontageName)), *MontageName);
			Montage->ClearFlags(RF_Transient);
			Montage->SetFlags(RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Montage);
			if (!SaveEnemyAsset(Montage)) return false;
		}
		const FString BlueprintName = TEXT("ABP_") + Kind;
		if (LoadObject<UAnimBlueprint>(nullptr, *(Folder + BlueprintName + TEXT(".") + BlueprintName), nullptr, LOAD_NoWarn)) return true;
		UAnimBlueprintFactory* Factory = NewObject<UAnimBlueprintFactory>();
		Factory->ParentClass = UWizardEnemyAnimInstance::StaticClass();
		Factory->TargetSkeleton = Skeleton;
		UAnimBlueprint* Blueprint = Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(),
			CreatePackage(*(Folder + BlueprintName)), *BlueprintName, RF_Public | RF_Standalone, nullptr, GWarn));
		if (!Blueprint) return false;
		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		UEdGraph* AnimGraph = nullptr;
		UAnimGraphNode_Root* Root = nullptr;
		for (UEdGraph* Graph : Graphs)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UAnimGraphNode_Root* Candidate = Cast<UAnimGraphNode_Root>(Node)) { AnimGraph = Graph; Root = Candidate; break; }
			}
			if (Root) break;
		}
		if (!AnimGraph || !Root) return false;
		auto AddNode = [AnimGraph](UEdGraphNode* Node, int32 X)
		{
			AnimGraph->AddNode(Node, false, false);
			Node->CreateNewGuid();
			Node->PostPlacedNewNode();
			Node->AllocateDefaultPins();
			Node->NodePosX = X;
		};
		UAnimGraphNode_BlendSpacePlayer* Player = NewObject<UAnimGraphNode_BlendSpacePlayer>(AnimGraph);
		Player->Node.SetBlendSpace(Blend);
		AddNode(Player, -500);
		for (FOptionalPinFromProperty& Property : Player->ShowPinForProperties)
		{
			if (Property.PropertyName == TEXT("X")) Property.bShowPin = true;
		}
		Player->ReconstructNode();
		UK2Node_VariableGet* Speed = NewObject<UK2Node_VariableGet>(AnimGraph);
		Speed->VariableReference.SetSelfMember(TEXT("Speed"));
		AddNode(Speed, -800);
		UAnimGraphNode_Slot* Slot = NewObject<UAnimGraphNode_Slot>(AnimGraph);
		Slot->Node.SlotName = TEXT("DefaultSlot");
		AddNode(Slot, -220);
		const UEdGraphSchema* Schema = AnimGraph->GetSchema();
		auto Output = [](UEdGraphNode* Node) -> UEdGraphPin*
		{
			for (UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == EGPD_Output) return Pin;
			return nullptr;
		};
		if (!Schema->TryCreateConnection(Output(Speed), Player->FindPin(TEXT("X"))) ||
			!Schema->TryCreateConnection(Output(Player), Slot->FindPin(TEXT("Source"))) ||
			!Schema->TryCreateConnection(Output(Slot), Root->FindPin(TEXT("Result")))) return false;
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error) return false;
		FAssetRegistryModule::AssetCreated(Blueprint);
		return SaveEnemyAsset(Blueprint);
	}
}
#endif

bool UWizardEnemyAssetTools::BuildHealthBarAsset()
{
#if WITH_EDITOR
	const TCHAR* Path = TEXT("/Game/Wizard/UI/WBP_EnemyHealthBar");
	if (LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/Wizard/UI/WBP_EnemyHealthBar.WBP_EnemyHealthBar"), nullptr, LOAD_NoWarn)) return true;
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UWizardEnemyHealthBar::StaticClass(),
		CreatePackage(Path), TEXT("WBP_EnemyHealthBar"), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	if (!Blueprint) return false;
	UWidgetTree* Tree = Blueprint->WidgetTree;
	USizeBox* Size = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Frame"));
	Tree->RootWidget = Size;
	Size->SetWidthOverride(88.f);
	Size->SetHeightOverride(10.f);
	UBorder* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Outline"));
	Border->SetBrushColor(FLinearColor(0.015f, 0.008f, 0.008f, 0.95f));
	Border->SetPadding(FMargin(2.f));
	Size->SetContent(Border);
	UProgressBar* Bar = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
	Bar->SetFillColorAndOpacity(FLinearColor(0.85f, 0.08f, 0.04f, 1.f));
	Bar->SetPercent(1.f);
	Border->SetContent(Bar);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error) return false;
	FAssetRegistryModule::AssetCreated(Blueprint);
	return SaveEnemyAsset(Blueprint);
#else
	return false;
#endif
}

bool UWizardEnemyAssetTools::BuildEnemyAnimationAssets()
{
#if WITH_EDITOR
	return BuildEnemyAnimations(TEXT("Slime")) && BuildEnemyAnimations(TEXT("Beholder"));
#else
	return false;
#endif
}
