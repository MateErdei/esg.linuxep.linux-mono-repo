import grp
import os
import pwd
import shutil
import subprocess
from Libs.UserUtils import create_users_and_group

from Libs.FakeManagement import FakeManagement


def sdds():
    return "/opt/test/inputs/edr_sdds"

def run_shell(args, **kwargs):
    print('run command {}'.format(args))
    output = subprocess.check_output(' '.join(args), shell=True, stderr=subprocess.STDOUT, **kwargs)
    print(output)


def write_file(file_path, content):
    with open(file_path, 'w') as file_handler:
        file_handler.write(content)


def install_component(sophos_install):
    create_users_and_group()

    plugin_dir_path = os.path.join(sophos_install, 'plugins/edr')
    for rel_path in ['tmp', 'var/ipc', 'var/ipc/plugins', 'base/etc', 'base/mcs/response','base/mcs/action']:
        full_path = os.path.join(sophos_install, rel_path)
        os.makedirs(full_path, exist_ok=True)
    write_file(os.path.join(sophos_install, 'base/etc/logger.conf'), "VERBOSITY=DEBUG")
    run_shell(['sudo', 'groupadd', '-f', 'sophos-spl-group'])

    shutil.copytree(os.path.join(sdds(), 'files/plugins'), os.path.join(sophos_install, 'plugins'))
    component_tests_dir = os.path.join(sophos_install, 'componenttests')
    plugin_executable = os.path.join(plugin_dir_path, 'bin/edr')
    livequery_executable = os.path.join(plugin_dir_path, 'bin/sophos_livequery')
    osquery_executable = os.path.join(plugin_dir_path, 'bin/osqueryd')
    for dirn in ['var', 'log', 'etc', 'extensions']:
        os.makedirs(os.path.join(plugin_dir_path, dirn), exist_ok=True)

    shutil.copytree('/opt/test/inputs/componenttests', component_tests_dir)
    extensions_dir = os.path.join(sophos_install, 'plugins/edr/extensions/')
    DelayTable = 'DelayControlledTable'
    shutil.copy(os.path.join(component_tests_dir, DelayTable), os.path.join(extensions_dir, DelayTable))
    BinaryData = 'BinaryDataTable'
    shutil.copy(os.path.join(component_tests_dir, BinaryData), os.path.join(extensions_dir, BinaryData))

    for path in [plugin_executable, osquery_executable, livequery_executable]:
        run_shell(['chmod', '+x', path])
    run_shell(['chmod', '+x', os.path.join(component_tests_dir, '*')])
    run_shell(['chmod', '+x', os.path.join(extensions_dir, '*')])
    os.environ['SOPHOS_INSTALL'] = sophos_install
    os.makedirs(os.path.join(sophos_install, "plugins/edr/etc/osquery.conf.d"), exist_ok=True)
    os.makedirs(os.path.join(sophos_install, "plugins/edr/etc/query_packs"), exist_ok=True)
    shutil.copy("/opt/test/inputs/qp/sophos-scheduled-query-pack.conf", os.path.join(sophos_install, "plugins/edr/etc/query_packs/sophos-scheduled-query-pack.conf"))
    shutil.copy("/opt/test/inputs/qp/sophos-scheduled-query-pack.conf", os.path.join(sophos_install, "plugins/edr/etc/osquery.conf.d/sophos-scheduled-query-pack.conf"))


def component_test_setup(sophos_install):
    for rel_path in ['tmp', 'plugins/edr/log']:
        full_path = os.path.join(sophos_install, rel_path)
        shutil.rmtree(full_path, ignore_errors=True)
        os.makedirs(full_path)
    os.environ['SOPHOS_INSTALL'] = sophos_install


class BaseMockService():
    def __init__(self, root_path):
        self.sspl = root_path
        self.google_test_dir = os.path.join(root_path, 'componenttests')
        self.management = FakeManagement()
        self.management.start_fake_management()

    def cleanup(self):
        self.management.stop_fake_management()

