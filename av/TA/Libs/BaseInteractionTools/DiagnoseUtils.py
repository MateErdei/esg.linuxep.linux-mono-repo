import subprocess
import os

BASE_PATH = os.path.join('/opt', 'sophos-spl')
# DIAGNOSE_PATH = os.path.join(BASE_PATH, 'bin', 'sophos_diagnose')


def start_diagnose(path_to_diagnose, output_folder):
    command = [path_to_diagnose, output_folder]
    log_path = os.path.join("/tmp", "diagnose.log")
    log = open(log_path, 'w')
    process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
    process.communicate()
    return process.returncode
