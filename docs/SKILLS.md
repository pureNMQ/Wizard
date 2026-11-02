# 技能与搭配

T03 规则见[规格 R03～R08](../.scratch/wizard-demo/spec.md)，验收状态与证据见[T03票据](../.scratch/wizard-demo/issues/03-skills-and-loadout.md)。本文描述当前工程职责，不另建玩法权威来源。

## 状态与执行

- PlayerState 持有 ASC、属性集和 SkillLoadoutComponent；Character 作为 Avatar。六技能通过定义的 SourceObject 授予，重复初始化不会重复授予。初始四槽由目录配置。
- SkillDefinition 是只读定义，SkillCatalog 显式引用六份定义；定义再引用图标、Montage、Niagara/Cascade和声音。运行时修改 Spec 与实例，不改 CDO。当前使用强资源引用，由可达目录保障 Cook，无需另设 PrimaryAsset 扫描。
- 输入只提交槽位；组件先检查资源、冷却、状态与目标，再停止普攻并请求 ASC 激活。Ability 再验证并 Commit；消耗与冷却通过 GE，冷却 Tag 独立于槽位。出手由原生 Notify → GameplayEvent → AbilityTask 推进，每次只消费一次。
- 范围伤害统一由 `WizardSkills::ApplyAreaDamage` 结算：范围与存活条件决定目标，敌人不互相遮挡，场景障碍仍通过 Visibility 查询阻挡；每个目标只结算一次。
- Projectile/Area/Blink/Shield/Heal 是已有行为类型；新增同机制配置通过新增定义及目录引用完成，新增机制才扩展 C++。伤害复用攻防规则，以 Spec 的倍率调整攻击力。
- 配置扩展时为新技能准备唯一ID和冷却Tag（通过项目GameplayTags配置注册即可，无需改业务分支），复制同机制定义并加入目录；修改数值、图标、表现引用及初始搭配。编辑资产和项目配置仍使用UE-MCP，之后验证Cook与独立包加载。
- 护盾数值使用属性，限时 GE 的 Handle 管理所有权。到期、吸收耗尽、死亡及 Avatar 替换移除该效果并清空护盾。减速由限时 GE 管理，实际移动组件监听属性变化。
- GameplayCue 管命中/范围/治疗表现及护盾持续表现；粒子有结束路径，核心伤害不依赖 Cue 是否可播放。弹丸拖尾随 Actor 销毁，不脱离 Actor 无限残留。

## 编辑与持久化边界

- AlchemyStation 显式引用当前地图已有的 Camp_Alchemy，营地盒体和交互距离可配置。范围/目标选择/提示属于[通用事件交互模块](INTERACTION.md)，Alchemy只接入营地条件和打开技能页；打开页面和每次编辑重新校验权限。
- 炼金台页面采用 MVC：Model 为 PlayerState 的 `UWizardSkillLoadoutComponent`，唯一持有四槽状态、营地权限和事务规则；Controller 为 `UWizardSkillMenuController`，负责拖拽会话、来源身份检查、命令派发、候选/槽位显示数据和模型变更订阅；View 为 `UWizardSkillMenu` 与 Tile/Tooltip/DragPreview，负责布局、显示和转发指针输入，不直接修改或订阅 Model。
- 页面每次激活创建独立 Controller，关闭时解除模型订阅并清空 Controller；拖拽请求携带弱会话身份、稳定技能 ID 和来源槽位。旧页面/关闭后/来源技能已变的请求拒绝。Controller 通过 EquipSkill/SwapSlots/UnequipSkill 调用 Model，每次提交仍由 Model 重验营地条件。有效变化刷新显示，业务失败只反馈原因。
- 原页面中的业务 HandleDrop/CanDragSkill、模型弱引用/事件句柄及 Drag.SourceMenu 已删除，没有保留两套业务路径；现有 Widget Blueprint 类、BindWidget 名称和布局继续复用。
- GetSnapshot/RestoreSnapshot 为 T08 提供四槽 ID 快照；RestoreSnapshot 是读档接缝，不作为界面编辑入口。有效变更才广播 OnLoadoutChanged，磁盘保存尚由 T08 接入。
- SkillMenu 复用现有 Menu 层；SkillBar 是独立 HUD 模块，聚合 HUD 只配置布局。没有增加 UI 层。冷却显示读取实际 GE 剩余时间。
- 技能列表位于页面上部ScrollBox，每行六格UniformGrid，扩展技能继续按行排列；上方显示“炼金台”。已装备候选压暗且在按下/开始拖拽时都拒绝发起拖拽，悬浮详情仍可查看。下方装备列表居中靠下，保留技能名和1～4按键；四槽均可留空。详情由`WBP_SkillTooltip`读取定义显示，拖拽预览`WBP_SkillDragPreview`只包含图标。
- 装备槽拖回技能列表（含滚动区空白）调用`UnequipSkill`，拖回其他空白或取消保持原样；拖入空槽装备，槽间交换支持空槽。按下空槽不会消耗/冷却/打断普攻。快照仍固定四个元素，`NAME_None`表示空槽、可重复；未知非空ID和重复非空技能仍拒绝。卸下保留能力授予与冷却，重绑Avatar不填回已保存的全空搭配。

## 验证入口

```powershell
pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action BuildEditor
pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Test
pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Package
pwsh -NoProfile -File Tools/Build/Invoke-Wizard.ps1 -Action Smoke -SkillSmoke -PackageRoot 'Saved/Packages/<本轮目录>'
```

开发控制台 `Wizard.SkillsSmokeTest` 使用真实地图、蒙太奇和碰撞验证六技能并清理临时对象；独立包完成后退出，PIE保留编辑器。该命令不模拟物理按键，不替代1234、E、Esc与拖拽的实际操作验证。测试会暂时调整运行时搭配/资源数值，不写配置资产或存档。

## T12 表现配置迁移

六技能优先引用独立Presentation，阶段粒子、声音与冲击等级由其配置；旧资源字段只兼容未迁移定义，不与新配置叠加播放。生命周期与试玩清单见[战斗表现](COMBAT_PRESENTATION.md)。
