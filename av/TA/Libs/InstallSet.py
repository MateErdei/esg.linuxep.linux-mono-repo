
import os
import subprocess
try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

from robot.libraries.BuiltIn import BuiltIn


class InstallSet(object):
    def __init__(self):
        self.__m_install_set_verified = False

    def Create_Install_Set_If_Required(self):
        if self.__m_install_set_verified:
            return

        # Read variables
        base = BuiltIn().get_variable_value("${TEST_INPUT_PATH}")
        install_set = BuiltIn().get_variable_value("${COMPONENT_INSTALL_SET}")
        sdds_component = BuiltIn().get_variable_value("${COMPONENT_SDDS_COMPONENT}")
        subprocess.run(['ls', base])
        o = subprocess.check_output(['ls', base])
        logger.error("BASE = ", o)

        self.__m_install_set_verified = os.path.isdir(install_set)
