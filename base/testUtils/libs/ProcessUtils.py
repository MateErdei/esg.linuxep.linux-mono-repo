import signal
import subprocess
import os
import shutil
import PathManager
import psutil
import time

class ProcessUtils(object):
    def spawn_sleep_process(self, process_name="PickYourPoison", run_from_location="/tmp"):
        pick_your_poison_path = os.path.join(PathManager.ROBOT_ROOT_PATH, "SystemProductTestOutput", "PickYourPoison")
        if not os.path.isfile(pick_your_poison_path):
            raise AssertionError("failed to find pick your poison executable")
        destination = os.path.join(run_from_location, process_name)
        shutil.copy(pick_your_poison_path, destination)


        process = subprocess.Popen([destination, "--run"])
        os.remove(destination)

        return process.pid, process_name, destination

    def get_process_memory(self, pid):
        # Deal with robot passing pid as string.
        pid = int(pid)
        process = psutil.Process(pid)

        # in bytes
        return process.memory_info().rss


    # Robot run process and start process don't seem to work well in some cases, they either interfere with each
    # other of just hang. So here are a couple simple run process functions to call from tests.
    def run_process_in_background(self, proc: str, *args) -> int:
        command = [proc]
        for arg in args:
            command.append(arg)
        process = subprocess.Popen(command)
        return process.pid

    def run_and_wait_for_process(self, proc: str, *args) -> int:
        command = [proc]
        for arg in args:
            command.append(arg)
        process = subprocess.Popen(command)
        return process.wait()

    def kill_process(self, pid, signal_to_send=signal.SIGINT):
        os.kill(pid, signal_to_send)
        wait_until = int(time.time()) + 30
        while psutil.pid_exists(pid) and int(time.time()) < wait_until:
            time.sleep(1)

        # kill the process if it hasn't finished gracefully
        if psutil.pid_exists(pid):
            print(f"The pid {pid} did not shutdown after sending a {signal_to_send} signal, sending SIGKILL")
            os.kill(pid, signal.SIGKILL)

