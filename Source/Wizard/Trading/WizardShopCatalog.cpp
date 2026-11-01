#include "Trading/WizardShopCatalog.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UWizardShopCatalog::IsConfigurationValid() const
{
	if (!bInfiniteStock || Products.IsEmpty()) return false;
	TSet<FName> Ids;
	for (const FWizardShopProduct& Product : Products)
	{
		const UWizardItemDefinition* Definition = Product.Definition.Get();
		if (!IsValid(Definition) || !Definition->IsConfigurationValid() || !Definition->bTradable || Product.BuyPrice <= 0
			|| Definition->Type == EWizardItemType::Material || Ids.Contains(Definition->ItemId)) return false;
		Ids.Add(Definition->ItemId);
	}
	return true;
}

#if WITH_EDITOR
EDataValidationResult UWizardShopCatalog::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (!IsConfigurationValid())
	{
		Context.AddError(FText::FromString(TEXT("商店目录需要无限库存和非空商品；商品定义须合法、可交易且为装备或药水，ID不重复、购买价格为正整数。")));
		return EDataValidationResult::Invalid;
	}
	return Parent == EDataValidationResult::Invalid ? Parent : EDataValidationResult::Valid;
}
#endif
