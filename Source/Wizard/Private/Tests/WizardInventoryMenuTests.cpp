// Wizard 项目：不依赖Slate的菜单会话边界、旧数量请求及订阅清理验证。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/WizardInventoryMenuController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInventoryMenuSessionTest, "Wizard.Inventory.MenuSession", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInventoryMenuSessionTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Model = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* Ore = NewObject<UWizardItemDefinition>(Model);
	Ore->ItemId = TEXT("Ore");
	Ore->MaxStack = 10;
	int32 Accepted = 0;
	Model->AddItems(Ore, 8, Accepted);
	UWizardInventoryMenuController* Menu = NewObject<UWizardInventoryMenuController>();
	TestTrue(TEXT("建立菜单会话"), Menu->Open(Model));
	int32 Changed = 0;
	Menu->OnChanged.AddLambda([&Changed]() { ++Changed; });
	FWizardInventoryDragRequest OldDrag;
	TestTrue(TEXT("创建有效拖拽"), Menu->MakeDragRequest(0, OldDrag));
	Menu->Select(0);
	TestEqual(TEXT("选择读取真实数量"), Menu->GetSelectedEntry().Quantity, 8);
	Model->AddItems(Ore, 1, Accepted);
	TestTrue(TEXT("过期数量选择不展示为有效物品"), Menu->GetSelectedEntry().IsEmpty());
	EWizardInventoryResult Feedback = EWizardInventoryResult::Success;
	Menu->OnFeedback.AddLambda([&Feedback](EWizardInventoryResult Result) { Feedback = Result; });
	const auto Before = Model->GetSnapshot();
	Menu->SplitSelected(2);
	TestEqual(TEXT("延迟数量请求拒旧"), Feedback, EWizardInventoryResult::StaleRequest);
	TestTrue(TEXT("旧数量请求不修改状态"), Before.Slots == Model->GetSnapshot().Slots);
	TestTrue(TEXT("当前会话消费旧拖拽但报告过期"), Menu->HandleDrop(OldDrag, 1));
	TestEqual(TEXT("旧拖拽版本反馈"), Feedback, EWizardInventoryResult::StaleRequest);
	Menu->Close();
	const int32 ClosedEvents = Changed;
	Model->AddItems(Ore, 1, Accepted);
	TestEqual(TEXT("关闭后解绑模型事件"), Changed, ClosedEvents);
	TestFalse(TEXT("关闭会话拒绝拖拽"), Menu->HandleDrop(OldDrag, 1));
	TestTrue(TEXT("同对象可重开"), Menu->Open(Model));
	TestFalse(TEXT("重开会话拒绝旧SessionId"), Menu->HandleDrop(OldDrag, 1));
	FWizardInventoryDragRequest CurrentDrag;
	Menu->MakeDragRequest(0, CurrentDrag);
	UWizardInventoryMenuController* Other = NewObject<UWizardInventoryMenuController>();
	Other->Open(Model);
	TestFalse(TEXT("其他菜单不能消费来源会话"), Other->HandleDrop(CurrentDrag, 1));
	TestTrue(TEXT("新会话拖拽成功"), Menu->HandleDrop(CurrentDrag, 1));
	TestEqual(TEXT("移动后数量保留"), Model->GetSnapshot().Slots[1].Quantity, 10);
	Other->Close();
	Menu->Close();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInventoryQuantityRequestTest, "Wizard.Inventory.FrozenQuantityRequest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInventoryQuantityRequestTest::RunTest(const FString& Parameters)
{
	UWizardInventoryComponent* Model = NewObject<UWizardInventoryComponent>();
	UWizardItemDefinition* Ore = NewObject<UWizardItemDefinition>(Model);
	Ore->ItemId = TEXT("Ore");
	Ore->MaxStack = 10;
	int32 Accepted = 0;
	Model->AddItems(Ore, 16, Accepted);
	UWizardInventoryMenuController* Menu = NewObject<UWizardInventoryMenuController>();
	Menu->Open(Model);
	EWizardInventoryResult Feedback = EWizardInventoryResult::Success;
	Menu->OnFeedback.AddLambda([&Feedback](EWizardInventoryResult Result) { Feedback = Result; });
	Menu->Select(0);
	FWizardInventoryQuantityRequest Split;
	TestTrue(TEXT("打开拆分窗口冻结请求"), Menu->PrepareQuantity(false, Split));
	Menu->Select(1);
	Menu->SubmitQuantity(Split, 3);
	TestEqual(TEXT("选择其他格后仍拆分原物品"), Model->GetSnapshot().Slots[0].Quantity, 7);
	TestEqual(TEXT("新选中格不受影响"), Model->GetSnapshot().Slots[1].Quantity, 6);
	TestEqual(TEXT("拆出新堆叠"), Model->GetSnapshot().Slots[2].Quantity, 3);
	TestEqual(TEXT("拆分反馈成功"), Feedback, EWizardInventoryResult::Success);
	const auto AfterSplit = Model->GetSnapshot();
	Menu->SubmitQuantity(Split, 1);
	TestEqual(TEXT("成功请求不能重复提交"), Feedback, EWizardInventoryResult::StaleRequest);
	TestTrue(TEXT("重放失败保持物品状态"), AfterSplit.Slots == Model->GetSnapshot().Slots);
	Menu->Select(0);
	FWizardInventoryQuantityRequest Stale;
	Menu->PrepareQuantity(false, Stale);
	Model->AddItems(Ore, 1, Accepted);
	const auto AfterExternalChange = Model->GetSnapshot();
	Menu->SubmitQuantity(Stale, 2);
	TestEqual(TEXT("外部变更后冻结令牌过期"), Feedback, EWizardInventoryResult::StaleRequest);
	TestTrue(TEXT("过期请求不改变库存"), AfterExternalChange.Slots == Model->GetSnapshot().Slots);
	Menu->Select(0);
	FWizardInventoryQuantityRequest PreviousSession;
	Menu->PrepareQuantity(false, PreviousSession);
	Menu->Close();
	Menu->Open(Model);
	Menu->OnFeedback.AddLambda([&Feedback](EWizardInventoryResult Result) { Feedback = Result; });
	Menu->Select(1);
	Menu->SubmitQuantity(PreviousSession, 2);
	TestEqual(TEXT("重开后旧会话令牌失效"), Feedback, EWizardInventoryResult::StaleRequest);
	TestTrue(TEXT("旧会话失败不修改库存"), AfterExternalChange.Slots == Model->GetSnapshot().Slots);
	FWizardInventoryQuantityRequest InvalidQuantity;
	Menu->PrepareQuantity(false, InvalidQuantity);
	Menu->SubmitQuantity(InvalidQuantity, 0);
	TestEqual(TEXT("非法数量由业务拒绝"), Feedback, EWizardInventoryResult::InvalidQuantity);
	TestTrue(TEXT("非法数量不改背包"), AfterExternalChange.Slots == Model->GetSnapshot().Slots);
	UWizardInventoryMenuController* Other = NewObject<UWizardInventoryMenuController>();
	Other->Open(Model);
	Other->OnFeedback.AddLambda([&Feedback](EWizardInventoryResult Result) { Feedback = Result; });
	Other->SubmitQuantity(InvalidQuantity, 1);
	TestEqual(TEXT("其他控制器拒绝消费令牌"), Feedback, EWizardInventoryResult::StaleRequest);
	TestTrue(TEXT("跨会话失败不变更库存"), AfterExternalChange.Slots == Model->GetSnapshot().Slots);
	Other->Close();
	Menu->Close();
	return true;
}
#endif
