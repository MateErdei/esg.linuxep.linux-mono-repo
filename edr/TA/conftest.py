import pytest
import sys

sys.path.append('/opt/test/inputs/test_scripts')
sys.path.append('/opt/test/inputs/test_scripts/Libs')

from Libs.fixtures.BaseMockService import BaseMockService, install_component, component_test_setup
from Libs.fixtures.EDRPlugin import EDRPlugin
from _pytest.runner import runtestprotocol
from pathlib import Path
import os
import shutil


def pytest_configure():
    pytest.sophos_install_location = ""


@pytest.fixture(scope="session")
def install_component_setup(tmpdir_factory):
    """
    Required to be executed once during the test, it copies the files from the component SDDS to the 'SOPHOS_INSTALL'
    a temporary directory setup by pytest, usually inside /var/tmp/pytest-of-xxxx/ (where xxxx is the hostname)

    :param tmpdir_factory:
    :return:
    """
    dir_path = tmpdir_factory.mktemp("sophos-spl")
    sophos_install = dir_path.strpath
    install_component(sophos_install)
    pytest.sophos_install_location = sophos_install
    return sophos_install


@pytest.fixture(scope="class")
def sspl_mock(install_component_setup, request):
    sophos_install = install_component_setup
    component_test_setup(sophos_install)
    component = BaseMockService(sophos_install)

    def fin():
        component.cleanup()

    request.addfinalizer(fin)
    return component

@pytest.fixture(scope="class")
def edr_plugin_instance(request):
    edr = EDRPlugin()

    def fin():
        edr.stop_edr()

    request.addfinalizer(fin)
    return edr


def collect_logs(test_name):
    """
    Copy all logs from the install directory
    """
    dir_to_export_logs_to = "/opt/test/logs/test_logs/{}".format(test_name)

    # Clear old logs if running via debug-loop
    shutil.rmtree(dir_to_export_logs_to)

    os.makedirs(dir_to_export_logs_to)
    for filename in Path(pytest.sophos_install_location).rglob('*.log'):
        print("Copying log file: {} to {}".format(filename, dir_to_export_logs_to))
        shutil.copy(filename, dir_to_export_logs_to)


def pytest_runtest_protocol(item, nextitem):
    reports = runtestprotocol(item, nextitem=nextitem)
    print("Current install location: {}".format(pytest.sophos_install_location))

    for report in reports:
        if report.when == 'call':
            # report.outcome will be "passed" if the test passed
            print("Result: {}, Test: {}".format(item.name, report.outcome))
            collect_logs(item.name)
    return True
