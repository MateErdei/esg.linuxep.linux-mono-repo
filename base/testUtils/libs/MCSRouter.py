#!/usr/bin/env python
# -*- coding: utf-8 -*-


import subprocess
import sys
import os
import time
import http.client
import ssl
import shutil
import difflib
import xml.sax.saxutils
import xml.dom.minidom
import hashlib
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn
import robot.libraries.BuiltIn
import CentralUtils
import PathManager
import json

DUMMY_ALC_POLICY = """<?xml version="1.0"?><AUConfigurations xmlns:csc="com.sophos\msys\csc"><csc:Comp RevID="ba53ba11" policyType="1"/></AUConfigurations>"""
DUMMY_ALC_EVENT = """<?xml version="1.0" encoding="utf-8"?><event type="sophos.mgt.entityAppEvent" xmlns="http://www.sophos.com/EE/AUEvent"><timestamp>20180425 130943</timestamp><user><userId/><domain/></user><appInfo><number>{}</number><message><message_inserts><insert>Linux NextGen</insert><insert>sdds:SOPHOS</insert></message_inserts></message><updateSource>Sophos</updateSource></appInfo><entity><productId>SAVLINUX</productId><entityInfo xmlns="http://www.sophos.com/EntityInfo">AGENT:WIN:1.0.0</entityInfo></entity></event>"""
DUMMY_ALC_STATUS = """<?xml version="1.0" encoding="utf-8"?><status type="sau" xmlns="com.sophos\mansys\status"><CompRes Res="{}" RevID="86889a6c37de351406fd461f04cfd4eaf8c4cd408e0762ae5e33199759c74ccd" policyType="1" xmlns="com.sophos\msys\csc"/><autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd"><endpoint id="2aca040f82c2bf7b0766db544288a082"/></autoUpdate></status>"""
DUMMY_BLANK_XML = """<?xml version="1.0"?><stuff/>"""

DUMMY_EDR_RESPONSE = """{
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "durationMillis": 32,
        "sizeBytes": 0,
        "rows": 0,
        "errorCode": 0,
        "errorMessage": "OK"
    },
    "columnMetaData": [
        {"name": "pathname", "type": "TEXT"},
        {"name": "sophosPID", "type": "TEXT"},
        {"name": "start_time", "type": "BIGINT"}
    ]
}
"""

# Note that the RevID is changed below such that it will trigger a status message to be sent.
DUMMY_DEFAULT_MCS_POLICY = """<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
    <meta protocolVersion="1.1"/>
    <csc:Comp RevID="aaaaaaaaac21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>
    <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>PolicyRegToken</registrationToken>
    <servers>
        <server>https://localhost:4443/mcs</server>
    </servers>
    <messageRelays/>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <policyChangeServers/>
    </configuration>
</policy>"""

DUMMY_MCS_POLICY_WITH_NON_ASCII_CHARACTERS = r"""<�xml version='1.0'�>
<policy xmlns:csc�="com.sophos\msys\csc" type="mcs">
<meta protocolVersion="1.1"></meta>
<csc:Comp RevID="thisIsTheRevIdOfANonAsciiMcsPolicy中英字典" policyType="25"></csc:Comp>
<configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
<registrationToken>PolicyRegToken</registrationToken>
<servers>
<server>https://localhost:4443/mcs</server>
</servers>
<messageRelays></messageRelays>
<useSystemProxy>true</useSystemProxy>
<useAutomaticProxy>true</useAutomaticProxy>
<useDirect>true</useDirect>
<randomSkewFactor>1</randomSkewFactor>
<commandPollingDelay default="1"></commandPollingDelay>
<policyChangeServers></policyChangeServers>
</configuration>
</policy>
"""

DUMMY_MCS_POLICY = """<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
    <meta protocolVersion="1.1"/>
    <csc:Comp RevID="0e09e27cfc21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>
    <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>PolicyRegToken</registrationToken>
    <servers>
        <server>https://localhost:4443/mcs</server>
    </servers>
    <messageRelays>{}</messageRelays>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <policyChangeServers/>
    </configuration>
</policy>"""

DUMMY_MCS_POLICY_WITH_PROXY = """<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
    <meta protocolVersion="1.1"/>
    <csc:Comp RevID="0e09e27cfc21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>
    <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>PolicyRegToken</registrationToken>
    <servers>
        <server>https://localhost:4443/mcs</server>
    </servers>
    <proxies>
    <proxy>localhost:3000</proxy>
    </proxies>
    <proxyCredentials>
        <credentials>{}</credentials>
    </proxyCredentials>
    <messageRelays/>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>true</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <policyChangeServers/>
    </configuration>
</policy>"""

DUMMY_MCS_POLICY_WITH_DIRECT_DISABLED = """<?xml version="1.0"?>
<policy xmlns:csc="com.sophos\msys\csc" type="mcs">
    <meta protocolVersion="1.1"/>
    <csc:Comp RevID="0e09e27cfc21f8d7510d562c56e34507711bdce48bfdfd3846965f130fef142a" policyType="25"/>
    <configuration xmlns="http://www.sophos.com/xml/msys/mcspolicy.xsd" xmlns:auto-ns1="com.sophos\mansys\policy">
    <registrationToken>PolicyRegToken</registrationToken>
    <servers>
        <server>https://localhost:4443/mcs</server>
    </servers>
    <messageRelays/>
    <useSystemProxy>true</useSystemProxy>
    <useAutomaticProxy>true</useAutomaticProxy>
    <useDirect>false</useDirect>
    <randomSkewFactor>1</randomSkewFactor>
    <commandPollingDelay default="20"/>
    <policyChangeServers/>
    </configuration>
</policy>"""


def _update_xml_revid(xmlcontent):
    # No attempt is made to verify that assumptions are valid
    # xml is valid, it has an element csc:Comp and inside this element there is a RevID attribute.
    # If any of those assumptions are not valid, it will throw
    doc = xml.dom.minidom.parseString(xmlcontent)
    csc_comp = doc.getElementsByTagName('csc:Comp')[0]
    # blank revid, calculate hash, and re-set revid
    csc_comp.setAttribute('RevID', '')
    h = hashlib.sha256(doc.toxml().encode('utf-8'))
    csc_comp.setAttribute('RevID', h.hexdigest())
    return doc.toxml()


def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${%s}" % varName)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


class MCSRouter(object):

    ROBOT_LIBRARY_SCOPE = 'TEST SUITE'
    
    def __init__(self):
        self.support_path = PathManager.get_support_file_path()
        self.cloud_server_path = os.path.join(self.support_path, "CloudAutomation")
        self.tmp_path = os.path.join(".", "tmp")
        self.cloud_server_log = os.path.join(self.tmp_path, "cloudServer.log")
        self.cloud_std_err = self.cloud_server_log + '1'
        self.cloud_std_handler = open(self.cloud_std_err, 'w')
        self.local_mcs_token = "ThisIsARegToken"
        self.local_mcs_URL = "https://localhost:4443/mcs"
        self.local_proxy_port = 20000
        self.local_proxy_URL = "https://localhost:{}".format(self.local_proxy_port)
        self.cloud_server_process = None
        self.proxy = None
        self.mcsrouter = None
        self.mcsrouter_log = None
        self.devnull = open(os.devnull, 'w')
        self.default_mcs_policy=None
        self.alc_status_file = "ALC_status.xml"

        # Get Robot variables.
        self.sophos_install = get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
        self.register_central_path = get_variable("REGISTER_CENTRAL",
                                                  os.path.join(self.sophos_install, "base", "bin", "registerCentral"))
        self.router_path = get_variable("MCS_ROUTER", '/opt/sophos-spl/base/bin/mcsrouter')
        self.mcs_dir = get_variable("MCS_DIR", os.path.join(self.sophos_install, "base", "mcs"))
        self.mcs_router_runtime_folders = [os.path.join(self.mcs_dir, "action"),
                                           os.path.join(self.mcs_dir, "event"),
                                           os.path.join(self.mcs_dir, "policy"),
                                           os.path.join(self.mcs_dir, "status"),
                                           os.path.join(self.mcs_dir, "status", "cache"),
                                           os.path.join(self.sophos_install, "base", "etc"),
                                           os.path.join(self.sophos_install, "base", "etc", "sophosspl"),
                                           os.path.join(self.sophos_install, "logs", "base", "sophosspl"),
                                           os.path.join(self.sophos_install, "var", "cache", "mcs_fragmented_policies"),
                                           os.path.join(self.sophos_install, "var", "lock-sophosspl")]
        self.mcs_policy_time = None

    def __del__(self):
        self.cloud_std_handler.close()

    def _mcsrouter_exec_log(self):
        return os.path.join(self.tmp_path, 'mcsrouter_exec.log')

    def dump_mcs_router_dir_contents(self):
        listing = []
        for folder in self.mcs_router_runtime_folders:
            if os.path.isdir(folder):
                p = subprocess.Popen(["ls", "-lR", folder], stdout=subprocess.PIPE)
                (output, err) = p.communicate()
                directory_list = output.decode('utf-8')
                listing.append(directory_list + "\n")
        logger.info(''.join(listing))
        for folder in self.mcs_router_runtime_folders:
            if not os.path.isdir(folder):
                logger.info("{} is not a directory".format(folder))
                continue
            for filename in os.listdir(folder):
                filepath = os.path.join(folder, filename)
                if os.path.isfile(filepath):
                    logger.info(filepath)
                    logger.info(open(filepath, 'r').read())



    def dump_python_processes(self):
        p = subprocess.Popen(["ps", "-C", "python"], stdout=subprocess.PIPE)
        (python_processes, err) = p.communicate()
        logger.info("Python Processes:\n{}".format(python_processes))

    # Fake Cloud Utils
    def start_local_cloud_server(self, *args):
        self.ensure_fake_cloud_certs_generated()
        command = [sys.executable, os.path.join(self.cloud_server_path, "cloudServer.py")]
        self._launch_local_fake_cloud_process(command, args)
        self.__ensure_server_started(self.cloud_server_process, self.cloud_server_log, 'START')

    def _launch_local_fake_cloud_process(self, command, args):
        for arg in args:
            command.append(arg)
        logger.info(command)
        self.cloud_server_process = subprocess.Popen(command, stdout=self.devnull, stderr=self.cloud_std_handler)
        logger.info(self.cloud_server_process)
        pid = self.cloud_server_process.pid
        time.sleep(1)
        poll_return = self.cloud_server_process.poll()
        if poll_return:
            out, err = self.cloud_server_process.communicate()
            raise AssertionError("Failed to start server {}.\n Stdout: {} \n Stderr: {}".format(
                poll_return, out.decode(), err.decode()))

    def stop_local_cloud_server(self):
        logger.info(self.cloud_server_process)
        if self.cloud_server_process and self.cloud_server_process.returncode is None:
            pid = self.cloud_server_process.pid
            self.cloud_server_process.kill()
            self.cloud_server_process.wait()
            self.cloud_server_process = None
            logger.info("Local cloud server stopped: pid={}".format(pid))
        else:
            logger.warn("Local cloud server not running!")

    # Start fake Cloud server with kitty fuzzer
    def start_fuzzed_local_cloud_server(self, *args):
        command = [sys.executable, os.path.join(self.support_path, "fuzz_tests", "fuzzed_cloud_server.py")]
        self._launch_local_fake_cloud_process(command, args)

    def cleanup_local_cloud_server_logs(self):
        if os.path.exists(self.cloud_std_err):
            with open(self.cloud_std_err, 'r') as file_handler:
                cloud_std_err = file_handler.read()

            if cloud_std_err:
                logger.info("StdError associated with cloud Server: {}".format(cloud_std_err))
        server_log = os.path.join(self.tmp_path, "cloudServer.log")
        server_action_log = os.path.join(self.tmp_path, "cloudServer-action.log")
        for log in [server_log, server_action_log]:
            if os.path.exists(log):
                open(log, "w").close()

    def register_with_central(self, command=None):
        if command is None:
            command = CentralUtils.get_registration_command()
        args = command.split()  # Assume command never has any spaces in it - URL can't, Token doesn't
        if args[0] == "/opt/sophos-av/engine/registerMCS":
            args[0] = self.register_central_path

        if not os.path.isfile(args[0]):
            logger.error("{} doesn't exist: register will fail".format(args[0]))
            logger.error("{} {} {}".format((args[0] == "/opt/sophos-av/engine/registerMCS"), args[0], "/opt/sophos-av/engine/registerMCS"))

        logger.info("Register command = '{}'".format(args))
        class Result(object):
            pass

        result = Result()
        proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=self.devnull)
        result.stdout, result.stderr = proc.communicate()
        result.stdout_path = None
        result.stderr_path = None
        result.rc = proc.wait()
        logger.info("Register stdout: {}".format(result.stdout))
        logger.info("Register stderr: {}".format(result.stderr))
        if result.rc != 0:
            raise AssertionError("Command {} failed with {}".format(args,result.rc))
        return result

    def deregister_from_central(self):
        args = [self.register_central_path, "--deregister"]
        subprocess.check_call(args)

    def Fail_Register_With_Central_With_Bad_Token(self):
        command = CentralUtils.get_registration_command()
        args = command.split()  # Assume command never has any spaces in it - URL can't, Token doesn't
        if args[0] == "/opt/sophos-av/engine/registerMCS":
            args[0] = self.register_central_path

        # alter args[1] (Token)
        args[1] = args[1][:10]

        logger.info("Register command = '{}'".format(args))

        proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0]
        result = proc.wait()
        assert result != 0
        logger.info("Registration command output = '{}'".format(stdout))

    def register_with_local_cloud_server(self, token=None, url=None):
        if token is None:
            token = self.local_mcs_token

        if url is None:
            url = self.local_mcs_URL

        command = [self.register_central_path, token, url]
        logger.info("Register command = '{}'".format(command))
        subprocess.check_call(command, stdout=self.devnull)

    def fail_to_register_with_local_cloud_server(self, expected_returncode):
        expected_error = "Command '['/opt/sophos-spl/base/bin/registerCentral', 'ThisIsARegToken', 'https://localhost:4443/mcs']' returned non-zero exit status {}.".format(expected_returncode)
        try:
            self.register_with_local_cloud_server()
            if expected_returncode != "0":
                raise AssertionError("Expected to fail, but registration passed")
        except subprocess.CalledProcessError as error:
            print ("caught stuff")
            if not expected_error == str(error):
                print("Expected error: {} is not the same as the error: {}".format(expected_error, error))
                raise

    def register_with_new_token_local_cloud_server(self):
        self.register_with_local_cloud_server("A-NEW-TOKEN", self.local_mcs_URL)

    def register_with_local_cloud_server_with_different_directory(self, work_dir):
        command = [self.register_central_path, self.local_mcs_token, self.local_mcs_URL]
        subprocess.check_call(command, stdout=self.devnull, cwd=work_dir)

    def register_with_local_cloud_server_with_proxy_creds_and_message_relay(self, username, password, relay):
        command = [self.register_central_path, self.local_mcs_token, self.local_mcs_URL, "--proxycredentials", "{}:{}".format(username, password), "--messagerelay", relay]
        subprocess.check_call(command, stdout=self.devnull)

    def check_registration_directories(self):
        for folder in self.mcs_router_runtime_folders:
            if not os.path.exists(folder):
                raise AssertionError("MCS folder not created: {}".format(folder))

    def check_correct_mcs_password_and_id_for_local_cloud_saved(self):
        logger.info(self.cloud_server_process)
        
        if not self.is_mcs_config_present():
            raise AssertionError("No MCS Config - registration failed.")

        config_dict = self.read_mcs_config()
        expected_password = "ThisIsThePassword"
        expected_id = "ThisIsAnMCSID+1"

        if len(config_dict) == 0:
            self.__dump_mcs_config()
            raise AssertionError("MCS config empty - registration failed.")

        if config_dict.get("MCSPassword", None) != expected_password:
            self.__dump_mcs_config()
            raise AssertionError("MCSPassword is incorrect - registration failed.")

        if config_dict.get("MCSID", "")[:15] != expected_id:
            self.__dump_mcs_config()
            raise AssertionError("MCSID is incorrect - registration failed.")

    def register_different_working_directories(self, cwd):
        MCSRouter.register_with_local_cloud_server_with_different_directory(self, cwd)
        MCSRouter.check_correct_mcs_password_and_id_for_local_cloud_saved(self)
        MCSRouter.check_registration_directories(self)

    def check_last_live_query_response(self, expected_json):
        last_respone_path = os.path.join(self.tmp_path, "last_query_response.json")
        dict_expected = json.loads(expected_json)
        with open(last_respone_path, 'r') as f:
            dict_actual = json.loads(f.read())

        def compare_and_raise(expected, actual, compare_key):
            if expected[compare_key] != actual[compare_key]:
                raise AssertionError("Query failed for key {}. Expected {}. Actual {}".format(compare_key, expected, actual))

        for key in ['type', 'columnMetaData']:
            if key in dict_expected:
                compare_and_raise(dict_expected, dict_actual, key)
        expected_metadata = dict_expected['queryMetaData']
        actual_metadata = dict_actual['queryMetaData']

        for key in ['rows', 'errorCode', 'errorMessage']:
            if key in expected_metadata:
                compare_and_raise(expected_metadata, actual_metadata, key)

        if 'rows' in expected_metadata:
            len_actual = len(dict_actual['columnData'])
            len_expected = expected_metadata['rows']
            if len_actual != len_expected:
                raise AssertionError("The row count does not match, actual: {}, expected:{}".format(len_actual, len_expected))

        if 'columnData' in dict_actual:
            return dict_actual['columnData']
        return None

    def check_cloud_server_log_contains(self, expected, occurs, return_line_after=False, filter_line_start=""):
        import codecs
        server_log = os.path.join(self.tmp_path, "cloudServer.log")
        occurs = int(occurs)
        with codecs.open(server_log, "r", 'utf-8') as f:
            lines = f.readlines()
            for idx, line in enumerate(lines):
                if expected in line:
                    occurs -= 1
                    if occurs == 0:
                        if return_line_after:
                            line_after = lines[idx+1]
                            return line_after[line_after.find(filter_line_start) + len(filter_line_start):]
                        else:
                            return
            else:
                raise AssertionError("Expected cloud server log to contain {}".format(expected))

    def check_cloud_server_log_contains_pattern(self, expected, occurs=1):
        import codecs
        import re
        server_log = os.path.join(self.tmp_path, "cloudServer.log")
        occurs = int(occurs)
        with codecs.open(server_log, "r", 'utf-8') as f:
            for line in f.readlines():
                if re.match(expected, line):
                    occurs -= 1
                    if occurs == 0:
                        return
            else:
                raise AssertionError("Expected cloud server log to contain {}".format(expected))


    def check_cloud_server_log_for_update_event(self, code):
        self.check_cloud_server_log_contains("ALC event = " + DUMMY_ALC_EVENT.format(code), 1)
        
    def check_cloud_server_log_for_alc_status(self, res):
        self.check_cloud_server_log_contains(xml.sax.saxutils.escape(DUMMY_ALC_STATUS.format(res), {"\"":"&quot;"}), 1)
        
    def check_cloud_server_log_for_mcs_status(self, occurs):
        self.check_cloud_server_log_contains("Received status with appID: MCS", occurs)

    def check_cloud_server_log_for_mcs_status_and_return_it(self, occurs):
        return self.check_cloud_server_log_contains("Received status with appID: MCS", occurs, True, "MCS status = ")

    def check_cloud_server_log_for_default_statuses(self, occurs=1):
        self.check_cloud_server_log_contains("Received status with appID: APPSPROXY", occurs)
        self.check_cloud_server_log_contains("Received status with appID: AGENT", occurs)
        self.check_cloud_server_log_contains("Received status with appID: MCS", occurs)

    def compare_default_command_poll_interval_with_log(self, expected_interval, tolerance, ignore_get_num=0):
        server_log = os.path.join(self.tmp_path, "cloudServer.log")
        expected_interval = float(expected_interval)
        tolerance = float(tolerance)
        ignore_get_num = int(ignore_get_num)
        epoch_time = []
        log_of_gets = []
        with open(server_log, "r") as f:
            for line in f.readlines():
                if "GET - /mcs/commands/applications" in line:
                    if ignore_get_num > 0:
                        ignore_get_num -= 1
                        continue
                    log_of_gets.append(line)
                    timeStr = line.split(" ")[1].split(",")[0]
                    timeStruct = time.strptime(timeStr, "%H:%M:%S")
                    epoch_time.append(time.mktime(timeStruct))

        if len(epoch_time) < 2:
            raise AssertionError("Expected cloud server log to contain at least two command polls")

        differences = [epoch_time[i+1]-epoch_time[i] for i in range(len(epoch_time)-1)]
        average = (float(sum(differences)) / max(len(differences), 1))
        if abs(average - expected_interval) <= tolerance:
            logger.info("Command Polls:\n{}".format(log_of_gets))
            return
        else:
            logger.info("Command Polls:\n{}".format(log_of_gets))
            raise AssertionError("Expected default poll time to be {} instead it was {}".format(expected_interval, average))

    def send_policy(self, policy_type, data):
        headers = {"Content-type": "application/octet-stream", "Accept": "text/plain"}

        # Open HTTPS connection to fake cloud at https://127.0.0.1:4443
        conn = http.client.HTTPSConnection("127.0.0.1","4443", context=ssl._create_unverified_context())
        conn.request("PUT", "/controller/{}/policy".format(policy_type), data.encode('utf-8'), headers)
        conn.getresponse()
        conn.close()

    def send_policy_file(self, policy_type, policy_path):
        f = open(policy_path, 'r')
        self.send_policy(policy_type, f.read())

    def send_fake_alc_policy(self):
        data = DUMMY_ALC_POLICY
        self.send_policy("alc", data)

    def check_policy_written(self, expected_policy_name, expectedContent):
        alc_policy = os.path.join(self.mcs_dir, "policy", expected_policy_name)
        if os.path.exists(alc_policy):
            with open(alc_policy, "r") as f:
                contents = f.read()
            if contents != expectedContent:
                difference = ''.join( difflib.context_diff(contents, expectedContent, fromfile='expected', tofile='actually'))
                logger.info("Expected content: {}".format(expectedContent))
                logger.info("Current content: {}".format(contents))
                raise AssertionError("Differ\n" + difference)
            return
        raise AssertionError("{} policy not found".format(expected_policy_name))

    def check_fake_alc_policy_written(self):
        self.check_policy_written("ALC-1_policy.xml", DUMMY_ALC_POLICY)

    def check_policy_written_match_file(self, expected_policy_name, filepath ):
        expected_content = open(filepath,'r').read()
        self.check_policy_written(expected_policy_name, expected_content)

    def send_mcs_policy_with_new_message_relay(self, message_relay):
        data = DUMMY_MCS_POLICY.format(message_relay)
        self.send_policy("mcs", data)

    def send_default_mcs_policy(self):
        self.send_policy("mcs", DUMMY_DEFAULT_MCS_POLICY)

    def send_mcs_policy_with_non_ascii_characters(self):
        self.send_policy("mcs", DUMMY_MCS_POLICY_WITH_NON_ASCII_CHARACTERS)

    def send_mcs_policy_with_proxy(self, credentials):
        data = DUMMY_MCS_POLICY_WITH_PROXY.format(credentials)
        self.send_policy("mcs", data)

    def send_mcs_policy_with_direct_disabled(self):
        self.send_policy("mcs", DUMMY_MCS_POLICY_WITH_DIRECT_DISABLED)

    def send_cmd_to_fake_cloud(self, cmd, filename=None):
        conn = http.client.HTTPSConnection("127.0.0.1","4443", context=ssl._create_unverified_context())
        conn.request("GET", "/"+cmd)
        r2 = conn.getresponse()
        logger.info('Response ' + str(r2))
        conn.close()
        if filename is not None:
            with open(filename, "wb") as f:
                f.write(r2.read())

    def trigger_update_now(self):
        self.send_cmd_to_fake_cloud("action/updatenow")

    def queue_update_now(self, creation_time=None):
        path = "action/queueupdatenow"
        if creation_time is not None:
            path = path + "?creationtime=" + str(creation_time)
        self.send_cmd_to_fake_cloud(path)

    def trigger_scan_now(self):
        self.send_cmd_to_fake_cloud("action/scannow")

    def trigger_clear_nonascii(self):
        self.send_cmd_to_fake_cloud("action/clearNonAsciiAction")


    def set_mcs_policy_command_poll(self, interval):
        self.update_mcs_policy_and_send("commandPollingDelay default=\"5\"", "commandPollingDelay default=\"{}\"".format(interval))

    def set_mcs_policy_registration_token_empty(self):
        self.update_mcs_policy_and_send("PolicyRegToken", "")

    def set_default_mcs_policy(self):
        if self.default_mcs_policy is None:
            raise AssertionError("No default mcs policy stored!")
        self.send_policy("mcs", self.default_mcs_policy)

    def store_default_mcs_policy(self):
        mcs_policy_file = os.path.join(self.mcs_dir, "policy", "MCS-25_policy.xml")
        if os.path.exists(mcs_policy_file):
            with open(mcs_policy_file, "r") as f:
                self.default_mcs_policy = f.read()

    def update_mcs_policy_and_send(self, to_replace, with_replacement):
        mcs_policy_file = os.path.join(self.mcs_dir, "policy", "MCS-25_policy.xml")
        if os.path.exists(mcs_policy_file):
            with open(mcs_policy_file, "r") as f:
                file_content = f.read()
            file_content = file_content.replace(to_replace, with_replacement)
            new_xml_content = _update_xml_revid(file_content)
            self.send_policy("mcs", new_xml_content)
        else:
            raise AssertionError("MCS policy file missing")

    def send_event_file(self, appid, path2file):
        event_file = os.path.join(self.tmp_path, appid + "_event-001.xml")
        with open(event_file, "w") as f:
            f.write(open(path2file).read())
        target_path = os.path.join(self.mcs_dir, "event", appid + "_event-001.xml")
        print( target_path)
        shutil.move(event_file, target_path )

    def send_update_event(self, event_id):
        event_file = os.path.join(self.tmp_path, "ALC-event-001.xml")
        with open(event_file, "w") as f:
            f.write(DUMMY_ALC_EVENT.format(event_id))

        shutil.move(event_file, os.path.join(self.mcs_dir, "event", "ALC_event-001.xml"))

    def send_edr_response(self, app_id, correlation_id):
        file_name = "{}_{}_response.json".format(app_id, correlation_id)
        response_file = os.path.join(self.tmp_path, file_name)
        with open(response_file, "w") as f:
            f.write(DUMMY_EDR_RESPONSE)

        shutil.move(response_file, os.path.join(self.mcs_dir, "response", file_name))
        return DUMMY_EDR_RESPONSE
        
    def send_alc_status(self, res):
        status_file = os.path.join(self.tmp_path, self.alc_status_file)
        with open(status_file, "w") as f:
            f.write(DUMMY_ALC_STATUS.format(res))
        shutil.move(status_file, os.path.join(self.mcs_dir, "status", self.alc_status_file))

    def send_status_file(self, filename):
        status_file = os.path.join(self.tmp_path, self.alc_status_file)
        filepath = os.path.join(PathManager.get_support_file_path(), "CentralXml", filename)
        shutil.copy(filepath, status_file)
        shutil.move(status_file, os.path.join(self.mcs_dir, "status", self.alc_status_file))

    def fail_register(self, token, address):
        command = [self.register_central_path, token, address]
        proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        stdout = proc.communicate()[0]
        returncode = proc.wait()

        logger.info("Register output: {}".format(stdout))

        if returncode == 0:
            raise AssertionError("Registration was successful with {}".format(str(command)))

        return returncode


    # Registration / Deregistration
    def fail_register_with_junk_address(self, address):
        command = [self.register_central_path, self.local_mcs_token, address]
        class Result(object):
            pass

        result = Result()
        proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=self.devnull)
        result.stdout, result.stderr  = proc.communicate()
        result.stdout = result.stdout.decode("utf-8")
        result.stderr = result.stderr.decode("utf-8")
        result.stdout_path = None
        result.stderr_path = None
        result.rc = proc.wait()
        if result.rc == 0:
            raise AssertionError("Registration was successful with bad address")
        if result.rc != 4:
            raise AssertionError("Unexpected return code: {}".format(result.rc))
        return result

    def deregister(self):
        command = [self.register_central_path, "--deregister"]
        subprocess.check_call(command, stdout=self.devnull)

    def check_correct_mcs_password_and_id_for_deregister(self):
        config_dict = self.read_mcs_config()
        expected_password = ""
        expected_id = "reregister"
        if config_dict["MCSPassword"] != expected_password or config_dict["MCSID"][:15] != expected_id:
            logger.info("MCSPassword {}, MCSID {}".format(config_dict["MCSPassword"], config_dict["MCSID"]))
            raise AssertionError("MCSPassword or MCSID is incorrect - registration failed.")
    
    def fail_register_with_https_server(self, message):
        command = [self.register_central_path, self.local_mcs_token, self.local_proxy_URL]
        try:
            register_proc = subprocess.Popen(command, stdout=self.devnull, stderr=self.devnull)
        except Exception as e:
            raise AssertionError("Could not run registerCentral")
        # 10 retries - registerCentral could take a while as it reattempts connection 10 times.
        for i in range(10):
            register_log = os.path.join(self.sophos_install, "logs", "base", "register_central.log")
            if os.path.exists(register_log):
                with open(register_log, "r") as f:
                    log_contents = f.read()
                    for line in message.split("|"):
                        if line in log_contents:
                            register_proc.terminate()
                            return
            time.sleep(0.5)
        register_proc.terminate()
        raise AssertionError("Did not log {} in registration log".format(message))
    
    def fail_register_with_https_server_version_mismatch(self):
        self.fail_register_with_https_server("[SSL: WRONG_VERSION_NUMBER]|[SSL: WRONG_SSL_VERSION]")
    
    def fail_register_with_https_server_gateway_error(self):
        self.fail_register_with_https_server("Bad Gateway")
        
    def fail_register_with_https_server_certificate_verify_failed(self):
        self.fail_register_with_https_server("[SSL: CERTIFICATE_VERIFY_FAILED]")

    # MCS Router
    def start_mcsrouter(self):
        command = [self.router_path, "--no-daemon", "--console", "-v"]
        self.mcsrouter_log = open(self._mcsrouter_exec_log(), 'w')
        self.mcsrouter = subprocess.Popen(command, stdout=self.mcsrouter_log, stderr=self.mcsrouter_log)

    def _report_startup_errors(self):
        try:
            with open(self._mcsrouter_exec_log(), 'r') as f:
                out = f.read()
                if out:
                    logger.info('mcsrouter finished: {}'.format(out))
        except IOError:
            pass
        except Exception as ex:
            logger.warn("On printing mcsrouter logs files: {}".format(ex))

    def stop_mcsrouter_if_running(self):
        if self.mcsrouter:
            self.dump_python_processes()
            logger.info("Killing mcsrouter with pid {}".format(int(self.mcsrouter.pid)))
            self.mcsrouter.terminate()
            if self.mcsrouter_log:
                self.mcsrouter_log.close()
                self._report_startup_errors()
                self.mcsrouter_log = None
            self.mcsrouter = None
    
    def check_for_existing_mcsrouter(self):
        p = subprocess.Popen(["ps", "-ef"], stdout=subprocess.PIPE)
        (processes, err) = p.communicate()
        output = processes.decode('utf-8')
        for process in output.split("\n"):
            if "mcs_router.py" in process and "python" in process:
                logger.warn("mcsrouter is running before the tests have started")

    # Message Relay / Proxy Utils
    def start_message_relay(self):
        relay_log_file_path = os.path.join(self.tmp_path, "relay.log")
        relay_log_file = open(relay_log_file_path, "w")
        command = [sys.executable, os.path.join(self.support_path, "https_proxy.py"), str(self.local_proxy_port)]
        self.proxy = subprocess.Popen(command, stdout=relay_log_file, stderr=subprocess.STDOUT)
            
    def cleanup_proxy_logs(self):
        message_relay_log = os.path.join(self.tmp_path, "relay.log")
        proxy_log = os.path.join(self.tmp_path, "proxy.log")
        for log in [message_relay_log, proxy_log]:
            if os.path.exists(log):
                os.unlink(log)
    
    def stop_proxy_if_running(self):
        if self.proxy:
            self.proxy.kill()
            self.proxy = None
            
    def start_https_server(self, protocol):
        proxy_log_file_path = os.path.join(self.tmp_path, "proxy.log")
        proxy_log_file = open(proxy_log_file_path, "w")
        cert_location = os.path.join(self.cloud_server_path, "server-private.pem")
        command = [sys.executable, os.path.join(self.support_path, "https_proxy.py"), str(self.local_proxy_port), protocol, "--certfile", cert_location]
        self.proxy = subprocess.Popen(command, stdout=proxy_log_file, stderr=subprocess.STDOUT)
        self.__ensure_server_started(self.proxy, proxy_log_file_path, 'Serving HTTP Proxy')
        
    def switch_to_original_cacert(self):
        try:
            os.makedirs(os.path.join(self.sophos_install, "base", "etc"))
        except EnvironmentError:
            pass
        config_path = os.path.join(self.sophos_install, "base", "etc", "mcsrouter.conf")
        with open(config_path, "w") as f:
            f.write("CAFILE={}".format(os.path.join(self.mcs_dir, "certs", "mcs_rootca.crt.bk")))

    # Utils
    def mcs_config_path(self):
        return os.path.join(self.sophos_install, "base", "etc", "sophosspl",  "mcs.config")

    def __dump_mcs_config(self):
        p = self.mcs_config_path()
        logger.info(p)
        try:
            data = open(p).read()
            logger.info(data)
        except EnvironmentError:
            logger.info("Can't read {}".format(p))

    def read_mcs_config(self):
        path = self.mcs_config_path()
        config_dict = {}
        if os.path.exists(path):
            with open(path, "r") as f:
                for line in f.readlines():
                    line = line.strip()
                    if "=" in line:
                        (key, value) = line.split("=", 1)
                        config_dict[key] = value
        return config_dict

    def check_mcs_config_not_present(self):
        config_dict = self.read_mcs_config()
        if config_dict != {}:
            raise AssertionError("MCS Config File has been written")

    def is_mcs_config_present(self):
        return os.path.isfile(self.mcs_config_path())

    def cleanup_mcsrouter_directories(self):
        for full_path in self.mcs_router_runtime_folders:
            if full_path.endswith("/etc"):
                continue
            if os.path.isdir(full_path):
                for filename in os.listdir(full_path):
                    filename_path = os.path.join(full_path, filename)
                    if os.path.isfile(filename_path):
                        logger.info("Remove file: {}".format(filename_path))
                        os.remove(filename_path)

    def check_file_exists(self, path):
        return os.path.exists(path)

    def check_action_file_exists(self, filename):
        action_dir = os.path.join(self.mcs_dir, "action")
        action_file_path = os.path.join(action_dir, filename)
        if not self.check_file_exists(action_file_path):
            list_of_files = ', '.join(os.listdir(action_dir))
            raise AssertionError("Action file does not exist at {}. Current files: {}".format(action_file_path, list_of_files))

    def policypath(self, policyname):
        return os.path.join(self.mcs_dir, "policy", policyname)

    def get_policy_hash(self, policyname):
        policy_file_path = self.policypath(policyname)
        return hashlib.sha256(open(policy_file_path).read()).hexdigest()

    def check_policy_files_exists(self, *args):
        for filename in args:
            policy_file_path = os.path.join(self.mcs_dir, "policy", filename)
            if not self.check_file_exists(policy_file_path):
                raise AssertionError("Policy file does not exist at {}".format(policy_file_path))

    def check_policy_files_exists_and_hash_is_different(self, policyname, previoushash):
        policy_file_path = self.policypath(policyname)
        if not self.check_file_exists(policy_file_path):
            raise AssertionError("Policy file does not exist at {}".format(policy_file_path))

        currenthash = self.get_policy_hash(policyname)
        if currenthash == previoushash:
            raise AssertionError("Policy file has previous hash at {} - {}".format(policy_file_path, currenthash))
                
    def check_event_directory_empty(self):
        event_folder_path = os.path.join(self.mcs_dir, "event")
        if os.path.exists(event_folder_path):
            if os.listdir(event_folder_path) != []:
                raise AssertionError("Event files still exist in {}".format(event_folder_path))
        else:
            raise AssertionError("Event folder doesn't exist.")
    
    def check_temp_folder_doesnt_contain_atomic_files(self, base=None):
        tmp_dir = os.path.join(self.sophos_install, "tmp")
        dir_contents = os.listdir(tmp_dir)

        # Filter out files we expect in tmp from installer
        if base is None:
            dir_contents = [ f for f in dir_contents if f not in ("addedFiles", "removedFiles", "changedFiles", "policy")]
        else:
            dir_contents = [ f for f in dir_contents if f.startswith(base)]
        only_files = [f for f in dir_contents if os.path.isfile(os.path.join(tmp_dir, f))]

        if len(only_files) != 0:
            raise AssertionError("Temp folder is not empty! {}".format(only_files))

    def check_temp_policy_folder_doesnt_contain_policies(self, base=None):
        dir_contents = os.listdir(os.path.join(self.sophos_install, "tmp", "policy"))

        if len(dir_contents) != 0:
            raise AssertionError("Temp folder is not empty! {}".format(dir_contents))


    def check_envelope_log_contains(self, expected, occurs=1):
        import codecs
        log_file = os.path.join( self.sophos_install, 'logs/base/sophosspl/mcs_envelope.log')
        occurs = int(occurs)
        with codecs.open(log_file, "r", 'utf-8') as f:
            for line in f.readlines():
                if expected in line:
                    occurs -= 1
                    if occurs == 0:
                        return
            else:
                raise AssertionError("Expected envelope log to contain {}".format(expected))


    def __ensure_server_started(self, prochandler, logfilepath, markToFind):
        lastContent = ""
        for i in range(10):
            if prochandler.poll():
                raise AssertionError("Server has failed to start: {}".format(prochandler.returncode))
            if os.path.exists(logfilepath):
                with open(logfilepath, "r") as log:
                    lastContent = log.read()
                    if markToFind in lastContent:
                        logger.info("Server started and producing logs {}".format(os.path.abspath(logfilepath)))
                        os.path.abspath('.')
                        return
            time.sleep(1)
        raise AssertionError("Server has failed to start correctly. Log: {}".format(lastContent))

    def wait_new_mcs_policy_downloaded(self, timeout = 30):
        """Monitor policy folder for a new mcs policy to arrive up to timeout seconds"""
        future_time = time.time() + timeout
        mcs_policy_path = os.path.join(self.sophos_install, 'base/mcs/policy/MCS-25_policy.xml')
        while future_time > time.time():
            try:
                created_time = os.stat(mcs_policy_path).st_ctime
                if self.mcs_policy_time:
                    if created_time > self.mcs_policy_time:
                        # file is more recent than last time it was checked.
                        self.mcs_policy_time = created_time
                        return
                else:
                    # first time ever the file was downloaded. Update mcs_policy_time and return
                    self.mcs_policy_time = created_time
                    return
            except OSError:
                # policy file is not present is not an error. Just means not downloaded yet.
                pass

            time.sleep(1)
        raise AssertionError("Timeout waiting for new policy")

    def send_blank_xml_policy(self):
        self.send_policy("mcs", DUMMY_BLANK_XML)

    def break_and_send_mcs_policy(self, target_xml, broken_xml):
        broken_policy = DUMMY_DEFAULT_MCS_POLICY.replace(target_xml, broken_xml)
        self.send_policy("mcs", broken_policy)

    def get_cmdline(self, pid):
        p = os.path.join("/proc", pid, "cmdline")
        try:
            return open(p).read()
        except EnvironmentError:
            return None


    def _pid_of_mcs_router(self):
        pids = []
        for pid in os.listdir("/proc"):
            cmdline = self.get_cmdline(pid)
            if cmdline is None:
                continue
            if "mcsrouter.mcs_router" in cmdline :
                pids.append(int(pid))
        return pids

    def kill_mcsrouter(self):
        """
        Kill any running mcsrouter
        :return:
        """
        pids_of_mcs_router = self._pid_of_mcs_router()
        if pids_of_mcs_router:
            for pid in pids_of_mcs_router:
                os.kill(pid, 9)
            return True
        return False

    def check_mcs_router_process_running(self, require_running=True):
        if require_running in ['True', 'False']:
            require_running = require_running.lower() == "true"
        assert isinstance(require_running, bool)
        pids = self._pid_of_mcs_router()
        pid = None
        if pids:
            if len(pids) != 1:
                raise AssertionError("Only one instance of mcsrouter should be running at any given time. Found {}".format(pids))
            pid = pids[0]

        if pid and not require_running:
            raise AssertionError("MCSrouter running with pid: {}".format(pid))
        if pid and require_running:
            return pid
        if not pid and not require_running:
            return None
        if not pid and require_running:
            raise AssertionError("No MCSrouter running")

    def ensure_fake_cloud_certs_generated(self):
        proc = subprocess.Popen(["make", "all"], cwd=self.cloud_server_path, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
        out, err = proc.communicate()
        rc = proc.returncode
        assert rc == 0, "failed to run make all in cloud automation folder"


    def create_back_dated_alc_status_cache_file(self, output_file, status):
        """
        create_back_dated_alc_status_cache_file: Creates a json file where the ALC status is just over 7 days old.
        :param output_file: location to write the created status cache json file.
        """
        now = int(time.time())
        older_than_7_days = now - (60 * 60 * 24 * 7 + 1 )
        hash_object = hashlib.md5(status.encode())
        alc_status_hash =  hash_object.hexdigest()

        status_file_contents = r'{"MCS": {"timestamp": 1581068893.815016, "status_hash": "dacbef969e71827dc783759d200a00f4"},' + \
                               r' "ALC": {"timestamp": ' + str(older_than_7_days) + r', "status_hash": "' + alc_status_hash + r'"},' + \
                               r' "APPSPROXY": {"timestamp": 1581068893.8161232, "status_hash": "7b52e4dd17ccef61ddc23fa24c7ded69"},' + \
                               r' "AGENT": {"timestamp": 1581068893.9086106, "status_hash": "073e11d9fdbc4902b784295b43ed4f89"}}'

        with open(output_file, 'w') as status_file:
            status_file.write(status_file_contents)


def main(argv):
    r = MCSRouter()
    r.kill_mcsrouter()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
