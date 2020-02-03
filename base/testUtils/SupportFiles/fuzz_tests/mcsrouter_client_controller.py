from kitty.controllers.client import ClientController
import psutil
import time

from setup_mcs_fakecloud import SetupMCSAndFakeCloud
from LogUtils import LogUtils

################# Controller Implementation #################
# Controller has the test suite setup and controls execution of each test case(fuzz)
#
#############################################################

class MCSFuzzerClientController(ClientController):

    def __init__(self, name='MyClientController', logger=None, restart_mcs=False, robot_run=True):
        '''
        :param name: name of the object
        :param restart_mcs, restart mcs for every test
        :param logger: logger for this object (default: None)
        '''
        super(MCSFuzzerClientController, self).__init__(name, logger)
        self._restart_mcs = restart_mcs
        self._mcs_router_process = None
        self._mcs_test_setup = SetupMCSAndFakeCloud(robot_run=robot_run)
        # generate before fakecloud attempts to use certificates
        self._mcs_test_setup.regenerate_certificates()
        self.run_once_flag = True
        self.mcsrouter_logs_to_check = ['Caught exception at top-level', 'mcsrouter already running']

    def trigger(self):
        '''
        Called for every test by the framework.
        Overridden to induce/trigger the customer to make a specific class of request to the server
        The request should be inline with stage that that the fuzzer inside the server is fuzzing for this test
        :return: None
        '''

        pass

    def setup(self):
        '''
        Implements test suite setup for mcsrouter fuzzing
        Called at the beginning of the fuzzing session
        '''
        self._mcs_test_setup.setup_mcs_tests_and_start_local_cloud_server()

        # check if mcs is running and stop it
        self._mcs_test_setup.stop_mcsrouter_and_clear_logs()
        self.logger.debug("set up complete")

    def get_process(self, proc_name, cmdline_str=''):
        for p in psutil.process_iter(attrs=['pid', 'name', 'cmdline']):
            if proc_name in p.info['name'] and cmdline_str in p.info['cmdline']:
                return p
        return

    def pre_test(self, test_number):
        '''
        Implements testcase setup for mcsrouter fuzzing
        Called at the beginning of each test. maybe used to restart the mcsrouter
        '''
        if self._restart_mcs or self.run_once_flag:
            self._stop_process()
            self._mcs_router_process = None

            self._mcs_test_setup.register_with_local_cloud_with_check()

            self._mcs_router_process = self._mcs_test_setup.start_mcsrouter_and_return_proc()
            self.run_once_flag = False
            assert (self._mcs_router_process)

        # call the super
        super(MCSFuzzerClientController, self).pre_test(test_number)


        # check current mcsrouter process is the same one we started
        mcs_proc = self.get_process('python', 'mcsrouter.mcs_router')
        if not (mcs_proc and self._mcs_router_process.pid == mcs_proc.info['pid']):
            self.report.failed("mcsrouter process has restarted")
        else:
            # add process information to the report
            self.report.add('process_id', self._mcs_router_process.pid)

    def post_test(self):
        '''
        Implements testcase teardown for mcsrouter fuzzing
        called at the end of each testcase.
        Also do custom checks to verify if test passed or failed
        :return None:
        '''
        if self._restart_mcs and self._mcs_router_process:
            self._stop_process()
        ## Make sure process started by us
        assert (self._mcs_router_process)

        self._check_logs()
        super(MCSFuzzerClientController, self).post_test()

    def teardown(self):
        '''
        Called at the end of the fuzzing session, override with victim teardown
        '''
        super(MCSFuzzerClientController, self).teardown()
        self._stop_process()
        self._mcs_router_process = None

    def _stop_process(self):
        if self._is_victim_alive():
            self._mcs_router_process.terminate()
            time.sleep(0.5)
            if self._is_victim_alive():
                self._mcs_router_process.kill()
                time.sleep(0.5)
                if self._is_victim_alive():
                    raise Exception('Failed to kill mcsrouter process')

        # check for any other instances of mcsrouter
        other_mcs_proc = self.get_process('python', 'mcsrouter.mcs_router')
        if other_mcs_proc and other_mcs_proc.is_running():
            other_mcs_proc.kill()
            raise Exception('Found another instance of mcsrouter not started by this test')

    def _is_victim_alive(self):
        mcs_is_alive = self._mcs_router_process and (self._mcs_router_process.poll() is None)
        self.logger.debug("kitty asking if msrouter is still running returned : {}".format(str(mcs_is_alive)))
        return mcs_is_alive

    def _check_logs(self):
        # check mcs logs for unwanted errors
        for loggedfailure in self.mcsrouter_logs_to_check:
            try:
                logUtil = LogUtils()
                logUtil.check_mcsrouter_log_does_not_contain(loggedfailure)
                self.report.add('logs', 'logs checked successfully')
            # add logged failures to the report
            except AssertionError as assertionError:
                self.run_once_flag = True
                msg = assertionError.message
                self.report.failed(msg)
                self.logger.error(msg)
