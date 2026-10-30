#include "UI/WizardChallengePresentationTools.h"
#if WITH_EDITOR
#include "UI/WizardChallengeBanner.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

bool UWizardChallengePresentationTools::BuildChallengeBannerAsset()
{
#if WITH_EDITOR
	const TCHAR* Path = TEXT("/Game/Wizard/UI/WBP_ChallengeBanner");
	if (LoadObject<UWidgetBlueprint>(nullptr, Path, nullptr, LOAD_NoWarn)) return true;
	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(UWizardChallengeBanner::StaticClass(),
		CreatePackage(Path), TEXT("WBP_ChallengeBanner"), BPTYPE_Normal, UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass()));
	if (!Blueprint) return false;
	UWidgetTree* Tree = Blueprint->WidgetTree;
	UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	Tree->RootWidget = Canvas;
	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AnnouncementPanel"));
	Panel->SetBrushColor(FLinearColor(.012f, .025f, .05f, .78f));
	Panel->SetPadding(FMargin(28.f, 18.f));
	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(Panel);
	PanelSlot->SetAnchors(FAnchors(.5f, .28f));
	PanelSlot->SetAlignment(FVector2D(.5f, .5f));
	PanelSlot->SetPosition(FVector2D::ZeroVector);
	PanelSlot->SetSize(FVector2D(640.f, 150.f));
	UVerticalBox* Contents = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Contents"));
	Panel->SetContent(Contents);
	auto AddText = [&](const TCHAR* Name, int32 FontSize)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->bIsVariable = true;
		Text->SetJustification(ETextJustify::Center);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(.86f, .93f, 1.f)));
		Text->SetShadowOffset(FVector2D(2.f, 2.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .9f));
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Text->SetFont(Font);
		Contents->AddChildToVerticalBox(Text)->SetPadding(FMargin(0.f, 3.f));
		return Text;
	};
	AddText(TEXT("AnnouncementText"), 48)->SetText(FText::FromString(TEXT("挑战开始")));
	AddText(TEXT("DetailText"), 20)->SetText(FText::FromString(TEXT("清除所有敌人")));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error) return false;
	FAssetRegistryModule::AssetCreated(Blueprint);
	Blueprint->MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Blueprint->GetOutermost(), Blueprint,
		*FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
#else
	return false;
#endif
}
