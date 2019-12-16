import pytest
import sys

sys.path.append('/opt/test/inputs/test_scripts')
sys.path.append('/opt/test/inputs/test_scripts/Libs')

from Libs.fixtures.BaseMockService import BaseMockService, install_component, component_test_setup
from Libs.fixtures.EDRPlugin import EDRPlugin


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
