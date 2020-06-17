import subprocess
import os
import platform
import re
import glob
from datetime import datetime, timedelta

from robot.api import logger
import PathManager



def run_diagnose(pathTodiagnose,outputfolder):
    bashfile = os.path.join(PathManager.get_support_file_path(), "Diagnose/RunDiagnose.sh")
    command = ["bash", bashfile, pathTodiagnose, outputfolder]
    log_path = os.path.join("/tmp", "diagnose.log")
    log = open(log_path, 'w+')
    process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
    process.communicate()
    return process.returncode


def get_platform():
    platform_data = platform.platform()
    platform_data = platform_data.lower()
    if "centos" in platform_data:
        return "centos"
    elif "amazon" in platform_data or "amzn2" in platform_data:
        return "amazon_linux"
    elif "ubuntu" in platform_data or "debian" in platform_data or "with-glibc2.29" in platform_data:
        return "ubuntu"
    elif "rhel" in platform_data or "redhat" in platform_data:
        return "rhel"
    return "unknown"


def get_expected_files_for_platform():
    platform = get_platform()
    if "centos" == platform:
        return ["systemd-system.conf", "hosts", "fstab", "meminfo", "cpuinfo", "os-release", "boot.log", "selinux-config"]
    elif "amazon_linux" == platform:
        return ["systemd-system.conf", "hosts", "fstab", "meminfo", "cpuinfo", "os-release"]
    elif "ubuntu" == platform:
        return ["systemd-system.conf", "hosts", "fstab", "meminfo", "cpuinfo", "os-release"]
    elif "rhel" == platform:
        return ["systemd-system.conf", "hosts", "fstab", "meminfo", "cpuinfo", "os-release", "boot.log", "selinux-config"]
    raise ValueError('Platform unknown')


def check_file_permissions(path):
    list_of_files = glob.glob(path)
    for file_n in list_of_files:
        logger.debug("Permission checked for file: {}".format(file_n))
        st = os.stat(file_n)
        oct_perm = oct(st.st_mode)
        # We only care about the other users' permissions so only need to check last permission
        actual_permissions = oct_perm[-1:]
        if actual_permissions != '0':
            return "Unexpected Permissions: {}".format(str(oct_perm[-3:]))
    return "Correct Permissions"


def _get_date_time( date_time_string):
    try:
        date = datetime.strptime(date_time_string, '%b %d %H:%M:%S')
        date = date.replace(year=datetime.now().year)
        return date
    except Exception as ex:
        logger.info(repr(ex))
        pass
    try:
        date = datetime.strptime(date_time_string, '%b %d %Y %H:%M:%S')
        return date
    except Exception as ex:
        logger.info(repr(ex))
        pass
    return None


def _datetime_days_ago(days=10):
    return datetime.now()-timedelta(days=days)


def _get_oldest_journalctl_line_date(journal_ctl_file):
    # date followed by hostname followed by kernel
    # using the time as HH:MM:SS to stop it
    pattern=re.compile(r'(\w[\w\s]+ \d{2}:\d{2}:\d{2}) [\w\.-]+ \w+(\[\d+\])?:.*')
    latest_time = None
    entry = ""
    count = 0
    # journalctl usually orders entries by the oldest to the most recent. Hence,
    # it is not necessary to read the whole file.
    with open(journal_ctl_file) as jf:
        for line in jf:
            count += 1
            if count > 10:
                break
            logger.debug("Line: {}".format(line))
            matched = re.match(pattern, line)
            if matched:
                captured_data = matched.group(1)
                logger.debug("Captured data: {}".format(captured_data))
                line_date = _get_date_time(captured_data)
                if not line_date:
                    continue
                if latest_time is None:
                    latest_time = line_date
                    entry = line
                    continue
                if line_date < latest_time:
                    latest_time = line_date
                    entry = line
    return latest_time, entry, count


def _check_journalclt_does_not_have_old_entries(journal_ctl_file, days=10):
    oldest_date, journal_line, count_lines = _get_oldest_journalctl_line_date(journal_ctl_file)
    if oldest_date is None and count_lines > 5:
        raise AssertionError("Suspicious. No date found in journalctl found. Probably a test error")
    logger.info("Oldest date found on journal file: {} -> {}".format(journal_ctl_file, str(oldest_date)))
    if oldest_date:
        days_ago = _datetime_days_ago(int(days))
        if oldest_date < days_ago:
            raise AssertionError("Expected no entry older than {}. But found this: {}".format(
                                                                        str(days_ago), journal_line))


def check_journalclt_files_do_not_have_old_entries(system_dir, days=10):
    for filename in os.listdir(system_dir):
        if 'journalctl' in filename:
            full_path = os.path.join(system_dir, filename)
            logger.info("Checking file: {}".format(full_path))
            _check_journalclt_does_not_have_old_entries(full_path, days)


def create_log_file_of_specific_size(filename, size):
    with open(str(filename), "wb") as out:
        out.truncate(int(size))

def get_expected_path_of_base_manifest_files(diagnose_output):
    expected_comp = os.path.join(diagnose_output, 'BaseFiles/ServerProtectionLinux-Base-component')
    if os.path.exists(expected_comp):
        return expected_comp
    return os.path.join(diagnose_output, 'BaseFiles/ServerProtectionLinux-Base')
