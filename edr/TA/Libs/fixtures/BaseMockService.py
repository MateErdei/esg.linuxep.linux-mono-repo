import grp
import os
import pwd
import shutil
import subprocess

from Libs.FakeManagement import FakeManagement


def sdds():
    return "/opt/test/inputs/edr/SDDS-COMPONENT"

def run_shell(args, **kwargs):
    print('run command {}'.format(args))
    output = subprocess.check_output(' '.join(args), shell=True, stderr=subprocess.STDOUT, **kwargs)
    print(output)


def write_file(file_path, content):
    with open(file_path, 'w') as file_handler:
        file_handler.write(content)


def create_user(user):
    if user not in pwd.getpwall():
        os.system('useradd {}'.format(user))


def create_group(group):
    if group not in grp.getgrall():
        os.system('groupadd {}'.format(group))


def create_users_and_group():
    create_user('sophos-spl-user')
    create_group('sophos-spl-group')


def install_component(sophos_install):
    create_users_and_group()

    plugin_dir_path = os.path.join(sophos_install, 'plugins/edr')
    for rel_path in ['tmp', 'var/ipc', 'var/ipc/plugins', 'base/etc', 'base/mcs/response']:
        full_path = os.path.join(sophos_install, rel_path)
        os.makedirs(full_path, exist_ok=True)
    write_file(os.path.join(sophos_install, 'base/etc/logger.conf'), "VERBOSITY=DEBUG")
    run_shell(['sudo', 'groupadd', '-f', 'sophos-spl-group'])

    shutil.copytree(os.path.join(sdds(), 'files/plugins'), os.path.join(sophos_install, 'plugins'))
    component_tests_dir = os.path.join(sophos_install, 'componenttests')
    shutil.copytree('/opt/test/inputs/edr/componenttests', component_tests_dir )
    plugin_lib64_path = os.path.join(plugin_dir_path, 'lib64')
    plugin_executable = os.path.join(plugin_dir_path, 'bin/edr')
    osquery_executable = os.path.join(plugin_dir_path, 'bin/osqueryd')
    os.makedirs(os.path.join(plugin_dir_path, 'var'), exist_ok=True)
    os.makedirs(os.path.join(plugin_dir_path, 'logs'), exist_ok=True)
    os.makedirs(os.path.join(plugin_dir_path, 'etc'), exist_ok=True)
    run_shell(['ldconfig', '-lN', '*.so.*'], cwd=plugin_lib64_path)
    run_shell(['chmod', '+x', plugin_executable])
    run_shell(['chmod', '+x', os.path.join(component_tests_dir, '*')])
    run_shell(['chmod', '+x', osquery_executable])
    os.environ['SOPHOS_INSTALL'] = sophos_install


def component_test_setup(sophos_install):
    for rel_path in ['tmp', 'plugins/edr/log']:
        full_path = os.path.join(sophos_install, rel_path)
        shutil.rmtree(full_path, ignore_errors=True)
        os.makedirs(full_path)
    os.environ['SOPHOS_INSTALL'] = sophos_install


class BaseMockService:
    def __init__(self, root_path):
        self.sspl = root_path
        self.google_test_dir = os.path.join(root_path, 'componenttests')
        self.management = FakeManagement()
        self.management.start_fake_management()

    def cleanup(self):
        self.management.stop_fake_management()
