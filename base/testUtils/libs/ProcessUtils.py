import subprocess
import os
import shutil
import PathManager

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

