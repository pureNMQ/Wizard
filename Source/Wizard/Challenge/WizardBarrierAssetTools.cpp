#include "Challenge/WizardBarrierAssetTools.h"
#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

bool UWizardBarrierAssetTools::BuildBarrierMaterial()
{
#if WITH_EDITOR
	const TCHAR* Path = TEXT("/Game/Wizard/Materials/M_ArenaBarrier");
	if (LoadObject<UMaterial>(nullptr, Path, nullptr, LOAD_NoWarn)) return true;
	UMaterial* Material = NewObject<UMaterial>(CreatePackage(Path), TEXT("M_ArenaBarrier"), RF_Public | RF_Standalone);
	Material->BlendMode = BLEND_Additive;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	bool bNeedsRecompile = false;
	Material->SetMaterialUsage(bNeedsRecompile, MATUSAGE_InstancedStaticMeshes);
	auto AddExpression = [Material](UMaterialExpression* Expression, int32 X, int32 Y)
	{
		Material->GetExpressionCollection().AddExpression(Expression);
		Expression->MaterialExpressionEditorX = X;
		Expression->MaterialExpressionEditorY = Y;
	};
	UMaterialExpressionWorldPosition* Position = NewObject<UMaterialExpressionWorldPosition>(Material);
	AddExpression(Position, -800, -200);
	UMaterialExpressionTime* Time = NewObject<UMaterialExpressionTime>(Material);
	AddExpression(Time, -800, 0);
	UMaterialExpressionScalarParameter* Base = NewObject<UMaterialExpressionScalarParameter>(Material);
	Base->ParameterName = TEXT("BaseHeight");
	Base->DefaultValue = 5.f;
	AddExpression(Base, -800, 150);
	UMaterialExpressionScalarParameter* Height = NewObject<UMaterialExpressionScalarParameter>(Material);
	Height->ParameterName = TEXT("Height");
	Height->DefaultValue = 350.f;
	AddExpression(Height, -800, 300);
	UMaterialExpressionCustom* Glow = NewObject<UMaterialExpressionCustom>(Material);
	Glow->OutputType = CMOT_Float3;
	Glow->Description = TEXT("结界：底沿辉光、缓慢流纹与顶部淡出");
	Glow->Inputs.Empty();
	auto Input = [Glow](const TCHAR* Name, UMaterialExpression* Expression)
	{
		FCustomInput CustomInput;
		CustomInput.InputName = Name;
		CustomInput.Input.Expression = Expression;
		Glow->Inputs.Add(CustomInput);
	};
	Input(TEXT("P"), Position);
	Input(TEXT("T"), Time);
	Input(TEXT("BaseHeight"), Base);
	Input(TEXT("Height"), Height);
	// 在材质中随时间计算，避免对64片结界逐帧更新CPU参数。
	Glow->Code = TEXT("float h = saturate((P.z - BaseHeight) / max(Height, 1.0));\n")
		TEXT("float fade = 1.0 - smoothstep(0.28, 1.0, h);\n")
		TEXT("float base = exp(-h * 48.0);\n")
		TEXT("float wave = pow(saturate(sin(h * 32.0 - T * 1.4)), 12.0);\n")
		TEXT("float strands = pow(saturate(sin((P.x + P.y) * 0.04 + h * 8.0 - T * 0.8)), 20.0);\n")
		TEXT("float pulse = 0.9 + 0.1 * sin(T * 1.3);\n")
		TEXT("return float3(0.08, 0.65, 1.0) * (0.08 + wave * 0.22 + strands * 0.25 + base * 1.6) * fade * pulse * 2.5;");
	AddExpression(Glow, -350, 0);
	UMaterialExpressionConstant* Opacity = NewObject<UMaterialExpressionConstant>(Material);
	Opacity->R = .5f;
	AddExpression(Opacity, -350, 400);
	Material->GetEditorOnlyData()->EmissiveColor.Expression = Glow;
	Material->GetEditorOnlyData()->Opacity.Expression = Opacity;
	Material->PostEditChange();
	Material->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Material);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Material->GetOutermost(), Material,
		*FPackageName::LongPackageNameToFilename(Path, FPackageName::GetAssetPackageExtension()), Args);
#else
	return false;
#endif
}
