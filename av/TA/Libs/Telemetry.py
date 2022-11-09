#!/bin/python3

import grp
import json
import os
import pwd
import subprocess
import sys

from robot.api import logger
import robot.libraries.BuiltIn


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
    assert "onAccessConfigured" in av_dict, "No onAccessConfigured in telemetry"
    assert av_dict["lr-data-hash"] != "unknown", "LR Data Hash is set to unknown in telemetry"
    assert av_dict["ml-lib-hash"] != "unknown", "ML Lib Hash is set to unknown in telemetry"
    assert av_dict["ml-pe-model-version"] != "unknown", "No ML-PE Model is set to unknown in telemetry"
    assert av_dict["vdl-ide-count"] != "unknown", "No IDE Count is set to unknown in telemetry"
    assert av_dict["vdl-version"] != "unknown", "No VDL Version is set to unknown in telemetry"
    assert av_dict["sxl4-lookup"], "SXL4 Lookup is defaulting to True in telemetry"
    assert av_dict["health"] == 0, "Health is not set to 0 in telemetry, showing bad AV Plugin Health"
    assert av_dict["threatHealth"] == 1, "Threat Health is not set to 1 in telemetry (1 = good, 2 = suspicious)"


def _su_supports_group():
    return os.path.isfile("/etc/redhat-release") and os.path.isfile("/bin/su")


def debug_telemetry(telemetry_symlink):
    id_proc = subprocess.run(['sudo', "-u", "sophos-spl-user", "id"],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
    logger.info("id: %d: %s" % (id_proc.returncode, id_proc.stdout))
    if _su_supports_group():
        id_proc = subprocess.run(['su', "sophos-spl-user", "--group=sophos-spl-group", "--command=id"],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT)
        logger.info("id2: %d: %s" % (id_proc.returncode, id_proc.stdout))

    # ldd = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", "ldd", telemetry_symlink],
    #                      stdout=subprocess.PIPE,
    #                      stderr=subprocess.STDOUT)
    # logger.error("LDD: %d: %s" % (ldd.returncode, ldd.stdout))
    # ldd = subprocess.run(['su', "sophos-spl-user", "--group=sophos-spl-group",
    #                       '--command="ldd %s"' % telemetry_symlink],
    #                      stdout=subprocess.PIPE,
    #                      stderr=subprocess.STDOUT)
    # logger.error("LDD2: %d: %s" % (ldd.returncode, ldd.stdout))

    r = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", telemetry_symlink],
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    logger.info("Run: %d: %s" % (r.returncode, r.stdout))
    if _su_supports_group():
        r = subprocess.run(['su', "sophos-spl-user", "--group=sophos-spl-group", '--command=%s' % telemetry_symlink],
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
        logger.info("Run2: %d: %s" % (r.returncode, r.stdout))

    # ls = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", "ls", "-l", telemetry_symlink],
    #                     stdout=subprocess.PIPE,
    #                     stderr=subprocess.STDOUT)
    # logger.error("ls: %d: %s" % (ls.returncode, ls.stdout))
    # strace = subprocess.run(['sudo', "-u", "sophos-spl-user", "-g", "sophos-spl-group", "strace", telemetry_symlink],
    #                         stdout=subprocess.PIPE,
    #                         stderr=subprocess.STDOUT)
    # logger.error("strace: %d: %s" % (strace.returncode, strace.stdout))

    # sudoers = open("/etc/sudoers").read()
    # logger.error("sudoers: %s" % sudoers)


class Result:
    pass


def _sophos_spl_user_primary_group_is_sophos_spl_group():
    # Check if sophos-spl-user's primary group is sophos-spl-group?
    try:
        expected_gid = grp.getgrnam("sophos-spl-group").gr_gid
    except KeyError:
        logger.error("Failed to get sophos-spl-group")
        return False

    try:
        actual_gid = pwd.getpwnam("sophos-spl-user").pw_gid
    except KeyError:
        logger.error("Failed to get sophos-spl-user")
        return False

    if actual_gid == expected_gid:
        return True

    logger.error("sophos-spl-user Primary group isn't sophos-spl-group - actual %d vs expected %d" %
                 (
                     actual_gid,
                     expected_gid
                 ))
    return False


def run_telemetry(telemetry_symlink, telemetry_config):
    if _sophos_spl_user_primary_group_is_sophos_spl_group():
        # run with sudo
        command = [
            'sudo', '-u', 'sophos-spl-user', telemetry_symlink, telemetry_config
        ]
    elif _su_supports_group():
        #  run with su
        command = ['su', 'sophos-spl-user', '--group=sophos-spl-group', '--command="%s %s"' % (
            telemetry_symlink,
            telemetry_config
        )]
    else:
        # try sudo with group
        command = [
            'sudo', '-u', 'sophos-spl-user', '-g', 'sophos-spl-group', telemetry_symlink, telemetry_config
        ]

    logger.info("Running telemetry using %s" % (str(command)))
    r = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=15)
    result = Result()
    result.rc = r.returncode
    result.stdout = r.stdout
    logger.info("Result %d: %s" % (result.rc, result.stdout))
    return result


def _get_variable(var_name, default_value=None):
    try:
        return robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % var_name) or default_value
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


def _get_sophos_install():
    return _get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")


def _get_av_log_by_run(path=None):
    """
    Get av.log split into runs (based on 0 timestamp)
    :return:
    """
    if path is None:
        av_log_dir = os.path.join(_get_sophos_install(), "plugins", "av", "log")
        av_log = os.path.join(av_log_dir, "av.log")
    else:
        av_log = path

    # Load av log
    data = open(av_log).read()
    # Split into lines
    lines = data.splitlines()
    # Split lines into runs
    runs = [[]]
    previous = -1
    for line in lines:
        line = line.strip()
        if line == "":
            continue

        parts = line.split()
        try:
            timestamp = int(parts[0])
            if timestamp < previous:
                runs.append([])
            previous = timestamp
        except ValueError:
            pass

        runs[-1].append(line)
    return runs


def av_log_contains_only_one_no_saved_telemetry_per_start(path=None):
    runs = _get_av_log_by_run(path)

    # Verify that
    # "TelemetryHelperImpl <> There is no saved telemetry at: /opt/sophos-spl/base/telemetry/cache/av-telemetry.json"
    # only appears once per run
    logger.debug("Found %d runs" % len(runs))
    for run in runs:
        found = False
        for line in run:
            if "There is no saved telemetry at" in line:
                if found:
                    # Duplicate Telemetry line
                    raise Exception("Found duplicate 'There is no saved telemetry at' line")
                logger.debug("Found saved telemetry line in run")
                found = True


def __p(s):
    print(s)


def __main(argv):
    logger.debug = __p
    av_log_contains_only_one_no_saved_telemetry_per_start(argv[1])
    return 0


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
