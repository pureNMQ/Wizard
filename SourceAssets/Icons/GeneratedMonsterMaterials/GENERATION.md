# 怪物素材图标生成记录

2026-11-01，按用户授权使用内置 image_gen 生成四张独立透明PNG；原始输出直接复制入本目录，未作像素编辑。用于对应的Wizard物品定义与地面掉落图标。它们不是第三方Game-icons.net素材，不沿用该目录的作者署名。

## 完整提示词

各图使用以下模板，将 `{subject}` 替换为下表内容：

Use case: stylized-concept. Asset type: square fantasy RPG inventory material icon, readable at 32 pixels. Primary request: {subject}. Style: bold flat white glyph with a clear silhouette and a few generous transparent cutout details, matching monochrome fantasy inventory icons. Pure white foreground only. Composition: one centered isolated object occupying 75 percent of a square canvas, safe padding on all sides. Background: genuinely transparent alpha, no black square. No text, letters, border, frame, watermark, gradients, shadows or tiny details.

| PNG | subject |
| --- | --- |
| SlimeMucus.png | a single thick droplet-shaped blob of slime mucus pooling at its base, with two small bubbles and a few dripping strands, clearly an ingredient and not a whole monster |
| SlimeEye.png | one small slime monster eyeball enclosed in a wobbly rounded coating of jelly, a large round pupil and a short gelatinous drip beneath it, clearly distinct from a human eye |
| BeholderTentacle.png | one curved S-shaped severed fantasy observer monster tentacle, tapering to a curled tip, with three broad suction cups, no blood |
| BeholderEye.png | one large fantasy observer monster eyeball with a vertical slit pupil, a thick spiky folded outer rim and two short organic trailing strands, no blood |

生成选项：transparent_background=true；每件素材分别生成，无参考图输入。导入资产位于`/Game/Wizard/UI/Icons/T_<PNG文件名>`，导入通过UE-MCP。
