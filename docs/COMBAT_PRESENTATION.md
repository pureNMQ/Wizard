# 战斗表现配置与验收

2026-11-02，T12首轮。用户确认范围见规格末尾，状态见[T12](../.scratch/wizard-demo/issues/12-combat-presentation.md)。

## 配置入口

`/Game/Wizard/Data/Presentation`包含Basic、Fireball、IceArrow、Lightning、Blink、Shield、Heal、Slime、Beholder、Player、Challenge共11份配置。Stages配置各阶段的粒子、声音、缩放、偏移、寿命和音量；Projectile控制弹道，ColorOverrides覆盖Niagara用户颜色参数。HitStopSeconds与ImpactLevel控制命中顿帧和震屏等级。

DA_BasicCombat、六技能定义、怪物及玩家蓝图引用各自Presentation；Challenge_Arena只新增表现引用，地图布局不变。技能原资源字段兼容旧定义，配置Presentation后完整优先使用新配置，不重复播放。GE_BasicAttackDamage旧Cue已解除，命中和撞墙由单次碰撞入口分别发出。

Wizard/Effects/Combat保存项目副本和程序材质，火球与冰箭继续使用用户指定的Wizard/Effects副本。TargetRing材质通过100cm的Plane和Radius/50缩放对应真实伤害范围；护盾球体跟随角色且无碰撞。准备动作、护盾和减速由GameplayCue管理；取消、GE移除和Cue销毁均有清理。怪物前摇数值、攻击判定和伤害公式不变。

Cue蓝图位于`/Game/Wizard/GAS/Cues`。继承原生父类相同标签的蓝图须使用能推导该标签的名字：`GC_Wizard_Combat_Burst`、`GC_Wizard_Combat_Windup`、`GC_Wizard_Skill_Impact`、`GC_Wizard_Skill_Shield`。UE5.4从资产名推导标签失败时会回退父类GameplayCueTag，却可能留下空GameplayCueName；后者才是管理器扫描所用的资产注册字段。因此只读回GameplayCueTag或编译成功不足以证明事件能够分发。`GC_CombatSlow`显式覆写为不同于父类的Slow标签，其注册正常。`Wizard.Combat.Feedback.CueDiscovery`检查实际运行时分发表及处理类加载，不播放音画。

该目录同时加入DefaultGame.ini的DirectoriesToAlwaysCook。GameplayCueNotifyPaths只决定运行时扫描范围，不保证仅通过标签间接引用的蓝图会被Cook；交付须核对Cook产物和IoStore打包输入清单中存在Cue资产。

第二轮修正：冰箭弹丸/命中Mesh1绑定MI_IceCrystal，观察者对应Mesh1绑定MI_EnemyCrystal；两者继承项目副本M_CrystalBase，分别覆盖纹理色阶、边缘光和粗糙度。粒子ColorOverrides不能代替网格材质配置。普攻命中采用Hit3副本NS_ArcaneImpact；闪现采用Circle1副本NS_BlinkVortex（关闭烟雾渲染），紫色符环与螺旋短促显示在起终点；护盾采用持续球体加NS_ShieldOrbit光点（关闭烟雾/螺旋/符环渲染），开启、消散和承伤使用短光点，不再引用雷击冲击波。盾体材质有0.09基础透明度和Fresnel轮廓，存续期间持续可辨。

GE触发的Cue通过EffectContext携带SourceObject，手动Cue可以直接携带。处理器使用FGameplayCueParameters::GetSourceObject()读取两种来源，否则护盾与减速的持续表现会在有数值效果时仍然缺失。CueEffectContext工程测试覆盖该边界、重复激活及移除清理，不验证实际像素或声音。

Wizard/Audio包含16个SoundCue、SA_Combat衰减及普通/高优先两份并发配置（8/6）。普通命中不能占用玩家受击、破盾和雷击的高优先声音组。Cue采用WavePlayer→Modulator，有轻微随机音高，无循环。声源来自已恢复RPG_Fantasy_SFX，实际听感仍待用户确认。

## 反馈与生命周期

AttributeSet在GAS结算边界计算实际生命损失、护盾吸收和有效治疗。伤害结果在SetHealth同步触发死亡/清场之前分发，保证最后一个敌人仍有击杀反馈；订阅者只消费结果，不得回写战斗状态。初始化、换装、回营地恢复不冒战斗数字。

PlayerController的CombatFeedback组件管理数字、镜头和时间缩放。0.1秒内同目标同类数字合并，最多32组，0.85秒消失。WBP_CombatFeedback复用既有HUD层，可配置字号和颜色；生命损失用减号、治疗用加号、吸收用方括号。

火球35ms、雷击55ms、击杀45ms、玩家破盾40ms，倍率为原值的0.1；单帧取最大请求，进行中不延长，结束后120ms间隔。震屏80/120/160ms，真实时间衰减，幅度0.15/0.3/0.5度；组件bShakeEnabled、bHitStopEnabled和Intensity可调整。暂停、失焦、解除占有、死亡、回营地及退出恢复原时间倍率；失焦期间不触发新的局部反馈。

## 用户试玩清单

2026-11-02玩家死亡动画：BP_WizardCharacter.DeathMontage引用AM_WizardDeath，使用BattleWizardPBR/DieAnim（0.833秒）、DeathSlot、0.08秒淡入且关闭自动淡出。ABP_WizardCharacter在既有上半身攻击混合之后增加全身DeathSlot。PlayerState在取消能力后同步角色死亡表现，重复通知不重播；回营生命恢复时立即停止死亡Montage并返回原动画图，结算仍为2秒。纯表现不决定死亡、掉落或回营规则。

2026-11-02护盾修订：ShieldRadiusScale配置盾体半径相对角色胶囊半高的倍率，初值(1.25,1.25,1.35)，横向/竖向较旧值(0.8,0.8,1.1)增加约56%/23%，保留跑动姿态空间。盾体继续直接附着角色根组件、无碰撞；M_CombatShield和Shield配置的开启/消散/环绕粒子采用金色(3,1.65,0.12)。玩家模型材质和承伤规则不变，动态动作包裹效果待用户验收。

2026-11-02减速表现修订：NS_IceStatus只渲染Smoke与Sparks1，关闭冰刺Mesh1/Mesh2、Trace与Wave。专用M_IceMist将原烟雾透明度乘0.22、发光乘0.4；Status相对偏移Z=-15厘米，缩放(0.12,0.12,0.18)。附着效果继续由3秒减速GE/Cue生命周期清除，不使用Style.Lifetime定时销毁。实际贴合与存续观感待用户验收。

2026-11-02普攻色彩修订：DA_BasicPresentation的Flash/Sparks1采用紫色(1.35,0.24,3)，与原拖尾RGB比例一致；REF/Trail保留原亮度。史莱姆Death独立使用NS_SlimeDeathParticles（NS_Death副本，关闭Flash渲染器，保留绿色碎粒），避免大面积死亡闪光混淆普攻击中色。编辑态配置/编译已核验，实际观感待用户确认。

1. 单点普攻、长按连射、撞墙、击杀史莱姆，确认杖尖/弹道/命中/声音/数字同步。
2. 火球与冰箭命中；减速结束时霜雾消失。雷击圈对应真实范围，命中多个敌人只发生一次镜头冲击。
3. 闪现有效落点与墙后失败；护盾开启、吸收、破盾、到期、刷新；治疗有效回血与满血失败。
4. 史莱姆聚力、观察者蓄能及红色弹道可辨识；玩家掉血与护盾吸收反馈有区别。
5. 顿帧中松开左键、打开菜单、切出窗口、死亡与回营地，不持续慢动作、不残留连射和状态特效。
6. 连续三轮挑战检查波次、结界、胜败提示。固定分辨率、画质和群怪条件，与旧包比较流畅度，记录卡顿、声音拥挤及遮挡。

Agent没有启动PIE或成品做视听验收。声音审美、动画接触感、粒子缩放与群战性能需要用户反馈后调优，尚无运行时性能结论。
