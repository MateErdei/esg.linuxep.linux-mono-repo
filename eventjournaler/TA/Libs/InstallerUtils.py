import grp
import os
import pwd
import shutil
import stat
import subprocess
import tempfile
import time

import psutil
import robot.libraries.BuiltIn
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn

SOPHOS_GROUP = "sophos-spl-group"


def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${%s}" % varName)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


def get_sophos_install():
    return get_variable("SOPHOS_INSTALL")


def get_group(gid, cache=None):
    if cache is None:
        cache = {}
    if gid in cache:
        return cache[gid]

    try:
        cache[gid] = grp.getgrgid(gid).gr_name
    except KeyError:
        cache[gid] = str(gid)

    return cache[gid]


def get_user(uid, cache=None):
    if cache is None:
        cache = {}
    if uid in cache:
        return cache[uid]

    try:
        cache[uid] = pwd.getpwuid(uid).pw_name
    except KeyError:
        cache[uid] = str(uid)

    return cache[uid]


def get_result_and_stat(path):
    def get_perms(mode):
        p = oct(mode & 0o1777)[-4:]
        if p[0] == '0':
            p = p[-3:]
        return p

    s = os.lstat(path)
    result = "{}, {}, {}, {}".format(get_perms(s.st_mode), get_group(s.st_gid), get_user(s.st_uid), path)
    return result, s


def get_file_info_for_installation():
    """
    ${FileInfo}=  Run Process  find  ${SOPHOS_INSTALL}  -type  f
    Should Be Equal As Integers  ${FileInfo.rc}  0  Command to find all files and permissions in install dir failed: ${FileInfo.stderr}
    Create File    ./tmp/NewFileInfo  ${FileInfo.stdout}
    ${FileInfo}=  Run Process  grep  -vFf  tests/installer/InstallSet/ExcludeFiles  ./tmp/NewFileInfo
    Create File    ./tmp/NewFileInfo  ${FileInfo.stdout}
    ${FileInfo}=  Run Process  cat  ./tmp/NewFileInfo  |  xargs  stat  -c  %a, %G, %U, %n  shell=True
    Create File    ./tmp/NewFileInfo  ${FileInfo.stdout}
    ${FileInfo}=  Run Process  sort  ./tmp/NewFileInfo

    And similar for directories and symlinks

    :return:
    """
    SOPHOS_INSTALL = get_sophos_install()
    exclusions = open(os.path.join(get_variable("ROBOT_TESTS_DIR"), "InstallSet/ExcludeFiles")).readlines()
    exclusions = set((e.strip() for e in exclusions))

    SOPHOS_INSTALL = os.path.join(SOPHOS_INSTALL, "plugins/eventjournaler")
    exclusions = set((e.strip() for e in exclusions))

    fullFiles = set()
    fullDirectories = [SOPHOS_INSTALL]

    for (base, dirs, files) in os.walk(SOPHOS_INSTALL):
        for f in files:
            include = True
            for e in exclusions:
                if e in f or e in base:
                    include = False
                    break
            if include:
                fullFiles.add(os.path.join(base, f))
        for d in dirs:
            include = True
            for e in exclusions:
                if e in d or e in base:
                    include = False
                    break
            if include:
                fullDirectories.append(os.path.join(base, d))

    details = []
    symlinks = []
    unixsocket = []

    for f in fullFiles:
        result, s = get_result_and_stat(f)
        if stat.S_ISLNK(s.st_mode):
            if not result.endswith('pyc'):
                symlinks.append(result)
        elif stat.S_ISSOCK(s.st_mode):
            unixsocket.append(result)
        else:
            if not result.endswith('pyc.0'):
                details.append(result)

    key = lambda x: x.replace(".", "").lower()
    symlinks.sort(key=key)
    details.sort(key=key)

    directories = []
    for d in fullDirectories:
        result, s = get_result_and_stat(d)
        directories.append(result)
    directories.sort(key=key)

    return "\n".join(directories), "\n".join(details), "\n".join(symlinks)


def get_systemd_file_info():
    """

    ${SystemdPath}  Run Keyword If  os.path.isdir("/lib/systemd/system/")  Set Variable   /lib/systemd/system/
    ...  ELSE  Set Variable   /usr/lib/systemd/system/
    ${SystemdInfo}=  Run Process  find  ${SystemdPath}  -name  sophos-spl*  -type  f  -exec  stat  -c  %a, %G, %U, %n  {}  +
    Should Be Equal As Integers  ${SystemdInfo.rc}  0  Command to find all systemd files and permissions failed: ${SystemdInfo.stderr}
    Create File  ./tmp/NewSystemdInfo  ${SystemdInfo.stdout}
    ${SystemdInfo}=  Run Process  sort  ./tmp/NewSystemdInfo

    :return:
    """

    systemd_path = "/usr/lib/systemd/system/"
    if os.path.isdir("/lib/systemd/system/"):
        systemd_path = "/lib/systemd/system/"

    fullFiles = []

    for (base, dirs, files) in os.walk(systemd_path):
        for f in files:
            if not f.startswith("sophos-spl"):
                continue
            p = os.path.join(base, f)
            if os.path.islink(p):
                continue
            fullFiles.append(p)

    results = []
    for p in fullFiles:
        result, s = get_result_and_stat(p)
        results.append(result)
    key = lambda x: x.replace(".", "").replace("-", "").lower()
    results.sort(key=key)

    return "\n".join(results)


# using the subprocess.PIPE can make the robot test to hang as the buffer could be filled up.
# this auxiliary method ensure that this does not happen. It also uses a temporary file in
# order not to keep files or other stuff around.
def run_proc_with_safe_output(args, shell=False):
    logger.debug('Run Command: {}'.format(args))
    with tempfile.TemporaryFile(dir=os.path.abspath('.')) as tmpfile:
        p = subprocess.Popen(args, stdout=tmpfile, stderr=tmpfile, shell=shell)
        p.wait()
        tmpfile.seek(0)
        output = tmpfile.read().decode()
        return output, p.returncode


def unmount_all_comms_component_folders(skip_stop_proc=False):
    def _umount_path(fullpath):
        stdout, code = run_proc_with_safe_output(['umount', fullpath])
        if 'not mounted' in stdout:
            return
        if code != 0:
            logger.info(stdout)

    def _stop_commscomponent():
        stdout, code = run_proc_with_safe_output(["/opt/sophos-spl/bin/wdctl", "stop", "commscomponent"])
        if code != 0 and not 'Watchdog is not running' in stdout:
            logger.info(stdout)

    if os.path.exists('/opt/sophos-spl/bin/wdctl'):

        # stop the comms component as it could be holding the mounted paths and
        # would not allow them to be unmounted.
        counter = 0
        while not skip_stop_proc and counter < 5:
            counter = counter + 1
            stdout, errcode = run_proc_with_safe_output(['pidof', 'CommsComponent'])
            if errcode == 0:
                logger.info("Commscomponent running {}".format(stdout))
                _stop_commscomponent()
                time.sleep(1)
            else:
                logger.info("Skip stop comms componenent")
                break

    dirpath = '/opt/sophos-spl/var/sophos-spl-comms/'

    mounted_entries = ['etc/resolv.conf', 'etc/hosts', 'usr/lib', 'usr/lib64', 'lib',
                       'etc/ssl/certs', 'etc/pki/tls/certs', 'etc/pki/ca-trust/extracted', 'base/mcs/certs',
                       'base/remote-diagnose/output']
    for entry in mounted_entries:
        try:
            fullpath = os.path.join(dirpath, entry)
            if not os.path.exists(fullpath):
                continue
            _umount_path(fullpath)
            if os.path.isfile(fullpath):
                os.remove(fullpath)
            else:
                shutil.rmtree(fullpath)
        except Exception as ex:
            logger.error(str(ex))


def get_delete_user_cmd():
    with open(os.devnull, "w") as devnull:
        if subprocess.call(["which", "deluser"], stderr=devnull, stdout=devnull) == 0:
            return "deluser"
        if subprocess.call(["which", "userdel"], stderr=devnull, stdout=devnull) == 0:
            return "userdel"


def get_delete_group_cmd():
    with open(os.devnull, "w") as devnull:
        if subprocess.call(["which", "delgroup"], stderr=devnull, stdout=devnull) == 0:
            return "delgroup"
        if subprocess.call(["which", "groupdel"], stderr=devnull, stdout=devnull) == 0:
            return "groupdel"


def does_group_exist(group=SOPHOS_GROUP):
    try:
        grp.getgrnam(group)
        return True
    except KeyError:
        return False


def does_user_exist(user):
    try:
        pwd.getpwnam(user)
        return True
    except KeyError:
        return False


def remove_user(delete_user_cmd, user):
    output, retCode = run_proc_with_safe_output([delete_user_cmd, user])
    if retCode != 0:
        logger.info(output)
        processes_to_kill = [{"pid": process.pid, "name": process.name()} for process in psutil.process_iter() if
                             process.username() == user]
        for process in processes_to_kill:
            logger.info(f"Killing {process['name']}")
            output, retCode = run_proc_with_safe_output(["kill", str(process["pid"])])
            if retCode != 0:
                logger.info(output)


def _get_file_content(file_path):
    if os.path.exists(file_path):
        with open(file_path, 'r') as file_handler:
            return file_handler.read()
    return ""


def uninstall_sspl(installdir=None):
    if installdir is None:
        installdir = get_sophos_install()
    uninstaller_executed = False
    counter = 0
    if os.path.isdir(installdir):
        p = os.path.join(installdir, "bin", "uninstall.sh")
        if os.path.isfile(p):
            try:
                contents, returncode = run_proc_with_safe_output(['bash', '-x', p, '--force'])
                uninstaller_executed = True
            except EnvironmentError as e:
                print("Failed to run uninstaller", e)
            if returncode != 0:
                logger.info(contents)

        while counter < 5 and os.path.exists(installdir):
            counter = counter + 1
            try:
                logger.info("try to rm all")
                unmount_all_comms_component_folders(True)
                output, returncode = run_proc_with_safe_output(['rm', '-rf', installdir])
                if returncode != 0:
                    logger.error(output)
                    time.sleep(1)
                    # try to unmount leftover tmpfs
                    if "Device or resource busy" in output:
                        output, returncode = run_proc_with_safe_output(
                            "mount | grep \"tmpfs on /opt/sophos-spl/\" | awk '{print $3}' | xargs umount", shell=True)
                        logger.info(output)
            except Exception as ex:
                logger.error(str(ex))
                time.sleep(1)

    # Attempts to uninstall based on the env variables in the sophos-spl .service file
    output, returncode = run_proc_with_safe_output(["systemctl", "show", "-p", "Environment", "sophos-spl"])
    installdir_string = "SOPHOS_INSTALL="
    installdir_name = "sophos-spl"
    if installdir_string in output:
        # Cuts the path we want out of the stdout
        start = output.find(installdir_string) + len(installdir_string)
        end = output.find(installdir_name) + len(installdir_name)
        installdir = output[start:end]
        logger.info(f"Installdir from service files: {installdir}")
        p = os.path.join(installdir, "bin", "uninstall.sh")
        if os.path.isfile(p):
            try:
                subprocess.call([p, "--force"])
            except EnvironmentError as e:
                print("Failed to run uninstaller", e)
        subprocess.call(['rm', '-rf', installdir])

    # Find delete user command, deluser on ubuntu and userdel on centos/rhel.
    delete_user_cmd = get_delete_user_cmd()
    logger.debug(f"Using delete user command:{delete_user_cmd}")

    # Find delete group command, delgroup on ubuntu and groupdel on centos/rhel.
    delete_group_cmd = get_delete_group_cmd()
    logger.debug(f"Using delete group command:{delete_group_cmd}")
    counter2 = 0
    time.sleep(0.2)
    while does_group_exist() and counter2 < 5:
        logger.info(f"removing group, it should have already been removed by now. Counter: {counter}")
        counter2 = counter2 + 1
        for user in ['sophos-spl-user', 'sophos-spl-network', 'sophos-spl-local']:
            if does_user_exist(user):
                remove_user(delete_user_cmd, user)

        out, code = run_proc_with_safe_output([delete_group_cmd, SOPHOS_GROUP])
        if code != 0:
            logger.info(out)

        time.sleep(0.5)
    if uninstaller_executed and (counter > 0 or counter2 > 0):
        logger.info(_get_file_content('/tmp/install.log'))
        message = f"Uninstaller failed to properly clean everything. Attempt to remove /opt/sophos-spl {counter} " \
                  f"attempt to remove group {counter2}"
        logger.warn(message)
        raise RuntimeError(message)


def require_uninstalled(*args):
    installdir = get_sophos_install()
    uninstall_sspl(installdir)
