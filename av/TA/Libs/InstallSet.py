
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

    def create_install_set(self):
        base = BuiltIn().get_variable_value("${TEST_INPUT_PATH}")
        logger.error("BASE = %s" % base)
        install_set = BuiltIn().get_variable_value("${COMPONENT_INSTALL_SET}")
        logger.error("install_set = %s" % install_set)
        sdds_component = BuiltIn().get_variable_value("${COMPONENT_SDDS_COMPONENT}")
        logger.error("sdds_component = %s" % sdds_component)
        subprocess.run(['ls', base])
        o = subprocess.check_output(['ls', base])
        logger.error("BASE ls = %s" % o)

        if not os.path.isdir(install_set):
            self.create_install_set()

    def Create_Install_Set_If_Required(self):
        if self.__m_install_set_verified:
            return

        # Read variables
        install_set = BuiltIn().get_variable_value("${COMPONENT_INSTALL_SET}")
        if not os.path.isdir(install_set):
            self.create_install_set()

