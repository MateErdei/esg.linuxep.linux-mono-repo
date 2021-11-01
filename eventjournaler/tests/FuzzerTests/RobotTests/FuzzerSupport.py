try:
    from robot.api import logger
except:
    import logging
    logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG)
    logger = logging.getLogger(__name__)

import subprocess
import os
import sys
import tempfile
import base64
import shutil
import pathlib

FuzzerRelativePath = "cmake-fuzz/tests/FuzzerTests/LibFuzzerScripts"
FuzzRelativePath = "cmake-afl-fuzz/tests/FuzzerTests/AflFuzzScripts"
TIMEOUTMINUTES=1

class FuzzerSupport:
    def __init__(self):
        self._tmp_dir = None
        self._ej_path = ""
        self._input_root_dir = ""

    def __del__(self):
        self._clean_tmp_dir()

    def _verify_paths(self):
        if not os.path.isdir(self._ej_path):
            raise AssertionError("Invalid path to event journaler source Code: {}".format(self._ej_path))
        if not os.path.isdir(self._input_root_dir):
            raise AssertionError("Invalid path to fuzz input (hint tests/FuzzerTests/data/) directory: {}".format(self._input_root_dir))

    def _clean_tmp_dir(self):
        if self._tmp_dir:
            logger.info("Cleaning up temp directory: {}".format(self._tmp_dir))
            shutil.rmtree(self._tmp_dir)
            self._tmp_dir = None

    def fuzzer_set_paths(self, tests_path):
        location_of_current_file = str(pathlib.Path(__file__).parent.absolute())
        logger.info("Setting paths")
        if location_of_current_file.endswith("tests/FuzzerTests/RobotTests"):
            self._ej_path = location_of_current_file[:-29]
        else:
            raise AssertionError("expected script to be in tests/FuzzerTests/RobotTests, it was in {}".format(location_of_current_file))

        self._input_root_dir = os.path.join(self._ej_path, tests_path)
        self._verify_paths()

    def _log_binary_file(self, file_path):
        """Log the binary file in a way that can be displayed in the robot report and reconstructed."""
        fileh = open(file_path, 'rb')
        file_content = fileh.read()
        fileh.close()
        base64_one = base64.encodestring(file_content)
        logger.info("Getting File: {}".format(file_path))
        logger.info("Decode it with: echo <content> | base64 --decode > file.bin")
        logger.info(base64_one)

    def _copy_to_filer6(self, filepaths, testname):
        filer6 = "/mnt/filer6/linux"
        fuzz_output_dir = "/mnt/filer6/linux/SSPL/fuzz_test_crash_inputs/"
        fuzz_test_output_dir = os.path.join(fuzz_output_dir, testname)

        if not os.path.ismount(filer6):
            popen = subprocess.Popen(["mount", filer6], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            output, stderror = popen.communicate()

            if output:
                logger.info(output)

            if stderror:
                logger.info(stderror)

            returncode = popen.returncode
            if returncode != 0:
                return returncode

        for filepath in filepaths:
            logger.info("copying file: {} to filer6".format(filepath))
            if not os.path.exists(fuzz_test_output_dir):
                output2 = subprocess.check_output(["sudo", "mkdir", fuzz_test_output_dir])
                logger.debug(output2)
            output2 = subprocess.check_output(["sudo", "cp", filepath, fuzz_test_output_dir])
            logger.debug(output2)

    def setup_base_build(self):
        logger.info("Setting up build")
        if os.path.exists("/vagrant"):
            logger.info("Not doing setup base build on vagrant as packages should be fetched on host machine")
            return
        try:
            output = subprocess.check_output(["tests/FuzzerTests/setup_inputs/gatherBuildInputs.sh"],
                                             cwd=self._ej_path)
            logger.debug(output)

            output2 = subprocess.check_output(["./build.sh", "--no-build"], cwd=self._ej_path)
            logger.debug(output2)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.warn(ex.output)

            raise AssertionError("Failed to setup for build")

        logger.info("setup for build completed")

    def ensure_afl_fuzzer_targets_built(self):
        error_message = "Failed to build AFLFuzzer Targets."
        if self._check_is_afl_fuzz_target_present():
            return
        fuzzer_path = os.path.join(self._ej_path, "tests/FuzzerTests/AflFuzzScripts")

        logger.info("Running buildFuzzTests.sh from {}".format(fuzzer_path))
        try:
            output = subprocess.check_output(["./buildFuzzTests.sh"], shell=True, cwd=fuzzer_path)
            logger.debug(output)
            if not self._check_is_afl_fuzz_target_present():
                raise AssertionError(error_message)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.warn(ex.output)
            raise AssertionError(error_message)

    def run_afl_fuzzer_by_name(self, target_name):
        self._verify_paths()
        fuzzer_path = os.path.join(self._ej_path, FuzzRelativePath, target_name)
        afl_exec_path = os.path.join(self._ej_path, "..", "afl-2.52b/afl-fuzz")
        fuzzer_input = os.path.join(self._input_root_dir, target_name)
        cmake_fuzz_dir = os.path.join(self._ej_path, FuzzRelativePath)
        finding_dir = os.path.join(self._ej_path, FuzzRelativePath, "findings_" + target_name)

        required_dir_to_run = "/tmp/base/etc"

        if not os.path.isdir(fuzzer_input):
            raise AssertionError("Input CORPUS not present. {}".format(fuzzer_input))
        if not os.path.exists(fuzzer_path):
            raise AssertionError( "Failed to find fuzzer target. Expected: {}".format(fuzzer_path))
        if not os.path.exists(finding_dir):
            os.makedirs(finding_dir)
        if not os.path.exists(required_dir_to_run):
            os.makedirs(required_dir_to_run)

        logger.info("Running: {}".format(fuzzer_path))

        default_timeout = str(8*60*60)
        timeout = os.environ.get('AFL_FUZZ_TIMEOUT', default_timeout)

        logger.info("timeout set to: {}".format(timeout))

        try:
            environment = os.environ.copy()
            environment['AFL_SKIP_CPUFREQ'] = '1'

            fuzz_log = open(os.path.join("/tmp", "fuzz.log"), 'w+')
            logger.info("Running from current dir: {}".format(cmake_fuzz_dir))

            args = ['timeout', timeout, afl_exec_path, '-i',
                    fuzzer_input, "-o", finding_dir, "-m", "400", fuzzer_path]
            logger.info("Command executed: {}".format(args))
            popen = subprocess.Popen(args, stdout=fuzz_log, stderr=subprocess.PIPE, env=environment, cwd=cmake_fuzz_dir)
            output, stderror = popen.communicate()
            returncode = popen.returncode
            if output:
                logger.info("output: {}".format(output.decode()[:500]))
            if stderror:
                logger.info("afl run stderror:{}".format(stderror.decode()))

            failures = self.check_fuzz_results(target_name)
            if failures:
                self._copy_to_filer6(failures, target_name)
                raise AssertionError("Fuzzer found error and reported the following files: {}".format(failures))

            if returncode != 124:
                return returncode
            return 0

        except Exception as ex:
            logger.warn("Fuzzer reported error {}".format(str(ex)))
            return 2

    def _check_is_afl_fuzz_target_present(self):
        files = ["fuzzRuneventpubsubtests.sh",
                 "fuzzRuneventpubsubtestsInVagrant.sh"]
        expected_fuzzer_path = os.path.join(self._ej_path, FuzzRelativePath)
        if not os.path.isdir(expected_fuzzer_path):
            logger.info("alc path does not exist: {}".format(expected_fuzzer_path))
            return False
        full_paths = [os.path.join(expected_fuzzer_path, filename) for filename in files]
        file_exist_list = [os.path.exists(filepath) for filepath in full_paths]
        if all(file_exist_list):
            files_present = [filepath for filepath in full_paths if os.path.exists(filepath)]
            return True
        else:
            files_present = [filepath for filepath in full_paths if os.path.exists(filepath)]
            logger.info("Files present in alc fuzzer path: {}".format(files_present))
            files_not_present = [filepath for filepath in full_paths if not os.path.exists(filepath)]
            logger.info("Files that were not present: {}".format(files_not_present))
            return False

    def check_fuzz_results(self, target_name):
        logger.info("Fuzzer finished. Checking results of {}".format(target_name))
        finding_dir = os.path.join(self._ej_path, FuzzRelativePath, "findings_" + target_name)
        fuzzer_path = os.path.join(self._ej_path, FuzzRelativePath, target_name)
        crash_dir = os.path.join(finding_dir,"crashes")
        hangs_dir = os.path.join(finding_dir,"hangs")
        if not os.path.exists(crash_dir) and not os.path.exists(hangs_dir) and os.path.exists(finding_dir):
            logger.warn("Neither crash nor hangs folder was created. Check if afl has really executed. ")
            return[]

        failures = []
        atleastOne = False

        for filename in os.listdir(crash_dir):
            logger.info("Checking file: {}".format(filename))
            atleastOne = True
            full_path = os.path.join(crash_dir, filename)
            self._log_binary_file(full_path)

            if not self._check_and_convict_afl_input_file(full_path, fuzzer_path):
                logger.warn("Confirmed file failed: {}".format(filename))
                failures.append(full_path)

        for filename in os.listdir(hangs_dir):

            atleastOne = True
            full_path = os.path.join(hangs_dir, filename)
            self._log_binary_file(full_path)

            if not self._check_and_convict_afl_input_file(full_path, fuzzer_path):
                logger.warn("Confirmed file failed: {}".format(filename))
                failures.append(full_path)

        if not atleastOne:
            logger.info("No file created after the execution of Fuzzer. All good!")

        return failures

    def _check_and_convict_afl_input_file(self, input_file, fuzzer):
        failed = False
        try:

            ps = subprocess.Popen(("cat", input_file), stdout=subprocess.PIPE)
            popen = subprocess.Popen(fuzzer, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                     stdin=ps.stdout, shell=True)
            popen.wait()

            returncode = popen.returncode
            output, stderr = popen.communicate()

            if output:
                out = "convict output:{}".format(output)
                logger.info(out)
            # fuzzer happens to use the stderr to print its output.
            if stderr:
                err = "convict stderror:{}".format(stderr)
                logger.info(err)
            if returncode != 0 and returncode != 2:
                failed = True
                logger.info("returncode of convict attempt: %s"%(returncode))

        except Exception as ex:
            failed = True
            logger.warning("Fuzzer reported error {}".format(str(ex)))
            output = str(ex)
            logger.debug(output)

        if failed:
            self._log_binary_file(input_file)
            return False
        return True
