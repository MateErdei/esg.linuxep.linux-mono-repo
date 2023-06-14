# Useful operating system utilities

import os
import subprocess
import time
from robot.api import logger


def command_available_on_system(command: str) -> bool:
    return subprocess.run([b"which", command], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode == 0


def os_uses_yum() -> bool:
    return command_available_on_system("yum")


def os_uses_apt() -> bool:
    return command_available_on_system("apt")


def os_uses_zypper() -> bool:
    return command_available_on_system("zypper")


def zypper_env():
    env = os.environ.copy()
    env['ZYPP_LOCK_TIMEOUT'] = "600"
    return env


def one_time_zypper_setup():
    start = time.time()
    while start + 60 > time.time():
        result = subprocess.run(['systemctl', 'status', 'cloud-init'], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if b"active (exited)" in result.stdout:
            break
        else:
            logger.debug("cloud-init not finished: " + result.stdout.decode("UTF-8", errors="replace"))
        time.sleep(4)

    start = time.time()
    while start + 60 > time.time():
        result = subprocess.run(['zypper', '--non-interactive', 'refresh'], env=zypper_env(),
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode == 0:
            break
        else:
            logger.error("Failed to refresh zypper: " + result.stdout.decode("UTF-8", errors="replace"))
        time.sleep(4)


ONE_TIME_PACKAGE_MANAGER_SETUP_DONE = False


def one_time_package_manager_setup():
    global ONE_TIME_PACKAGE_MANAGER_SETUP_DONE
    if ONE_TIME_PACKAGE_MANAGER_SETUP_DONE:
        return

    if os_uses_zypper():
        one_time_zypper_setup()

    ONE_TIME_PACKAGE_MANAGER_SETUP_DONE = True


def is_package_installed(package_name: str) -> bool:
    one_time_package_manager_setup()
    logger.info(f"Checking if package is installed: {package_name}")
    if os_uses_apt():
        result = subprocess.run(["apt", "list", "--installed", package_name],
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        stdout = result.stdout
        lines = stdout.split(u"\n")
        # Prints output similar to the below where we can see nano is installed.
        # nano/bionic,now 2.9.3-2 amd64 [installed]
        for line in lines:
            if line.startswith(f"{package_name}/"):
                return True
        return False
    elif os_uses_yum():
        return subprocess.call(["yum", "--cacheonly", "list", "--installed", package_name]) == 0
    elif os_uses_zypper():
        return subprocess.call(["zypper", "search", "-s", package_name], env=zypper_env()) == 0
    else:
        logger.error("Could not determine whether machine uses apt, yum or zypper")


def get_pkg_manager():
    if os_uses_apt():
        package_manager = ["apt", "-y"]
    elif os_uses_yum():
        package_manager = ["yum", "-y"]
    elif os_uses_zypper():
        package_manager = ["zypper", "--non-interactive"]
    else:
        raise AssertionError("Could not determine whether machine uses apt or yum")
    return package_manager


def install_package(pkg_name):
    one_time_package_manager_setup()
    cmd = get_pkg_manager()
    cmd += ["install", pkg_name]
    logger.info(f"Running command: {cmd}")
    for _ in range(60):
        ret = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=zypper_env())
        if ret.returncode == 0:
            return
        else:
            logger.info(f"Command output: {ret.stdout}")
            time.sleep(3)
    raise AssertionError(f"Could not install package: {pkg_name}")


def remove_package(pkg_name):
    one_time_package_manager_setup()
    cmd = get_pkg_manager()
    cmd += ["remove", pkg_name]
    logger.info(f"Running command: {cmd}")
    for _ in range(60):
        ret = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, env=zypper_env())
        if ret.returncode == 0:
            return
        else:
            logger.info(f"Command output: {ret.stdout}")
            time.sleep(3)
    raise AssertionError(f"Could not remove package: {pkg_name}")


if __name__ == "__main__":
    print(is_package_installed("auditd"))
