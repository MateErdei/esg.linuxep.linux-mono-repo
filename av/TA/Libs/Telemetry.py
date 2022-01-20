import json
import subprocess

from robot.api import logger


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
    assert "threatHealth" in av_dict, "No Threat Health value in telemetry"
    assert "threatMemoryUsage" in av_dict, "No Threat Memory Usage in telemetry"
    assert "threatProcessAge" in av_dict, "No Threat Process Age in telemetry"
    assert av_dict["lr-data-hash"] != "unknown", "LR Data Hash is set to unknown in telemetry"
    assert av_dict["ml-lib-hash"] != "unknown", "ML Lib Hash is set to unknown in telemetry"
    assert av_dict["ml-pe-model-version"] != "unknown", "No ML-PE Model is set to unknown in telemetry"
    assert av_dict["vdl-ide-count"] != "unknown", "No IDE Count is set to unknown in telemetry"
    assert av_dict["vdl-version"] != "unknown", "No VDL Version is set to unknown in telemetry"
    assert av_dict["sxl4-lookup"], "SXL4 Lookup is defaulting to True in telemetry"
    assert av_dict["health"] == 0, "Health is not set to 0 in telemetry, showing bad AV Plugin Health"
    assert av_dict["threatHealth"] == 1, "Threat Health is not set to 1 in telemetry (1 = good, 2 = suspicious)"


def debug_telemetry(telemetry_symlink):
    id_proc = subprocess.run(['sudo', "-u", "sophos-spl-user", "id"],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
    logger.error("id: %d: %s" % (id_proc.returncode, id_proc.stdout))
    id_proc = subprocess.run(['su', "sophos-spl-user", "--group=sophos-spl-group", "--command=id"],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
    logger.error("id2: %d: %s" % (id_proc.returncode, id_proc.stdout))

    ldd = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", "ldd", telemetry_symlink],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    logger.error("LDD: %d: %s" % (ldd.returncode, ldd.stdout))
    ldd = subprocess.run(['su', "sophos-spl-user", "--group=sophos-spl-group",
                          '--command="ldd %s"' % telemetry_symlink],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    logger.error("LDD2: %d: %s" % (ldd.returncode, ldd.stdout))

    r = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", telemetry_symlink],
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    logger.error("Run: %d: %s" % (r.returncode, r.stdout))
    r = subprocess.run(['su', "sophos-spl-user", "--group=sophos-spl-group", '--command=%s' % telemetry_symlink],
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    logger.error("Run2: %d: %s" % (r.returncode, r.stdout))

    ls = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", "ls", "-l", telemetry_symlink],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT)
    logger.error("ls: %d: %s" % (ls.returncode, ls.stdout))
    strace = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", "strace", telemetry_symlink],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    logger.error("strace: %d: %s" % (strace.returncode, strace.stdout))

    # sudoers = open("/etc/sudoers").read()
    # logger.error("sudoers: %s" % sudoers)
