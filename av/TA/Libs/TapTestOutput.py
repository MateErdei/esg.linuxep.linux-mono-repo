
import os
import subprocess
import zipfile

try:
    from robot.api import logger
    logger.warning = logger.warn
    from robot.libraries.BuiltIn import BuiltIn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)


class TapTestOutput:
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self):
        self.__m_unpacked = False

    def __set_tap_test_output_permissions(self, dest):
        os.chmod(os.path.join(dest, "SendThreatDetectedEvent"), 0o755)
        os.chmod(os.path.join(dest, "SendDataToSocket"), 0o755)
        os.chmod(os.path.join(dest, "safestore_print_tool"), 0o755)
        os.chmod(os.path.join(dest, "SafeStoreTapTests"), 0o755)
        BuiltIn().set_global_variable("${AV_TEST_TOOLS}", dest)
        self.__m_unpacked = True
        return dest

    def __unpack_if_newer(self, zippath, dest):
        stat1 = os.stat(zippath)
        try:
            stat2 = os.stat(dest)
            if stat1.st_mtime < stat2.st_mtime:
                return
        except EnvironmentError:
            pass
        logger.info(f"Unpacking {zippath} to {dest}")
        os.makedirs(dest, exist_ok=True)
        with zipfile.ZipFile(zippath) as z:
            for entry in z.infolist():
                logger.info(f"Extracting {entry.filename} from {zippath}")
                z.extract(entry, dest)

    def extract_tap_test_output(self):
        """
        Do more clever extraction based for Bazel builds

        Replaces:
     ${BUILD_ARTEFACTS_FOR_TAP} := ${TEST_INPUT_PATH}/tap_test_output_from_build
    ${result} =   Run Process    tar    xzf    ${BUILD_ARTEFACTS_FOR_TAP}/tap_test_output.tar.gz    -C    ${TEST_INPUT_PATH}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}
    Set Global Variable  ${AV_TEST_TOOLS}         ${TEST_INPUT_PATH}/tap_test_output

        :return: ${AV_TEST_TOOLS}
        """
        TEST_INPUT_PATH = BuiltIn().get_variable_value("${TEST_INPUT_PATH}")
        dest = TEST_INPUT_PATH + "/tap_test_output"

        if self.__m_unpacked:
            return self.__set_tap_test_output_permissions(dest)

        src = os.path.join(TEST_INPUT_PATH, "tap_test_output_from_build", "tap_test_output.tar.gz")
        zip_dbg_src = "/vagrant/esg.linuxep.linux-mono-repo/.output/av/linux_x64_dbg/tap_test_output.zip"
        zip_rel_src = "/vagrant/esg.linuxep.linux-mono-repo/.output/av/linux_x64_rel/tap_test_output.zip"

        if os.path.isfile(zip_dbg_src):
            self.__unpack_if_newer(zip_dbg_src, dest)
            logger.info(f"Bazel build - unpacked {zip_dbg_src} in {dest}")
            return self.__set_tap_test_output_permissions(dest)
        elif os.path.isfile(zip_rel_src):
            self.__unpack_if_newer(zip_rel_src, dest)
            logger.info(f"Bazel build - unpacked {zip_rel_src} in {dest}")
            return self.__set_tap_test_output_permissions(dest)
        elif os.path.isdir(dest):
            # assume already unpacked
            logger.info(f"Bazel build - files in {dest}")
            return self.__set_tap_test_output_permissions(dest)
        elif not os.path.isfile(src):
            logger.error("No src archive or unpacked directory for tap_test_output programs")
            logger.error(f"TEST_INPUT_PATH={TEST_INPUT_PATH}")
            logger.error(f"zip_rel_src={zip_rel_src}")
            logger.error(f"zip_dbg_src={zip_dbg_src}")
            logger.error(f"TEST_INPUT_PATH listing ={os.listdir(TEST_INPUT_PATH)}")
            raise AssertionError("Failed to unpack or find tap_test_output")

        # unpack the tarfile
        result = subprocess.run(["tar", "xzf", src, "-C", TEST_INPUT_PATH], stderr=subprocess.PIPE)
        assert result.returncode == 0, f"Failed to unpack tarfile {src}: ${result.stderr}"
        assert os.path.isdir(dest), f"No correct dest directory created from unpacking tarfile {src}: ${result.stderr}"
        return self.__set_tap_test_output_permissions(dest)

    def get_send_threat_detected_tool(self):
        AV_TEST_TOOLS = self.extract_tap_test_output()
        tool = os.path.join(AV_TEST_TOOLS, "SendThreatDetectedEvent")
        assert os.path.isfile(tool)
        os.chmod(tool, 0o755)
        return tool

    def get_send_data_to_socket_tool(self):
        AV_TEST_TOOLS = self.extract_tap_test_output()
        tool = os.path.join(AV_TEST_TOOLS, "SendDataToSocket")
        assert os.path.isfile(tool)
        os.chmod(tool, 0o755)
        return tool
