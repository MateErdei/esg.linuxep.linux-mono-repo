
import os
import shutil
import sys
import subprocess
try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

from robot.libraries.BuiltIn import BuiltIn


def download_supplements(dest):
    # ensure manual dir is on sys.path
    Libs = os.path.dirname(__file__)
    TA = os.path.dirname(Libs)
    manual = os.path.join(TA, "manual")
    assert os.path.isdir(manual)
    if manual not in sys.path:
        sys.path.append(manual)
    import downloadSupplements
    downloadSupplements.LOGGER = logger
    ret = downloadSupplements.run(dest)
    assert ret == 0


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
        logger.info("BASE = %s" % base)
        logger.info("install_set = %s" % install_set)
        sdds_component = BuiltIn().get_variable_value("${COMPONENT_SDDS_COMPONENT}")
        logger.info("sdds_component = %s" % sdds_component)

        vdl = os.path.join(base, "vdl")
        ml_model = os.path.join(base, "ml_model")
        local_rep = os.path.join(base, "local_rep")

        if not os.path.isdir(sdds_component):
            logger.error("Failed to find SDDS_COMONENT for INSTALL_SET: %s" % sdds_component)
            return

        for x in (vdl, ml_model, local_rep):
            if not os.path.isdir(x):
                download_supplements(base)
                assert os.path.isdir(x)

        o = subprocess.check_output(['ls', base])
        logger.info("BASE ls = %s" % o.decode("UTF-8"))

        o = subprocess.check_output(['ls', vdl])
        logger.info("VDL ls = %s" % o.decode("UTF-8"))

        o = subprocess.check_output(['ls', ml_model])
        logger.info("ml_model ls = %s" % o.decode("UTF-8"))

        o = subprocess.check_output(['ls', local_rep])
        logger.info("local_rep ls = %s" % o.decode("UTF-8"))

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
        self.__m_install_set_verified = self.verify_install_set(install_set)
        if self.__m_install_set_verified:
            return
        self.create_install_set(install_set)

