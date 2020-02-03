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
import time

FuzzerRelativePath = "cmake-fuzz/tests/LibFuzzerTests"
FuzzRelativePath = "cmake-afl-fuzz/tests/FuzzTests"
TIMEOUTMINUTES=1


class FuzzerSupport( object):
    def __init__(self):
        self._tmp_dir = None
        self._everest_path = ""
        self._input_root_dir = ""
        self.mcs_fuzz_runner_process = None

    def __del__(self):
        self._clean_tmp_dir()

    def _verify_paths(self):
        if not os.path.isdir(self._everest_path):
            raise AssertionError("Invalid path to Base source Code: {}".format(self._everest_path))
        if not os.path.isdir(self._input_root_dir):
            raise AssertionError("Invalid path to everest-systemtest/SupportFiles/base_data/fuzz directory: {}".format(self._input_root_dir))

    def _clean_tmp_dir(self):
        if self._tmp_dir:
            logger.info("Cleaning up temp directory: {}".format(self._tmp_dir))
            shutil.rmtree(self._tmp_dir)
            self._tmp_dir = None

    def fuzzer_set_paths(self, base_path, tests_path):
        self._everest_path = base_path
        self._input_root_dir = tests_path
        self._verify_paths()
        self._everest_path = os.path.abspath(self._everest_path)
        self._input_root_dir = os.path.abspath(self._input_root_dir)

    def _check_is_fuzz_target_present(self):
        files = ["runFuzzerZMQTests.sh", "runFuzzerManagementAgentApiTest.sh", "ManagementAgentApiTest", "ZMQTests"]
        expected_fuzzer_path = os.path.join(self._everest_path, FuzzerRelativePath)
        if not os.path.isdir(expected_fuzzer_path):
            return False
        full_paths = [os.path.join(expected_fuzzer_path, filename) for filename in files]
        file_exist_list = [os.path.exists(filepath) for filepath in full_paths]
        return all(file_exist_list)

    def ensure_fuzzer_targets_built(self):
        error_message = "Failed to build libFuzzer Targets."
        if self._check_is_fuzz_target_present():
            return
        fuzzer_path = os.path.join(self._everest_path, "tests/LibFuzzerTests")
        logger.info("Running buildFuzzTests.sh from {}".format(fuzzer_path))
        try:
            output = subprocess.check_output(["./buildFuzzTests.sh"], shell=True, cwd=fuzzer_path)
            logger.debug(output)
            if not self._check_is_fuzz_target_present():
                raise AssertionError(error_message)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.warn(ex.output)
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
                os.makedirs(fuzz_test_output_dir)
            shutil.copy(filepath, fuzz_test_output_dir)

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

    def start_mcs_fuzzer(self, mcs_fuzzer_path, suite, range = 1):
        #range == 1 will run all mutations produced by fuzzer
        #range == 2 will run mutations/2 (half) and 3 will run a third
        environment = os.environ.copy()
        environment['ASAN_OPTIONS'] = 'detect_odr_violation=0'
        args = [sys.executable, mcs_fuzzer_path, "--suite", suite, "--range", str(range)]
        logger.info("Running: {}".format(args))
        self.mcs_fuzz_runner_process = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                                        env=environment)
        time.sleep(2)
        logger.info(self.mcs_fuzz_runner_process)
        pid = self.mcs_fuzz_runner_process.pid
        poll_return = self.mcs_fuzz_runner_process.poll()
        if poll_return:
            out, err = self.mcs_fuzz_runner_process.communicate()
            raise AssertionError("Failed to start fuzzed fake cloud server {}.\n Stdout: {} \n Stderr: {}".format(
                    poll_return, out.decode(), err.decode()))

    def wait_for_mcs_fuzzer(self):
        out,err= self.mcs_fuzz_runner_process.communicate()
        if self.mcs_fuzz_runner_process.returncode != 0:
            raise AssertionError("MCS Fuzzer found errors: {}\n{}".format(out.decode(), err.decode()))

    def kill_mcs_fuzzer(self):
        if self.mcs_fuzz_runner_process.returncode is None:
            self.mcs_fuzz_runner_process.terminate()

    def _run_fuzzer(self, fuzzer_path, arguments_as_list):
        args = [fuzzer_path]
        args += arguments_as_list
        failed = False
        try:
            environment = os.environ.copy()
            environment['ASAN_OPTIONS'] = 'detect_odr_violation=0'
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
        expected_fuzzer_path = os.path.join(self._everest_path, FuzzerRelativePath, target_name)
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
        self._run_fuzzer(expected_fuzzer_path, [input_folder,
                            expected_fuzzer_input,
                            '-max_total_time=' + str(max_time),
                            '-print_final_stats=1'])

        failures = self._check_and_convict_run_directory(expected_fuzzer_path)
        if failures:
            self._copy_to_filer6(failures, target_name)
            raise AssertionError("Fuzzer found error and reported the following files: {}".format(failures))


    def _check_is_alf_fuzz_target_present(self):
        files = ["fuzzRunparsealcpolicytestsInVagrant.sh",
                 "fuzzRunparsealcpolicytests.sh",
                 "fuzzRunsuldownloaderconfigtestsInVagrant.sh",
                 "fuzzRunsuldownloaderconfigtests.sh",
                 "fuzzRunsuldownloaderreporttestsInVagrant.sh",
                 "fuzzRunsuldownloaderreporttests.sh",
                 "parsealcpolicytests",
                 "suldownloaderconfigtests",
                 "suldownloaderreporttests"]
        expected_fuzzer_path = os.path.join(self._everest_path, FuzzRelativePath)
        if not os.path.isdir(expected_fuzzer_path):
            logger.info("alc path does not exist: {}".format(expected_fuzzer_path))
            return False
        full_paths = [os.path.join(expected_fuzzer_path, filename) for filename in files]
        file_exist_list = [os.path.exists(filepath) for filepath in full_paths]
        if all(file_exist_list):
            return True
        else:
            files_present = [filepath for filepath in full_paths if os.path.exists(filepath)]
            logger.info("Files present in alc fuzzer path: {}".format(files_present))
            files_not_present = [filepath for filepath in full_paths if not os.path.exists(filepath)]
            logger.info("Files that were not present: {}".format(files_not_present))
            return False

    def setup_base_build(self):
        logger.info("Setting up build")

        try:
            output = subprocess.check_output([sys.executable,
                                              "-m",
                                              "build_scripts.artisan_fetch",
                                              "build/release-package.xml"],
                                             cwd=self._everest_path)
            logger.debug(output)

            output2 = subprocess.check_output(["./build.sh", "--no-build"], cwd=self._everest_path)
            logger.debug(output2)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.warn(ex.output)

            raise AssertionError("Failed to setup for build")

        logger.info("setup for build completed")

    def ensure_alf_fuzzer_targets_built(self):
        error_message = "Failed to build AFLFuzzer Targets."
        if self._check_is_alf_fuzz_target_present():
            return
        fuzzer_path = os.path.join(self._everest_path, "tests/FuzzTests")

        logger.info("Running buildFuzzTests.sh from {}".format(fuzzer_path))
        try:
            output = subprocess.check_output(["./buildFuzzTests.sh"], shell=True, cwd=fuzzer_path)
            logger.debug(output)
            if not self._check_is_alf_fuzz_target_present():
                raise AssertionError(error_message)
        except subprocess.CalledProcessError as ex:
            logger.error("Failed to build target")
            logger.warn(ex.output)
            raise AssertionError(error_message)

    def run_alf_fuzzer_by_name(self, target_name):
        self._verify_paths()
        fuzzer_path = os.path.join(self._everest_path, FuzzRelativePath, target_name)
        afl_exec_path = os.path.join(self._everest_path, "..", "afl-2.52b/afl-fuzz")
        fuzzer_input = os.path.join(self._input_root_dir, target_name)
        cmake_fuzz_dir = os.path.join(self._everest_path, FuzzRelativePath)
        fuzzer_base_lib_path= os.path.join(self._everest_path, "libs")
        finding_dir = os.path.join(self._everest_path, FuzzRelativePath, "findings_" + target_name)

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

        # ensure the core_pattern is set to core as required by afl.
        with open('/proc/sys/kernel/core_pattern', 'w') as core_pattern:
            core_pattern.write('core\n')

        try:
            environment = os.environ.copy()
            environment['AFL_SKIP_CPUFREQ'] = '1'
            environment['LD_LIBRARY_PATH'] = fuzzer_base_lib_path

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
 

    def check_fuzz_results(self, target_name):
        logger.info("Fuzzer finished. Checking results of {}".format(target_name))
        finding_dir = os.path.join(self._everest_path, FuzzRelativePath, "findings_" + target_name)
        fuzzer_path = os.path.join(self._everest_path, FuzzRelativePath, target_name)
        crash_dir = os.path.join(finding_dir,"crashes")
        hangs_dir = os.path.join(finding_dir,"hangs")
        if not os.path.exists(crash_dir) and not os.path.exists(hangs_dir) and os.path.exists(finding_dir):
            logger.warn("Neither crash nor hangs folder was created. Check if alf has really executed. ")
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
            logger.warn("Fuzzer reported error {}".format(str(ex)))
            output = str(ex)
            logger.debug(output)

        if failed:
            self._log_binary_file(input_file)
            return False
        return True

    def dump_kittylogs_dir_contents(self):
        folder = os.path.abspath("./kittylogs/")
        if os.path.exists(folder):
            for filename in os.listdir(folder):
                filepath = os.path.join(folder, filename)
                if os.path.isfile(filepath):
                    logger.info(filepath)
                    logger.info(open(filepath, 'r').read())
