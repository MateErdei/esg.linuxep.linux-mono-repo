
import os
import shutil
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

    def susi_dir(self, install_set):
        return os.path.join(install_set, "files", "plugins", "av", "chroot", "susi", "distribution_version", "version1")

    def verify_install_set(self, install_set):
        if not os.path.isdir(install_set):
            return False

        dest = self.susi_dir(install_set)
        for x in "vdb", "mlmodel", "lrdata":
            if not os.path.isdir(os.path.join(dest, x)):
                return False

        return True

    def create_install_set(self, install_set):
        base = BuiltIn().get_variable_value("${TEST_INPUT_PATH}")
        logger.error("BASE = %s" % base)
        logger.error("install_set = %s" % install_set)
        sdds_component = BuiltIn().get_variable_value("${COMPONENT_SDDS_COMPONENT}")
        logger.error("sdds_component = %s" % sdds_component)

        vdl = os.path.join(base, "vdl")
        ml_model = os.path.join(base, "ml_model")
        local_rep = os.path.join(base, "local_rep")

        for x in (vdl, ml_model, local_rep, sdds_component):
            if not os.path.isdir(x):
                logger.error("Failed to find input for INSTALL_SET: %s" % x)

        o = subprocess.check_output(['ls', base])
        logger.error("BASE ls = %s" % o.decode("UTF-8"))

        o = subprocess.check_output(['ls', vdl])
        logger.error("VDL ls = %s" % o.decode("UTF-8"))

        o = subprocess.check_output(['ls', ml_model])
        logger.error("ml_model ls = %s" % o.decode("UTF-8"))

        o = subprocess.check_output(['ls', local_rep])
        logger.error("local_rep ls = %s" % o.decode("UTF-8"))

        # Copy SDDS-Component
        shutil.rmtree(install_set, ignore_errors=True)
        shutil.copytree(sdds_component, install_set)

        dest = self.susi_dir(install_set)
        vdl_dest = os.path.join(dest, "vdb")
        shutil.copytree(vdl, vdl_dest)
        shutil.copytree(ml_model, os.path.join(dest, "mlmodel"))
        shutil.copytree(local_rep, os.path.join(dest, "lrdata"))

        self.__m_install_set_verified = self.verify_install_set(install_set)

    def Create_Install_Set_If_Required(self):
        if self.__m_install_set_verified:
            return

        # Read variables
        install_set = BuiltIn().get_variable_value("${COMPONENT_INSTALL_SET}")
        if not self.verify_install_set(install_set):
            self.create_install_set(install_set)

