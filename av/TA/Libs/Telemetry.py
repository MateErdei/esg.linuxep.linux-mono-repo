import json

def check_telemetry(telemetry):
    telemetry_dict = json.loads(telemetry)
    assert "av" in telemetry_dict, "No AV section in telemetry"
    av_dict = telemetry_dict["av"]
    assert "lr-data-hash" in av_dict, "No LR Data Hash in telemetry"
    assert "ml-lib-hash" in av_dict, "No ML Lib Hash in telemetry"
    assert "ml-pe-model-version" in av_dict, "No ML-PE Model Version in telemetry"
    assert "vdl-ide-count" in av_dict, "No IDE Count in telemetry"
    assert "vdl-version" in av_dict, "No VDL Version in telemetry"
    assert "version" in av_dict, "No AV Version Number in telemetry"
    assert "sxl4-lookup" in av_dict, "No SXL4 Lookup setting in telemetry"
    assert "health" in av_dict, "No Health value in telemetry"
    assert av_dict["lr-data-hash"] != "unknown", "LR Data Hash is set to unknown in telemetry"
    assert av_dict["ml-lib-hash"] != "unknown", "ML Lib Hash is set to unknown in telemetry"
    assert av_dict["ml-pe-model-version"] != "unknown", "No ML-PE Model is set to unknown in telemetry"
    assert av_dict["vdl-ide-count"] != "unknown", "No IDE Count is set to unknown in telemetry"
    assert av_dict["vdl-version"] != "unknown", "No VDL Version is set to unknown in telemetry"
    assert av_dict["sxl4-lookup"] is True, "SXL4 Lookup is defaulting to True in telemetry"
    assert av_dict["health"] is 0, "Health is defaulting to running in telemetry"
