import os
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


def install_component(sophos_install):
    for rel_path in ['tmp', 'var/ipc', 'var/ipc/plugins', 'base/etc']:
        full_path = os.path.join(sophos_install, rel_path)
        os.makedirs(full_path, exist_ok=True)
    write_file(os.path.join(sophos_install, 'base/etc/logger.conf'), "VERBOSITY=DEBUG")
    run_shell(['sudo', 'groupadd', '-f', 'sophos-spl-group'])

    shutil.copytree(os.path.join(sdds(), 'files/plugins'), os.path.join(sophos_install, 'plugins'))
    plugin_lib64_path = os.path.join(sophos_install, 'plugins/edr/lib64')
    plugin_executable = os.path.join(sophos_install, 'plugins/edr/bin/edr')
    run_shell(['ldconfig', '-lN', '*.so.*'], cwd=plugin_lib64_path)
    run_shell(['chmod', '+x', plugin_executable])
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
        self.management = FakeManagement()
        self.management.start_fake_management()

    def cleanup(self):
        self.management.stop_fake_management()
