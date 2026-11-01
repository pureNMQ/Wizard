// Wizard 项目：玩家界面共用属性名称，避免背包与商店文案分歧。
#pragma once

#include "CoreMinimal.h"

struct FGameplayAttribute;

namespace WizardAttributeDisplay
{
	// 返回属性中文名称；未收录的属性回退到引擎属性名。
	FString GetLabel(const FGameplayAttribute& Attribute);
}
