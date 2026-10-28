"""T02 最小资产装配。保留已有 Widget 布局与场景布景，重复执行不重复添加目标。"""
import unreal

if not unreal.WizardCombatAssetTools.build_combat_assets():
    raise RuntimeError("Combat asset generation failed")

assets = unreal.AssetToolsHelpers.get_asset_tools()
config_path = "/Game/Wizard/Data/DA_BasicCombat"
config = unreal.load_asset(config_path)
if config is None:
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.WizardCombatConfig)
    config = assets.create_asset("DA_BasicCombat", "/Game/Wizard/Data", unreal.WizardCombatConfig, factory)
    if config is None:
        raise RuntimeError("Could not create combat config")
    unreal.EditorAssetLibrary.save_loaded_asset(config)

subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsystem.load_level("/Game/Wizard/Maps/L_WizardClearing")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
existing = [a for a in actors.get_all_level_actors() if a.get_actor_label() == "T02_CombatTarget"]
if not existing:
    enemy = actors.spawn_actor_from_class(unreal.WizardEnemy, unreal.Vector(-1580, -100, 120))
    if enemy is None:
        raise RuntimeError("Failed to create combat target")
    enemy.set_actor_label("T02_CombatTarget")
# 直接保存既有地图，只增一个可见目标；不执行初始场景生成。
subsystem.save_current_level()
unreal.log("WIZARD_COMBAT_ASSETS_PASS")
