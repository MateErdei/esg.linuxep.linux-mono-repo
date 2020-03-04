import os
import sys
import time


from kitty.model import GraphModel
from kitty.fuzzers import ServerFuzzer
from kitty.interfaces import WebInterface

from local_process import LocalProcessController
import livequery_command_template as lq_template
from push_server_target import PushServerTarget

current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
sys.path.insert(1, libs_dir)


from MCSRouter import MCSRouter

mcs_router = MCSRouter()

def run_push_fuzz_test():
    lq_template.livequery_command

    fuzzer = ServerFuzzer(name="PushServerFuzzer")
    fuzzer.set_interface(WebInterface(host='0.0.0.0', port=26000))
    fuzzer.set_skip_env_test(True)
    target = PushServerTarget(name='PushServerTarget')

    controller = LocalProcessController('PushClientController', mcs_router.router_path, ["--no-daemon", "--console", "-v"])
    time.sleep(5)

    target.set_controller(controller)

    model = GraphModel()
    model.connect(lq_template.livequery_command)
    fuzzer.set_model(model)
    fuzzer.set_target(target)
    fuzzer.set_delay_between_tests(0.5)

    fuzzer.set_range(50)

    fuzzer.start()
    print('-------------- done with fuzzing -----------------')
    fuzzer.stop()


def main():
    try:
        run_push_fuzz_test()
    except Exception as e:
        mcs_router.stop_local_cloud_server()
        raise e
    mcs_router.stop_local_cloud_server()

if __name__ == '__main__':
    main()
