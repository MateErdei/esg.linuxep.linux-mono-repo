import subprocess
import os

BASE_PATH = os.path.join('/opt', 'sophosspl')
DIAGNOSE_PATH = os.path.join(BASE_PATH, 'bin', 'sophos_diagnose')

def run_diagnose(pathTodiagnose, outputfolder):
    command = [DIAGNOSE_PATH, pathTodiagnose, outputfolder]
    log_path = os.path.join("/tmp", "diagnose.log")
    log = open(log_path, 'w+')
    process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
    process.communicate()
    return process.returncode
