import grp
import os
import pwd
import shutil
import subprocess
import glob
import re

import logging
logger = logging.getLogger("BaseMockService")
logger.setLevel(logging.DEBUG)

from Libs.FakeManagement import FakeManagement

PLUGIN = "av"


def ret_if_dir(p):
    if os.path.isdir(p):
        return p
    return None


def output():
    OUTPUT = os.environ.get("OUTPUT", None)
    if OUTPUT is None:
        TA_DIR = os.environ.get("TA_DIR", None)
        if TA_DIR is None:
            fixtures = os.path.abspath(os.path.dirname(__file__))
            Libs = os.path.dirname(fixtures)
            TA_DIR = os.path.dirname(Libs)
        OUTPUT = os.path.join(TA_DIR, "..", "output")

    return (
            ret_if_dir(os.path.join("/opt/test/inputs", PLUGIN)) or
            ret_if_dir(OUTPUT) or
            None
    )


def sdds():
    OUTPUT = output()
    ## TODO: For production testing, we'll need to get supplements LINUXDAR-2442
    test = os.path.join(OUTPUT, "INSTALL-SET")
    if os.path.isdir(test):
        return test
    return os.path.join(OUTPUT, "SDDS-COMPONENT")


def base_sdds():
    return (
        os.environ.get("SSPL_BASE_SDDS", None) or
        os.path.join(output(), "base-sdds")
    )


def component_tests_src():
    return os.path.join(output(), 'componenttests')


def run_shell(args, **kwargs):
    print('run command {}'.format(args))
    output = subprocess.check_output(' '.join(args), shell=True, stderr=subprocess.STDOUT, **kwargs)
    print(output)


def write_file(file_path, content):
    with open(file_path, 'w') as file_handler:
        file_handler.write(content)


def create_user(user, group):
    if user not in pwd.getpwall():
        os.system('useradd {} -g {}'.format(user, group))


def create_group(group):
    if group not in grp.getgrall():
        os.system('groupadd {}'.format(group))


def create_users_and_group():
    create_group('sophos-spl-group')
    create_user('sophos-spl-user', 'sophos-spl-group')


COMPONENT_NAME = "av"


def create_library_symlinks(p):
    LIB_RE = re.compile(r"^(.*)\.\d+$")

    while True:
        mo = LIB_RE.match(p)
        if not mo:
            return

        os.symlink(os.path.basename(p), mo.group(1))
        p = mo.group(1)


def create_library_symlinks_from_glob(*p):
    p = os.path.join(*p)
    possible = glob.glob(p)
    possible.sort()
    p = possible[-1]
    create_library_symlinks(p)


def install_base(sophos_install):
    create_users_and_group()
    for rel_path in ['tmp', 'var/ipc', 'var/ipc/plugins', 'base/etc', 'base/mcs/action', 'base/mcs/policy', 'base/mcs/response', "base/lib64"]:
        full_path = os.path.join(sophos_install, rel_path)
        os.makedirs(full_path, exist_ok=True)

    write_file(os.path.join(sophos_install, 'base/etc/logger.conf'), "VERBOSITY=DEBUG")

    base_sdds_dir = base_sdds()
    base_files = os.path.join(base_sdds_dir, "files")
    base_libs = os.path.join(base_files, "base", "lib64")

    dest_libs = os.path.join(sophos_install, "base", "lib64")

    def copy_lib(lib_glob):
        src = glob.glob(os.path.join(base_libs, lib_glob))[0]
        dest = os.path.join(dest_libs, os.path.basename(src))
        shutil.copy(src, dest)
        create_library_symlinks(dest)
    #
    copy_lib("libstdc++.so.6.*")
    copy_lib("libcrypto.so.*")
    # copy_lib("liblog4cplus-2.0.so.*")
    # copy_lib("libprotobuf.so.*")
    # copy_lib("libzmq.so.*")


def install_component(sophos_install):
    # Ensure that appropriate libraries from base are present
    install_base(sophos_install)

    shutil.copytree(os.path.join(sdds(), 'files/plugins'), os.path.join(sophos_install, 'plugins'))
    plugin_dir_path = os.path.join(sophos_install, 'plugins', COMPONENT_NAME)
    plugin_lib64_path = os.path.join(plugin_dir_path, "lib64")
    for x in os.listdir(plugin_lib64_path):
        os.chmod(os.path.join(plugin_lib64_path, x), 0o755)

    create_library_symlinks_from_glob(os.path.join(plugin_lib64_path, "liblog4cplus-2.0.so.*"))
    create_library_symlinks_from_glob(os.path.join(plugin_lib64_path, "libprotobuf.so.*"))
    create_library_symlinks_from_glob(os.path.join(plugin_lib64_path, "libzmq.so.*"))
    create_library_symlinks_from_glob(os.path.join(plugin_lib64_path, "libsusi.so.*"))

    os.makedirs(os.path.join(plugin_dir_path, 'var'), exist_ok=True)
    os.makedirs(os.path.join(plugin_dir_path, 'log'), exist_ok=True)
    os.makedirs(os.path.join(plugin_dir_path, 'etc'), exist_ok=True)
    CHROOT = os.path.join(plugin_dir_path, 'chroot')
    os.makedirs(os.path.join(CHROOT, 'var'), exist_ok=True)
    os.makedirs(os.path.join(CHROOT, 'log'), exist_ok=True)

    CHROOT_LINK_DIR = os.path.join(CHROOT, plugin_dir_path[1:])
    os.makedirs(CHROOT_LINK_DIR, exist_ok=True)
    os.symlink("/", os.path.join(CHROOT_LINK_DIR, "chroot"))

    sbin = os.path.join(plugin_dir_path, 'sbin')
    for x in os.listdir(sbin):
        os.chmod(os.path.join(sbin, x), 0o700)

    os.environ['SOPHOS_INSTALL'] = sophos_install

    component_test_src_path = component_tests_src()
    if os.path.isdir(component_test_src_path):
        component_tests_dir = os.path.join(sophos_install, 'componenttests')
        shutil.copytree(component_test_src_path, component_tests_dir )
        run_shell(['chmod', '+x', os.path.join(component_tests_dir, '*')])


def component_test_setup(sophos_install):
    for rel_path in ['tmp', os.path.join('plugins', COMPONENT_NAME, 'log')]:
        full_path = os.path.join(sophos_install, rel_path)
        shutil.rmtree(full_path, ignore_errors=True)
        os.makedirs(full_path)
    os.environ['SOPHOS_INSTALL'] = sophos_install


class BaseMockService(object):
    def __init__(self, root_path):
        self.sspl = root_path
        self.google_test_dir = os.path.join(root_path, 'componenttests')
        self.management = FakeManagement()
        self.management.start_fake_management()

    def cleanup(self):
        logger.debug("Start cleanup")
        self.management.stop_fake_management()
        logger.debug("Finish cleanup")
