import os
import sys
import time

from kitty.interfaces import WebInterface
from kitty.model import GraphModel

from local_process import LocalProcessController
import livequery_command_template as lq_template
from push_server_target import PushServerTarget
from kitty.fuzzers import ServerFuzzer
import argparse
import logging

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)


from MCSRouter import MCSRouter
import ArgParserUtils

def run_push_fuzz_test(testsuite=None):
    parser = argparse.ArgumentParser(description='MCS_Router fuzz testing for SSPL Linux automation')
    parser.add_argument("--suite", default='livequery', type=str, \
                        help="Specify push command to fuzz", choices=['livequery', 'wakeup'])
    ArgParserUtils.add_mcs_fuzz_tests_args(parser)

    fuzzer_args, _ = parser.parse_known_args()

    fuzzer = ServerFuzzer(name="PushClientFuzzer", logger=logger)
    fuzzer.set_interface(WebInterface(host='0.0.0.0', port=26000))
    fuzzer.set_max_failures(fuzzer_args.max_failures)
    fuzzer.set_skip_env_test(True)

    target = PushServerTarget(name='PushServerTarget', logger=logger)

    mcs_router = MCSRouter()
    logs_path = os.path.join(mcs_router.tmp_path, "mcs_router.log")
    controller = LocalProcessController('PushClientController', mcs_router.router_path, ["--no-daemon", "--console", "-v"], log_file_path=logs_path)
    time.sleep(2)

    target.set_controller(controller)

    fuzzer.set_target(target)

    model = GraphModel()
    suite = testsuite if testsuite else fuzzer_args.suite
    fuzzer.logger.info("Setting up mcs fuzzer for suite = " + suite)
    if suite == "livequery":
        model.connect(lq_template.livequery_command)
    else:
        raise AssertionError("Unknown suite name only 'mcs', 'mdr' 'alc' 'livequery' are valid. Given suite {}".format(suite))

    fuzzer.set_model(model)
    fuzzer.set_range(end_index=model.num_mutations() / fuzzer_args.range)
    fuzzer.set_delay_between_tests(fuzzer_args.delay_between_tests)

    stage = fuzzer.model.get_template_info()['name']

    fuzzer.logger.info("Starting fuzz for stage : {}\n".format(stage))
    fuzzer.start()

    fuzzer.logger.info('-------------- done with fuzzing -----------------')
    fuzzer.logger.info("Finished fuzz for stage : {}\n".format(stage))
    fuzzer.stop()

    report = target.get_report()

    if report.get_status() == report.PASSED:
        return 0
    elif report.get_status() == report.FAILED or report.get_status() == report.ERROR:
        return 1

def main(argv):
    try:
        return run_push_fuzz_test()
    except Exception as e:
        logger.error("Fuzzer failed: {}".format(str(e)))
        raise e


if __name__ == '__main__':
    sys.exit(main(sys.argv))
