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

FuzzerRelativePath = "cmake-fuzz/tests/FuzzerTests"

TIMEOUTMINUTES=1

class FuzzerSupport:
    def __init__(self):
        self._tmp_dir = None
        self._edr_path = ""
        self._input_root_dir = ""

    def __del__(self):
        self._clean_tmp_dir()

    def _verify_paths(self):
        if not os.path.isdir(self._edr_path):
            raise AssertionError("Invalid path to edr source Code: {}".format(self._edr_path))
        if not os.path.isdir(self._input_root_dir):
            raise AssertionError("Invalid path to fuzz input (hint tests/FuzzerTests/data/) directory: {}".format(self._input_root_dir))

    def _clean_tmp_dir(self):
        if self._tmp_dir:
            logger.info("Cleaning up temp directory: {}".format(self._tmp_dir))
            shutil.rmtree(self._tmp_dir)
            self._tmp_dir = None

    def fuzzer_set_paths(self, tests_path):
        location_of_current_file = str(pathlib.Path(__file__).parent.absolute())

        if location_of_current_file.endswith("tests/FuzzerTests/RobotTests"):
            self._edr_path = location_of_current_file[:-32]
        else:
            raise AssertionError("expected script to be in tests/FuzzerTests/RobotTests, it was in {}".format(location_of_current_file))

        self._input_root_dir = os.path.join(self._edr_path, tests_path)
        self._verify_paths()

    def _check_is_fuzz_target_present(self):
        files = ["runFuzzerLiveQueryTests.sh", "LiveQueryTests" ]
        expected_fuzzer_path = os.path.join(self._edr_path, FuzzerRelativePath)
        if not os.path.isdir(expected_fuzzer_path):
            return False
        full_paths = [os.path.join(expected_fuzzer_path, filename) for filename in files]
        file_exist_list = [os.path.exists(filepath) for filepath in full_paths]
        return all(file_exist_list)

    def ensure_fuzzer_targets_built(self):
        error_message = "Failed to build libFuzzer Targets."
        if self._check_is_fuzz_target_present():
            return
        fuzzer_path = os.path.join(self._edr_path, "tests/FuzzerTests")
        logger.info("Running buildFuzzTests.sh from {}".format(fuzzer_path))
        try:
            output = subprocess.check_output(["./buildFuzzTests.sh"], shell=True, cwd=fuzzer_path)
            logger.debug(output)
            if not self._check_is_fuzz_target_present():
                raise AssertionError(error_message)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.info(ex.output)
            raise AssertionError(error_message)

    def _get_max_time(self):
        #return the number of seconds to run the fuzzer
        return os.environ.get('FUZZTESTTIMEOUT', '60')

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


    def _check_and_convict_input_file(self, input_file, fuzzer):
        failed, output = self._run_fuzzer(fuzzer, [input_file])
        logger.debug(output)
        if failed:
            self._log_binary_file(input_file)
            return False
        return True

    def _check_and_convict_run_directory(self, fuzzer):
        failures = []
        atleastOne = False
        for filename in os.listdir(self._tmp_dir):
            atleastOne = True
            if filename.startswith("crash") or filename.startswith("slow") or filename.startswith("timeout"):
                logger.warn("DoubleChecking Fuzzer {} output File : {}".format(fuzzer, filename))
                full_path = os.path.join(self._tmp_dir, filename)
                self._log_binary_file(full_path)
                if not self._check_and_convict_input_file(full_path, fuzzer):
                    logger.warn("Confirmed file failed: {}".format(filename))
                    failures.append(full_path)
                else:
                    logger.info("File not convicted: {}".format(filename))
        if not atleastOne:
            logger.info("No file created after the execution of Fuzzer. All good!")
        return failures

    def _run_fuzzer(self, fuzzer_path, arguments_as_list):
        args = [fuzzer_path]
        args += arguments_as_list
        failed = False
        try:
            environment = os.environ.copy()
            environment['ASAN_OPTIONS'] = 'detect_odr_violation=0'
            environment['REDIST'] = os.path.join(self._edr_path, 'redist')
            logger.info("Running: {}".format(args))
            popen = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                             env=environment, cwd=self._tmp_dir)
            output, stderror = popen.communicate()
            returncode = popen.returncode

            if output:
                logger.info(output)

            output2 = popen.stdout
            if output2:
                logger.info(output2)

            # fuzzer happens to use the stderr to print its output.
            if stderror:
                logger.info(stderror)

            if returncode != 0:
                logger.info("returncode: " + str(returncode))
                failed = True

        except Exception as ex:
            failed = True
            logger.warn("Fuzzer reported error {}".format(str(ex)))
            output = str(ex)
        return failed, output

    def _set_temp_dir(self, tmp_dir):
        if self._tmp_dir:
            self._clean_tmp_dir()
        else:
            self._tmp_dir = tmp_dir

    def run_fuzzer_by_name(self, target_name):
        self._verify_paths()
        expected_fuzzer_path = os.path.join(self._edr_path, FuzzerRelativePath, target_name)
        expected_fuzzer_input = os.path.join(self._input_root_dir, target_name)
        if not os.path.isdir(expected_fuzzer_input):
            raise AssertionError("Input CORPUS not present. {}".format(expected_fuzzer_input))
        if not os.path.exists(expected_fuzzer_path):
            raise AssertionError("Failed to find fuzzer target. Expected: {}".format(expected_fuzzer_path))
        logger.info("Running: {}".format(expected_fuzzer_path))
        self._set_temp_dir(tempfile.mkdtemp())
        input_folder = os.path.join(self._tmp_dir, 'input')
        os.mkdir(input_folder)
        max_time = self._get_max_time()
        args = [input_folder,
                expected_fuzzer_input,
                '-max_total_time=' + str(max_time),
                '-print_final_stats=1']
        dict_file_path = os.path.join(self._edr_path, 'tests/FuzzerTests/',target_name + '.dict')
        if os.path.exists(dict_file_path):
            args.append('-dict={}'.format(dict_file_path))

        self._run_fuzzer(expected_fuzzer_path, args)

        failures = self._check_and_convict_run_directory(expected_fuzzer_path)
        if failures:
            self._copy_to_filer6(failures, target_name)
            raise AssertionError("Fuzzer found error and reported the following files: {}".format(failures))


    def setup_base_build(self):
        logger.info("Setting up build")
        if os.path.exists("/vagrant"):
            logger.info("Not doing setup base build on vagrant as packages should be fetched on host machine")
            return
        try:
            output = subprocess.check_output(["tests/FuzzerTests/setup_inputs/gatherBuildInputs.sh"],
                                             cwd=self._edr_path)
            logger.debug(output)

            output2 = subprocess.check_output(["./build.sh", "--no-build"], cwd=self._edr_path)
            logger.debug(output2)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.warn(ex.output)

            raise AssertionError("Failed to setup for build")

        logger.info("setup for build completed")

