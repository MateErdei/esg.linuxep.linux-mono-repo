import shutil
import os
import stat
import sys
import subprocess
import time
import threading
import logging



current_dir = os.path.dirname(os.path.abspath(__file__))
libs_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir, 'libs'))
cloud_auto_dir = os.path.abspath(os.path.join(current_dir, os.pardir, 'CloudAutomation'))
sys_test_dir = os.path.abspath(os.path.join(current_dir, os.pardir, os.pardir))

sys.path.insert(1, libs_dir)

from MCSRouter import MCSRouter, get_variable
from FullInstallerUtils import *
import CentralUtils


def setup_logging(name):
    # create logger
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)

    # create console handler and set level to debug
    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)

    # create formatter
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    # add formatter to ch
    ch.setFormatter(formatter)

    # add ch to logger
    logger.addHandler(ch)
    return logger


class SetupMCSAndFakeCloud(object):
    def __init__(self, robot_run, install_dir="/opt/sophos-spl", install_files_dir="tests/mcs_router/installfiles"):
        self._install_dir = install_dir
        self.install_files_dir = os.path.join(sys_test_dir, install_files_dir)
        self._plugin_reg_dir = os.path.join(self._install_dir, )
        self._mcs_router = MCSRouter()
        self.logger = setup_logging("mcs-fuzz-controller")
        self.mcsrouter_log = os.path.join(self._install_dir, 'base', 'sophosspl', 'mcsrouter.log')
        self.robot_test = robot_run

    def mcs_test_suite_setup(self):
        if not self.robot_test:
            self.require_fresh_install()
        self.stop_system_watchdog()

        self._mcs_router.check_for_existing_mcsrouter()
        self._mcs_router.cleanup_mcsrouter_directories()
        self._mcs_router.cleanup_local_cloud_server_logs()

    def require_fresh_install(self):
        Uninstall_SSPL()
        run_full_installer()
        verify_group_created()
        verify_user_created()

    # Stop System Watchdog
    def stop_system_watchdog(self):
        cmd = ['systemctl', 'stop', 'sophos-spl']
        output = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        stdout, stderr = output.communicate()
        self.logger.debug("stdout = {}, stderr = {} ".format(stdout, stderr))

    def setup_mcs_tests_and_start_local_cloud_server(self):
        # Setup MCS Tests and Start Local Cloud Server
        if not self.robot_test:
            self.mcs_test_suite_setup()

        # Setup MCS Tests Local Fake Cloud Server
        CentralUtils.Set_Local_CA_Environment_Variable()

    def stop_mcsrouter_and_clear_logs(self):
        self._mcs_router.stop_mcsrouter_if_running()
        if os.path.exists(self.mcsrouter_log):
            os.remove(self.mcsrouter_log)

    def start_mcsrouter_and_return_proc(self):
        self._mcs_router.start_mcsrouter()
        return self._mcs_router.mcsrouter

    def restart_mcsrouter_and_clear_logs(self):
        self.stop_mcsrouter_and_clear_logs()
        return self.start_mcsrouter_and_return_proc()

    def register_with_local_cloud_with_check(self):
        # Register With Local Cloud Server
        self._mcs_router.register_with_local_cloud_server()
        # Check Correct MCS Password And ID For Local Cloud Saved
        self._mcs_router.check_correct_mcs_password_and_id_for_local_cloud_saved()

    def regenerate_certificates(self):
        if not self.robot_test:
            generate_local_fake_cloud_certificates('cleanCerts')
            generate_local_fake_cloud_certificates('all')

    def run_test(self):
        # Register With Local Cloud Server
        self._mcs_router.register_with_local_cloud_server()
        # Check Correct MCS Password And ID For Local Cloud Saved
        self._mcs_router.check_correct_mcs_password_and_id_for_local_cloud_saved()
        self._mcs_router.start_mcsrouter()

    def stop(self):
        self._mcs_router.stop_local_cloud_server()
        # uninstall_sspl_unless_cleanup_disabled()


def generate_local_fake_cloud_certificates(action='all'):
    # Note RANDFILE env variable is required to create the certificates on amazon
    myenv = os.environ.copy()
    myenv['OPENSSL_CONF'] = '../https/openssl.cnf'
    myenv['RANDFILE'] = '.rnd'
    cmd = ['make', action]
    cwd = os.getcwd()
    os.chdir(cloud_auto_dir)
    output = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=myenv)
    os.chdir(cwd)
    stdout, stderr = output.communicate()
    if stderr:
        print(stderr.decode())

    if action == 'all':
        # You need execute permission on all folders up to the certificate file to use it in mcsrouter (lower priv user)
        # This makes the necessary folder on jenkins and amazon have the correct permissions. Ubuntu has a+x on home folders
        # by default.
        if os.path.exists('/home/jenkins'):
            os.chmod('/home/jenkins', stat.S_IXOTH)  # chmod a+x  /home/jenkins  #RHEL and CentOS
        if os.path.exists('/root'):
            os.chmod('/root', stat.S_IXOTH)  # chmod  a+x  /root          #Amazon

        assert os.path.exists(os.path.join(cloud_auto_dir, 'server-private.pem'))
        assert os.path.exists(os.path.join(cloud_auto_dir, 'root-ca.crt.pem'))
