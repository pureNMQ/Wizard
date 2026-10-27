"""在 UE5.4 编辑器中生成首版林间营地；只接受空的目标关卡，避免覆盖手工编辑。"""

import json
import math
import random

import unreal


MAP = "/Game/Wizard/Maps/L_WizardClearing"
MODELS = "/Game/HandPaintedEnvironment/Assets/Models/"
TEXTURES = "/Game/HandPaintedEnvironment/Assets/Landscape/Textures/Summer_textures/"
MATERIALS = "/Game/Wizard/Environment/Materials"
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().split(".")[0] == MAP, "请先打开目标关卡"
assert not actors.get_all_level_actors(), "关卡必须为空；已有场景请手工调整，不能覆盖"
rng = random.Random(271027)
manifest = []
mesh_cache = {}


def vec(x, y, z):
    return unreal.Vector(float(x), float(y), float(z))


def record(actor, label, folder):
    actor.set_actor_label(label)
    actor.set_folder_path(unreal.Name(folder))
    manifest.append({"label": label, "folder": folder, "class": actor.get_class().get_name()})
    return actor


def mesh(name, label, x, y, scale=1.0, yaw=0.0, z=0.0, folder="04_Dressing", collision=True):
    path = name if name.startswith("/") else MODELS + name
    if path not in mesh_cache:
        mesh_cache[path] = unreal.load_asset(path)
    asset = mesh_cache[path]
    assert asset, path
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, vec(x, y, z))
    comp = actor.static_mesh_component
    comp.set_static_mesh(asset)
    actor.set_actor_rotation(unreal.Rotator(pitch=0.0, yaw=yaw, roll=0.0), False)
    size = (scale, scale, scale) if isinstance(scale, (float, int)) else scale
    actor.set_actor_scale3d(vec(*size))
    # 使用旋转缩放后的真实包围盒对齐地面，兼容不同素材的局部原点。
    center, extent = actor.get_actor_bounds(False)
    loc = actor.get_actor_location()
    actor.set_actor_location(vec(loc.x + x - center.x, loc.y + y - center.y, loc.z + z - center.z + extent.z), False, False)
    comp.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS if collision else unreal.CollisionEnabled.NO_COLLISION)
    return record(actor, label, folder)


def spawn(cls, label, xyz, folder="00_World", rotation=None):
    a = actors.spawn_actor_from_class(cls, vec(*xyz))
    if rotation:
        a.set_actor_rotation(unreal.Rotator(pitch=rotation[0], yaw=rotation[1], roll=rotation[2]), False)
    return record(a, label, folder)


def node(mat, cls, x, y, **props):
    n = unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
    for key, value in props.items():
        n.set_editor_property(key, value)
    return n


def connect(a, b, slot, output=""):
    assert unreal.MaterialEditingLibrary.connect_material_expressions(a, output, b, slot)


def ground_material():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = unreal.load_asset(MATERIALS + "/M_ClearingGround") if unreal.EditorAssetLibrary.does_asset_exist(MATERIALS + "/M_ClearingGround") else tools.create_asset("M_ClearingGround", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    assert mat
    unreal.MaterialEditingLibrary.delete_all_material_expressions(mat)
    pos = node(mat, unreal.MaterialExpressionWorldPosition, -1100, 0)
    mask = node(mat, unreal.MaterialExpressionComponentMask, -900, 0, r=True, g=True)
    connect(pos, mask, "")
    tiling = node(mat, unreal.MaterialExpressionMultiply, -700, 0, const_b=0.0025)
    connect(mask, tiling, "A")
    samples = []
    for i, name in enumerate(["Grass_1", "Ground_2_texture", "Stone_floor_texture"]):
        tex = node(mat, unreal.MaterialExpressionTextureSample, -450, i * 220,
                   texture=unreal.load_asset(TEXTURES + name))
        connect(tiling, tex, "UVs")
        samples.append(tex)
    code = """
float2 q = P.xy;
float noise = sin(q.x*.003 + sin(q.y*.005))*sin(q.y*.004)*65;
float camp = 1-smoothstep(1260+noise,1510+noise,length((q-float2(-2150,0))*float2(1,.95)));
float arena = 1-smoothstep(1650+noise,1900+noise,length(q-float2(2200,0)));
float path = (1-smoothstep(230+noise*.3,340+noise*.3,abs(q.y))) * (1-smoothstep(2200,2400,abs(q.x+100)));
float dirt = saturate(max(max(camp,arena),path));
float paving = 1-smoothstep(1380+noise*.4,1550+noise*.4,length(q-float2(2200,0)));
float3 grass = Grass.rgb * float3(.78,.86,.65);
float3 soil = Dirt.rgb * float3(.95,.87,.74);
float3 stone = Stone.rgb * float3(.72,.77,.73);
return lerp(lerp(grass,soil,dirt),stone,paving*.88);
"""
    custom_inputs = []
    for name in ["P", "Grass", "Dirt", "Stone"]:
        custom_input = unreal.CustomInput()
        custom_input.set_editor_property("input_name", name)
        custom_inputs.append(custom_input)
    custom = node(mat, unreal.MaterialExpressionCustom, -100, 0,
                  code=code, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT3,
                  inputs=custom_inputs)
    connect(pos, custom, "P")
    for n, tex in zip(["Grass", "Dirt", "Stone"], samples):
        connect(tex, custom, n, "RGB")
    unreal.MaterialEditingLibrary.connect_material_property(custom, "", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = node(mat, unreal.MaterialExpressionConstant, -100, 600, r=0.92)
    unreal.MaterialEditingLibrary.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def rune_material():
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset("M_RuneInlay", MATERIALS, unreal.Material, unreal.MaterialFactoryNew())
    color = node(mat, unreal.MaterialExpressionConstant3Vector, -200, 0,
                 constant=unreal.LinearColor(0.035, 0.32, 0.30, 1.0))
    glow = node(mat, unreal.MaterialExpressionConstant3Vector, -200, 200,
                constant=unreal.LinearColor(0.03, 0.55, 0.48, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


ground = mesh("/Engine/BasicShapes/Cube", "Ground_Walkable", 0, 0, (120, 86, .8), z=-80, folder="01_Terrain")
ground.static_mesh_component.set_material(0, ground_material())
rune = rune_material()

# 暖色斜阳配合冷色天空，让营地、道路和遗迹的轮廓在俯视相机下保持清晰。
sun = spawn(unreal.DirectionalLight, "Sun_LateAfternoon", (0, 0, 1800), rotation=(-48, -28, 0))
sun.light_component.set_editor_property("intensity", 6.0)
sun.light_component.set_editor_property("atmosphere_sun_light", True)
sun.light_component.set_light_color(unreal.LinearColor(1.0, .87, .69, 1.0))
sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky = spawn(unreal.SkyLight, "Sky_ForestFill", (0, 0, 1000))
sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky.light_component.set_editor_property("intensity", 2.5)
sky.light_component.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
sky.light_component.set_editor_property("cubemap", unreal.load_asset("/Engine/MapTemplates/Sky/DaylightAmbientCubemap"))
sky.light_component.set_editor_property("real_time_capture", False)
sky.light_component.recapture_sky()
spawn(unreal.SkyAtmosphere, "Sky_Atmosphere", (0, 0, 0))
fog = spawn(unreal.ExponentialHeightFog, "Fog_SoftDistance", (0, 0, -200))
fog.component.set_editor_property("fog_density", .009)
fog.component.set_editor_property("fog_height_falloff", .24)
fog.component.set_editor_property("start_distance", 4000.0)
pp = spawn(unreal.PostProcessVolume, "PostProcess_Clearing", (0, 0, 0))
pp.set_editor_property("unbound", True)
settings = pp.get_editor_property("settings")
for name, value in [("override_auto_exposure_min_brightness", True), ("override_auto_exposure_max_brightness", True),
                    ("auto_exposure_min_brightness", 1.0), ("auto_exposure_max_brightness", 1.0),
                    ("override_bloom_intensity", True), ("bloom_intensity", .22),
                    ("override_vignette_intensity", True), ("vignette_intensity", .18)]:
    settings.set_editor_property(name, value)
pp.set_editor_property("settings", settings)

# 营地建筑沿后缘排列，正中保留出生、移动和交互空间。
mesh("Witch_house", "Camp_WizardLodge", -2950, 1050, .85, 0, folder="02_Camp/Buildings")
mesh("Small_house", "Camp_MerchantHouse", -1260, 1120, .86, 0, folder="02_Camp/Buildings")
mesh("Awning", "Camp_MerchantCanopy", -1250, 685, 1.7, 0, z=175, folder="02_Camp/Shop")
mesh("Box_1", "Shop_Counter", -1240, 500, 1.5, folder="02_Camp/Shop")
mesh("Chest", "Shop_EquipmentChest", -930, 660, 1.0, -15, folder="02_Camp/Shop")
mesh("Barrel", "Shop_Barrel", -1600, 650, 1.1, 20, folder="02_Camp/Shop")
mesh("Box_2", "Shop_Supplies", -1680, 860, .9, 15, folder="02_Camp/Shop")
mesh("Cauldron", "Camp_Alchemy", -2550, 550, .85, folder="02_Camp/Props")
mesh("Well", "Camp_Well", -2980, -520, .9, -20, folder="02_Camp/Props")
mesh("Anvil", "Camp_Anvil", -3300, 260, .85, 25, folder="02_Camp/Props")
mesh("Log", "Camp_Woodpile", -3440, 520, .85, 90, folder="02_Camp/Props")
mesh("Stump", "Camp_Seat", -2100, -470, 1.0, 40, folder="02_Camp/Props")

# 石门由立柱和拱顶组成；通道下方净高及宽度均按玩家胶囊留足。
for side in [-1, 1]:
    mesh("Pillar", "Gate_Pillar_" + str(side), 100, side * 335, (1.15, 1.15, 1.05), folder="03_Ruins/Gateway")
mesh("Arch", "Gate_StoneArch", 100, 0, 1.35, 90, z=475, folder="03_Ruins/Gateway")
mesh("Altar", "Camp_ChallengePedestal", -480, -640, .65, 90, folder="02_Camp/Challenge")
mesh("Crystal_1", "Camp_ChallengeCrystal", -480, -640, .8, 20, z=175, folder="02_Camp/Challenge", collision=False)

# 碎石铺路保持低矮，减少相机与角色行走时的遮挡。
for i, x in enumerate(range(-2620, 1200, 180)):
    for j in [-1, 1]:
        mesh("Tile_" + str(1 + (i + j) % 7), "Path_Stone_%02d_%s" % (i, j), x + rng.uniform(-15, 15), j * 92 + rng.uniform(-15, 15),
             (1.25, 1.25, .16), rng.uniform(-15, 15), z=1, folder="01_Terrain/Path", collision=False)

# 遗迹围墙只占边缘，中央是约三十米宽的连续战斗空间。
for i, angle in enumerate(range(-150, 151, 30)):
    a = math.radians(angle)
    x, y = 2200 + math.cos(a) * 1810, math.sin(a) * 1810
    mesh("Pillar" if i % 3 == 0 else "Pillar_ruined_1", "Arena_RuinColumn_%02d" % i,
         x, y, rng.uniform(.8, 1.1), angle, folder="03_Ruins/Perimeter")
    if i < 10:
        a2 = math.radians(angle + 15)
        mesh("Stone_fence", "Arena_LowWall_%02d" % i, 2200 + math.cos(a2) * 1860, math.sin(a2) * 1860,
             (1.45, .85, .55), angle + 105, folder="03_Ruins/Perimeter")
mesh("Altar", "Arena_FarShrine", 3660, 0, 1.0, 90, folder="03_Ruins/Shrine")
for i, (dx, dy, s) in enumerate([(0, 0, 1.4), (120, 100, .7), (-100, -100, .85)]):
    mesh("Crystal_" + str(i + 1), "Arena_ShrineCrystal_%02d" % i, 3660 + dx, dy, s, i * 90,
         z=267 if i == 0 else 0, folder="03_Ruins/Shrine", collision=False)

# 低矮几何镶嵌组成法阵，不在场地中央添加阻挡。
for i in range(48):
    a = math.tau * i / 48
    ring = mesh("/Engine/BasicShapes/Cube", "Arena_RuneRing_%02d" % i,
                2200 + math.cos(a) * 760, math.sin(a) * 760, (0.46, .10, .012), math.degrees(a) + 90,
                z=1, folder="03_Ruins/RuneInlay", collision=False)
    ring.static_mesh_component.set_material(0, rune)
for i in range(8):
    a = math.tau * i / 8
    tile = mesh("Tile_4", "Arena_Runestone_%02d" % i, 2200 + math.cos(a) * 980, math.sin(a) * 980,
                (1.2, 1.2, .12), math.degrees(a), z=1, folder="03_Ruins/RuneInlay", collision=False)

# 林带使用固定种子，不让高冠树遮住中间的游戏区域。
tree_points = []
for i in range(26):
    x = -4700 + i * 370
    tree_points.append((x + rng.uniform(-130, 130), 2500 + rng.uniform(-220, 350)))
for i in range(19):
    x = -4700 + i * 500
    tree_points.append((x + rng.uniform(-100, 100), -2450 + rng.uniform(-300, 140)))
for side in [-1, 1]:
    for y in range(-1800, 2300, 450):
        tree_points.append((side * 4950 + rng.uniform(-120, 160), y + rng.uniform(-120, 120)))
for i, (x, y) in enumerate(tree_points):
    name = rng.choice(["Tree_1", "Tree_2", "Tree_4", "Pine_tree", "Pine_tree_2"])
    # 南侧前景稍矮，给俯视镜头留下视线。
    s = rng.uniform(.65, 1.0) if y > 0 else rng.uniform(.5, .74)
    mesh(name, "Forest_Tree_%03d" % i, x, y, s, rng.uniform(0, 360), z=-10, folder="05_Forest/Trees")
    if i % 2 == 0:
        mesh("Rock_" + str(rng.randint(1, 4)), "Forest_Boulder_%03d" % i, x + rng.uniform(-220, 220), y + rng.uniform(-180, 180),
             rng.uniform(.65, 1.1), rng.uniform(0, 360), z=-20, folder="05_Forest/Rocks")
    for j in range(2):
        mesh(rng.choice(["Bush_1", "Bush_2", "Fern"]), "Forest_Undergrowth_%03d_%d" % (i, j),
             x + rng.uniform(-260, 260), y + rng.uniform(-220, 220), rng.uniform(.65, 1.3), rng.uniform(0, 360),
             folder="05_Forest/Undergrowth", collision=False)

# 通道两侧的岩簇强化分区，但保留中间通路。
for i, (x, y, s) in enumerate([(-150, 980, 1.15), (150, 1400, 1.0), (-350, 1850, .8),
                              (-150, -1100, .8), (200, -1550, 1.0), (500, -2050, .75)]):
    mesh("Rock_2" if i % 2 else "Rock_3", "Gate_RockBank_%02d" % i, x, y, s, i * 57,
         z=-35, folder="05_Forest/GateBanks")
    mesh("Bush_2", "Gate_Bush_%02d" % i, x - 200, y - 100, 1.2, i * 43,
         folder="05_Forest/GateBanks", collision=False)

for i in range(48):
    if i < 24:
        x, y = rng.uniform(-3800, -1000), rng.choice([-1, 1]) * rng.uniform(1300, 1850)
    else:
        a = rng.uniform(-math.pi * .84, math.pi * .84)
        x, y = 2200 + math.cos(a) * rng.uniform(1900, 2250), math.sin(a) * rng.uniform(2000, 2300)
    mesh(rng.choice(["Flowers_1", "Flowers_2", "Mushroom_1", "Fern", "Plant"]), "Detail_Plant_%03d" % i,
         x, y, rng.uniform(.65, 1.0), rng.uniform(0, 360), folder="05_Forest/Details", collision=False)


def point_light(label, xyz, color, intensity, radius):
    light = spawn(unreal.PointLight, label, xyz, "00_World/AccentLights")
    light.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    light.light_component.set_editor_property("intensity", intensity)
    light.light_component.set_editor_property("attenuation_radius", radius)
    light.light_component.set_editor_property("cast_shadows", False)
    light.light_component.set_light_color(unreal.LinearColor(*color, 1.0))


for i, (x, y) in enumerate([(-1950, 600), (-900, -450), (-130, 470), (380, -470)]):
    mesh("street_light", "Camp_Lantern_%02d" % i, x, y, .9, 90 if i % 2 else -90, folder="02_Camp/Lanterns")
    point_light("Light_Lantern_%02d" % i, (x, y, 260), (1.0, .55, .17), 110, 600)
point_light("Light_ChallengeCrystal", (-480, -640, 260), (.08, .65, 1.0), 110, 650)
point_light("Light_ArenaShrine", (3550, 0, 400), (.08, .75, .7), 110, 850)

# 语义标记可供后续业务接入；不把未实现的商店、挑战和波次表现成已完成。
for label, xyz, tags in [
    ("Marker_CampShop", (-1250, 300, 100), ["Wizard.Camp.Shop"]),
    ("Marker_ChallengeStart", (-480, -400, 100), ["Wizard.Camp.Challenge"]),
    ("Marker_CampReturn", (-2100, -100, 100), ["Wizard.Camp.Return"]),
    ("Marker_ArenaSpawn_North", (2300, 1250, 100), ["Wizard.Arena.Spawn"]),
    ("Marker_ArenaSpawn_East", (3400, 700, 100), ["Wizard.Arena.Spawn"]),
    ("Marker_ArenaSpawn_South", (2300, -1250, 100), ["Wizard.Arena.Spawn"]),
]:
    marker = spawn(unreal.TargetPoint, label, xyz, "06_Gameplay/Markers")
    marker.set_editor_property("tags", [unreal.Name(t) for t in tags])
spawn(unreal.PlayerStart, "PlayerStart_Camp", (-2100, -100, 115), "06_Gameplay", (0, 0, 0))
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/Wizard.WizardGameMode"))

# 整体场地的外围碰撞使用隐藏的普通网格，不依赖装饰物是否自带碰撞。
for label, xyz, scale in [
    ("Bounds_North", (0, 3450, 0), (112, 1, 14)),
    ("Bounds_South", (0, -3450, 0), (112, 1, 14)),
    ("Bounds_West", (-5500, 0, 0), (1, 70, 14)),
    ("Bounds_East", (5500, 0, 0), (1, 70, 14)),
]:
    bound = mesh("/Engine/BasicShapes/Cube", label, xyz[0], xyz[1], scale, z=-50, folder="06_Gameplay/Bounds")
    bound.set_actor_hidden_in_game(True)
    bound.set_is_temporarily_hidden_in_editor(True)

# SCENE_FINISH_BEGIN
# 营地的篝火和补给角提供近景焦点，中心通路保持畅通。
for i in range(9):
    a = math.tau * i / 9
    mesh("Rock_1", "Camp_FireStone_%02d" % i, -2300 + math.cos(a) * 100, -590 + math.sin(a) * 100,
         (.27, .27, .22), i * 39, z=2, folder="02_Camp/Firepit", collision=False)
for i, yaw in enumerate([25, 145]):
    mesh("Log_1", "Camp_FireLog_%02d" % i, -2300, -590, .36, yaw, z=5 + i * 12,
         folder="02_Camp/Firepit", collision=False)
fire = spawn(unreal.Emitter, "Camp_Fire", (-2300, -590, 30), "02_Camp/Firepit")
fire.particle_system_component.set_template(unreal.load_asset("/Game/HandPaintedEnvironment/Assets/Particles/Fire"))
fire.set_actor_scale3d(vec(.45, .45, .45))
point_light("Light_CampFire", (-2300, -590, 130), (1.0, .34, .07), 110, 450)
for i, (x, y, yaw) in enumerate([(-3400, 1500, 0), (-3140, 1600, 20), (-1080, 1620, -15)]):
    mesh("Fence", "Camp_GardenFence_%02d" % i, x, y, 1.05, yaw, folder="02_Camp/Props")
for i, (x, y) in enumerate([(-1680, 690), (-1550, 820), (-990, 720)]):
    mesh("Box_" + str(2 + i), "Shop_SupplyCrate_%02d" % i, x, y, .8, i * 35, folder="02_Camp/Shop")
for i in range(6):
    mesh("Tile_" + str(1 + i % 7), "Camp_LodgeStep_%02d" % i, -2820 + i * 45, 170 + i * 95,
         (1.15, 1.15, .12), i * 12, z=1, folder="01_Terrain/Path", collision=False)
for label, xyz, rotation in [
    ("Camera_Overview", (100, -5700, 6600), (-49, 90, 0)),
    ("Camera_Camp", (-2050, -1650, 2050), (-48, 90, 0)),
    ("Camera_Arena", (2200, -2650, 3100), (-48, 90, 0)),
]:
    camera = spawn(unreal.CameraActor, label, xyz, "07_PreviewCameras", rotation)
    camera.camera_component.set_editor_property("field_of_view", 62.0)
    camera.camera_component.set_editor_property("constrain_aspect_ratio", False)
    # 预览相机不抢占玩家视角。
    camera.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.DISABLED)
# SCENE_FINISH_END
unreal.EditorLevelLibrary.set_level_viewport_camera_info(vec(100, -5700, 6600), unreal.Rotator(pitch=-49, yaw=90, roll=0))
assert levels.save_current_level()
out = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir() + "SceneChecks/build-manifest.json")
with open(out, "w", encoding="utf-8") as f:
    json.dump({"map": MAP, "actor_count": len(manifest), "actors": manifest}, f, ensure_ascii=False, indent=2)
unreal.log("WIZARD_CLEARING_BUILT " + str(len(manifest)))
