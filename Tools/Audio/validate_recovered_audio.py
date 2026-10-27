"""在 UE5.4 Python commandlet 中验证恢复音效，结果输出到 Saved。"""

import hashlib
import json
import wave
from pathlib import Path

import unreal


PROJECT_ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = PROJECT_ROOT / "SourceAssets/Audio/Recovered"
OUTPUT_ROOT = PROJECT_ROOT / "Saved/AudioImportChecks/20261027"


def read_pcm(path):
    """比较实际 PCM，忽略 WAV 容器中与音频无关的元数据块。"""
    with wave.open(str(path), "rb") as stream:
        parameters = {
            "channels": stream.getnchannels(),
            "sample_width": stream.getsampwidth(),
            "sample_rate": stream.getframerate(),
            "frames": stream.getnframes(),
        }
        digest = hashlib.sha256(stream.readframes(stream.getnframes())).hexdigest()
    return parameters, digest


def validate_record(record):
    asset_path = record["packagePath"] + "/" + record["name"]
    result = {"asset": asset_path, "source": record["source"]}
    try:
        source = PROJECT_ROOT / record["filePath"]
        if hashlib.sha256(source.read_bytes()).hexdigest() != record["wavSha256"]:
            raise RuntimeError("源 WAV 与恢复清单的摘要不一致")

        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(asset, unreal.SoundWave):
            raise RuntimeError("无法加载为 SoundWave")

        import_data = asset.get_editor_property("asset_import_data")
        imported_sources = import_data.extract_filenames() if import_data else []
        if len(imported_sources) != 1 or Path(imported_sources[0]).resolve() != source.resolve():
            raise RuntimeError("资产的重新导入路径未指向对应 WAV 源文件")

        expected = record["properties"]
        for property_name in ("duration", "volume", "pitch"):
            if abs(asset.get_editor_property(property_name) - expected[property_name]) > 0.001:
                raise RuntimeError("资产属性不匹配: " + property_name)
        for property_name in ("num_channels", "looping"):
            if asset.get_editor_property(property_name) != expected[property_name]:
                raise RuntimeError("资产属性不匹配: " + property_name)

        relative = Path(record["source"].removeprefix("/Game/"))
        exported = OUTPUT_ROOT / "Roundtrip" / relative.with_suffix(".wav")
        exported.parent.mkdir(parents=True, exist_ok=True)
        task = unreal.AssetExportTask()
        task.set_editor_property("object", asset)
        task.set_editor_property("filename", str(exported))
        task.set_editor_property("automated", True)
        task.set_editor_property("prompt", False)
        task.set_editor_property("replace_identical", True)
        task.set_editor_property("exporter", unreal.SoundExporterWAV())
        if not unreal.Exporter.run_asset_export_task(task):
            raise RuntimeError("UE5.4 WAV 重新导出失败")

        source_parameters, source_digest = read_pcm(source)
        exported_parameters, exported_digest = read_pcm(exported)
        if source_parameters != exported_parameters or source_digest != exported_digest:
            raise RuntimeError("重新导出的 PCM 数据或采样参数不一致")
        result.update(status="passed", pcm_sha256=source_digest, **source_parameters)
    except Exception as error:
        result.update(status="failed", error=str(error))
        unreal.log_error("[WizardAudioValidation] " + asset_path + ": " + str(error))
    return result


def main():
    engine = unreal.SystemLibrary.get_engine_version()
    if not engine.startswith("5.4."):
        raise RuntimeError("必须使用 UE5.4 执行验证，当前版本: " + engine)
    manifest = json.loads((SOURCE_ROOT / "manifest.json").read_text(encoding="utf-8"))
    records = manifest["records"]
    if len(records) != manifest["soundWaveCount"] or not records:
        raise RuntimeError("恢复清单的音效数量不一致或为空")
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    results = [validate_record(record) for record in records]
    summary = {
        "engine": engine,
        "total": len(results),
        "passed": sum(record["status"] == "passed" for record in results),
        "failed": sum(record["status"] == "failed" for record in results),
        "records": results,
    }
    (OUTPUT_ROOT / "validation-result.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8"
    )
    unreal.log("[WizardAudioValidation] " + str(summary["passed"]) + "/" + str(summary["total"]))
    if summary["failed"]:
        raise RuntimeError("恢复音效验证失败，详见 Saved/AudioImportChecks/20261027")


if __name__ == "__main__":
    main()
