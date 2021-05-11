# Useful operating system utilities
import subprocess
import time


def command_available_on_system(command: str) -> bool:
    return subprocess.run(["which", command], stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode == 0


def os_uses_yum() -> bool:
    return command_available_on_system("yum")


def os_uses_apt() -> bool:
    return command_available_on_system("apt")


def is_package_installed(package_name: str) -> bool:
    print(f"Checking if package is installed: {package_name}")
    if os_uses_apt():
        output = subprocess.run(["apt", "list", "--installed", package_name],
                                text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT).stdout.split("\n")
        # Prints output similar to the below where we can see nano is installed.
        # nano/bionic,now 2.9.3-2 amd64 [installed]
        for line in output:
            if line.startswith(f"{package_name}/"):
                return True
        return False
    elif os_uses_yum():
        return subprocess.call(["yum", "--cacheonly", "list", "--installed", package_name]) == 0
    else:
        print("ERROR, could not determine whether machine uses apt or yum")


def get_pkg_manager() -> str:
    if os_uses_apt():
        package_manager = "apt"
    elif os_uses_yum():
        package_manager = "yum"
    else:
        raise AssertionError("Could not determine whether machine uses apt or yum")
    return package_manager


def install_package(pkg_name):
    package_manager = get_pkg_manager()
    for _ in range(60):
        if subprocess.run([package_manager, "-y", "install", pkg_name]).returncode == 0:
            return
        else:
            time.sleep(3)
    raise AssertionError(f"Could not install package: {pkg_name}")


def remove_package(pkg_name):
    package_manager = get_pkg_manager()
    for _ in range(60):
        if subprocess.run([package_manager, "-y", "remove", pkg_name]).returncode == 0:
            return
        else:
            time.sleep(3)
    raise AssertionError(f"Could not remove package: {pkg_name}")
