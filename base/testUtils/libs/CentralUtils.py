#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import os
import pwd
import grp
import shutil
import time
import stat
import xml.dom.minidom
import socket
import sys
import subprocess
import json

from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError

## Before importing cloudClient!
os.environ.setdefault("TMPROOT", "/tmp")

import PathManager
PathManager.addPathToSysPath(PathManager.SUPPORTFILEPATH)

import CloudAutomation.cloudClient
import CloudAutomation.SendToFakeCloud


def get_install():
    try:
        return format(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"))
    except RobotNotRunningError:
        try:
            return os.environ.get("SOPHOS_INSTALL", "/opt/sophos-spl")
        except KeyError:
            return "/opt/sophos-spl"


class Options(object):
    def copy(self):
        o = Options()
        o.__dict__ = self.__dict__.copy()
        return o

    def __repr__(self):
        d = self.__dict__.copy()
        d.pop('password', None)
        return d.__repr__()


def getOptions():
    options = Options()
    options.wait_for_host = False
    options.ignore_errors = False
    options.hostname = None
    options.wait = 30

    CENTRAL_CONFIG = os.environ.get("MCS_CONFIG_SET", "sspl-nova")

    if CENTRAL_CONFIG.startswith("sspl-"):
        options.email = "dev2@ssplinux.test.com"
        options.password = "Ch1pm0nk"
        options.user = "sspl"

    if CENTRAL_CONFIG.startswith("ucmr-"):
        options.email = "trunk@savlinux.xmas.testqa.com"
        options.password = "Ch1pm0nk"
        options.user = "sspl"

    if CENTRAL_CONFIG.startswith("sspl-ostia-"):
        options.email = "manualdev@ssplinux.test.com"
        options.password = "Ch1pm0nk"
        options.user = "sspl"

    if CENTRAL_CONFIG.startswith("sav-"):
        options.email = "dev@savlinux.test.com"
        options.password = "Ch1pm0nk"
        options.user = "sav"

    if CENTRAL_CONFIG.endswith("-nova"):
        options.region = "https://fe.sandbox.sophos"
        options.proxy = None
        options.proxy_username = None
        options.proxy_password = None
        options.cloud_host = "sandbox.sophos"
        options.cloud_ip = os.environ.get("OVERRIDE_CLOUD_IP", "10.55.36.78")  # Allow override for AWS Nova instance
        options.CLOUD_SERVER = "nova"
        options.login = "pair@sandbox.sophos"

        if os.path.isfile("/root/OVERRIDE_CLOUD_IP"):  # File is only created on AWS instance
            options.cloud_ip = open("/root/OVERRIDE_CLOUD_IP").read().strip()
            options.login = "ubuntu@sandbox.sophos"

    elif CENTRAL_CONFIG.endswith("-dev"):
        options.region = 'https://p0.d.hmr.sophos.com'
        options.proxy = None
        options.proxy_username = None
        options.proxy_password = None
        options.cloud_host = None
        options.cloud_ip = None
        options.CLOUD_SERVER = "dev"

    return options


def reload_cloud_options():
    global OPTIONS
    OPTIONS = getOptions()


reload_cloud_options()


def cloud_command(*args, **kwargs):
    global OPTIONS
    options = OPTIONS.copy()
    options.__dict__.update(kwargs)

    return CloudAutomation.cloudClient.processArguments(options, args)


def cloud_command_expecting_zero(*args, **kwargs):
    ret = cloud_command(*args, **kwargs)
    if ret != 0:
        raise AssertionError("Cloud command failed: {} {} with {}".format(args, kwargs, ret))

def cloud_command_wait_for_host(*args):
    return cloud_command(*args,wait_for_host=True)


def getClient(**kwargs):
    global OPTIONS
    options = OPTIONS.copy()
    options.__dict__.update(kwargs)
    return CloudAutomation.cloudClient.CloudClient(options)


def setup_host_file():
    global OPTIONS
    cloud_host = OPTIONS.cloud_host
    if cloud_host != "sandbox.sophos":
        return False

    ip = OPTIONS.cloud_ip

    lines = open("/etc/hosts").readlines()
    out = []
    for line in lines:
        if cloud_host in line or "billing.sandbox.salesforce" in line:
            continue
        out.append(line)

    if not os.path.isfile("/etc/hosts.bak"):
        open("/etc/hosts.bak", "w").writelines(out)

    for prefix in ("mcs", "api", "mob", "billing", "fe", "legacy", "core", "utm", "wifi"):
        out.append("{} {}.{}\n".format(ip, prefix, cloud_host))
    out.append("{} {}\n".format(ip, cloud_host))
    out.append("{} billing.sandbox.salesforce\n".format(ip))

    open("/etc/hosts", "w").writelines(out)

    return True


def revert_hostfile():
    if os.path.isfile("/etc/hosts.bak"):
        os.rename("/etc/hosts", "/etc/hosts.tmp")
        os.rename("/etc/hosts.bak", "/etc/hosts")
        os.remove("/etc/hosts.tmp")


def append_host_file(ip_address, hostname):
    """
    Appends the hostfile. It is important the file is reverted at the end of a test where this function is used.
    :param ip_address: string of the ip address
    :param hostname: string of the hostname
    """
    lines = open("/etc/hosts").readlines()
    if not os.path.isfile("/etc/hosts.bak"):
        open("/etc/hosts.bak", "w").writelines(lines)
    lines.append("{} {}\n".format(ip_address, hostname))
    open("/etc/hosts", "w").writelines(lines)


def block_traffic_to_message_relay(hostname):
    ip_address = socket.gethostbyname(hostname)
    command = ["iptables", "-A", "OUTPUT", "-d", ip_address, "-j", "DROP"]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout = process.communicate()[0]
    if process.wait() != 0:
        raise AssertionError("error with setting iptables rule: {}".format(stdout.decode('utf-8')))


def unblock_traffic_to_message_relay(hostname):
    ip_address = socket.gethostbyname(hostname)

    command = ["iptables", "-L", "OUTPUT"]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    result, stderr = process.communicate()
    if process.wait() != 0:
        raise AssertionError("error with listing iptables rules: {}".format(stderr.decode('utf-8')))

    split_result = result.decode("utf-8").splitlines()

    element_contents = None
    for i in split_result:
        if ip_address in i:
            element_contents = i
    if element_contents is None:
        # no iptables rule to block traffic to Message Relay so nothing to do
        return
    rule_number = split_result.index(element_contents) - 1

    command = ["iptables", "-D", "OUTPUT", str(rule_number)]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout = process.communicate()[0]
    if process.wait() != 0:
        raise AssertionError("error with deleting iptables rule: {}".format(stdout.decode('utf-8')))


def get_registration_command():
    client = getClient()
    return client.getRegistrationCommand()


def get_sspl_thinstaller_url():
    client = getClient()
    return client.getSSPLThinInstallerURL()


def get_sspl_registration():
    bashfile = "./SupportFiles/CloudAutomation/BashScripts/getSSPLInstallerandCommand.sh"
    command = ["bash", bashfile, get_sspl_thinstaller_url()]
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout = process.communicate()[0]
    if process.wait() != 0:
        raise AssertionError("error with get registration command: {}".format(stdout.decode('utf-8')))
    with open('/tmp/registerCommand', 'r') as file:
        registerCommand = file.read()
    return registerCommand


def get_registration_token_and_url():
    command = get_registration_command()
    args = command.split()
    return (args[1],args[2])


def delete_updating_policy_for_hostname(hostname=None):
    client = getClient()
    hostname = CloudAutomation.cloudClient.host(hostname)
    output = client.deleteUpdatingPolicyForHostname(hostname)
    print(output,file=sys.stderr)
    return 0


def Delete_Server_From_Cloud(hostname=None):
    client = getClient()
    hostname = CloudAutomation.cloudClient.host(hostname)
    output = client.deleteServerByName(hostname)
    print(output,file=sys.stderr)
    return 0


def Check_Server_In_Cloud(hostname=None):
    client = getClient()
    hostname = CloudAutomation.cloudClient.host(hostname)

    if client.getServerByName(hostname) is None:
        return 1
    return 0


def Wait_For_Server_In_Cloud(hostname=None, wait=60, delay=1):
    global OPTIONS
    options = OPTIONS
    client = getClient()
    hostname = CloudAutomation.cloudClient.host(hostname)

    start = time.time()
    while time.time() < start + wait:
        if client.getServerByName(hostname) is not None:
            return True
        time.sleep(delay)
        delay += 1

    if client.getServerByName(hostname) is not None:
        return True

    raise AssertionError("{} not found in Central after {}".format(hostname, delay))


def Send_Command_From_Fake_Cloud(cmd):
    CloudAutomation.SendToFakeCloud.sendCmdToFakeCloud(cmd)


def Send_Query_From_Fake_Cloud(name, query, command_id="correlation-id"):
    query_dict = {"type": "sophos.mgt.action.RunLiveQuery",
                  "name": name,
                  "query": query}
    CloudAutomation.SendToFakeCloud.sendLiveQueryToFakeCloud(json.dumps(query_dict), command_id=command_id)

def Set_Local_CA_Environment_Variable():
    scriptdir = os.path.abspath(os.path.dirname(__file__))
    rootdir = os.path.dirname(scriptdir)
    supportFiles = os.path.join(rootdir, "SupportFiles")
    MCS_CA = os.path.join(supportFiles, "CloudAutomation", "root-ca.crt.pem")
    Set_MCS_CA_Environment_Variable(MCS_CA)

def getSophosGID():
    try:
        return grp.getgrnam("sophos-spl-group").gr_gid
    except KeyError:
        return 0


def copyCAToTempFile(srcfile):
    cert = copy_CA_to_file(srcfile, "/tmp")
    return cert


def copy_CA_to_file(srcfile,destination):
    MCS_CA = srcfile

    mcs_cert=os.path.join(destination, os.path.basename(srcfile))

    shutil.copy(MCS_CA, mcs_cert)
    assert os.path.isfile(mcs_cert)

    uid = pwd.getpwnam("root").pw_uid
    gid = getSophosGID()
    os.chown(mcs_cert, uid, gid)
    perms = stat.S_IREAD | stat.S_IWRITE | stat.S_IRGRP
    if gid == 0:
        # Make it readable for all users if we don't have sophos-spl-group
        perms |= stat.S_IROTH

    os.chmod(mcs_cert, perms)

    return mcs_cert

def Setup_MCS_CA_with_incorrect_permissions(cert_source):
    tmp_path = os.path.join("/tmp", "tempcertdir")
    os.makedirs(tmp_path)
    os.chmod(tmp_path, 0o700)

    shutil.copy(cert_source, tmp_path)
    cert_path = os.path.join(tmp_path, os.path.basename(cert_source))

    Set_MCS_CA_Environment_Variable(cert_path)

def Set_System_test_MCS_CA_Environment_Variable(ca_base_name):
    import SystemProductTestOutputInstall
    SystemProductTestOutputInstall.getSystemProductTestOutput()
    system_product_test_path = SystemProductTestOutputInstall.DESTINATION

    MCS_CA = os.path.join(system_product_test_path, ca_base_name)
    tempMCSCert = copyCAToTempFile(MCS_CA)
    Set_MCS_CA_Environment_Variable(tempMCSCert)


def Set_Nova_MCS_CA_Environment_Variable():
    Set_System_test_MCS_CA_Environment_Variable("nova_mcs_rootca.crt")


def get_nova_mcs_ca_path():
    import SystemProductTestOutputInstall
    SystemProductTestOutputInstall.getSystemProductTestOutput()
    system_product_test_path = SystemProductTestOutputInstall.DESTINATION
    MCS_CA = os.path.join(system_product_test_path, "nova_mcs_rootca.crt")
    tempMCSCert = copyCAToTempFile(MCS_CA)
    return tempMCSCert


def Get_Central_Ip():
    global OPTIONS
    options = OPTIONS
    return options.cloud_ip


def set_ca_env_override_flag():
    try:
        flag_path = os.path.join(get_install(), 'base/mcs/certs/ca_env_override_flag')
        open(flag_path, 'a').close()
    except OSError as error:
        logger.error("Could not create mcs_ca override flag with error: {}".format(error))


def Set_MCS_CA_Environment_Variable(ca_file):
    logger.info(os.environ)
    cwd = os.getcwd()
    print(cwd)
    os.environ["MCS_CA"] = ca_file
    if os.path.isdir(get_install()):
        set_ca_env_override_flag()
    logger.info("Setting MCS_CA to %s" % ca_file)


def Unset_CA_Environment_Variable():
    flag_path = os.path.join(get_install(), 'base/mcs/certs/ca_env_override_flag')
    if os.path.isfile(flag_path):
        os.remove(flag_path)
    try:
        del os.environ["MCS_CA"]
    except KeyError:
        pass

def check_server_last_success_update_time(start_time, hostname=None):
    client = getClient()
    client.options.hostname = CloudAutomation.cloudClient.host(hostname)
    client.options.wait = 60
    client.checkServerLastSuccessUpdateTime(start_time)


def get_nova_events(event_type, start_time, end_time, hostname=None):
    client = getClient()
    client.options.hostname = CloudAutomation.cloudClient.host(hostname)
    client.getEvents(event_type, start_time, end_time)


def update_now(hostname=None):
    client = getClient()
    client.updateNow(CloudAutomation.cloudClient.host(hostname))


def set_scheduled_update_time(day,hour,hostname=None):
    client = getClient()
    client.options.hostname = CloudAutomation.cloudClient.host(hostname)
    client.setDelayedUpdatePolicy(client.options.hostname, day, hour)


def getText(node):
    ret = []
    for n in node.childNodes:
        if n.nodeType == xml.dom.Node.TEXT_NODE:
            ret += n.data
    return "".join(ret)


def get_alc_policy_file_info(policy_input_path):
    policy = xml.dom.minidom.parse(policy_input_path)

    output_list = []

    cloud_subscription = policy.getElementsByTagName("cloud_subscription")[0]
    cloud_subscription_rigidname = cloud_subscription.getAttribute("RigidName")
    logger.info("cloud subscription rigidname: {}".format(cloud_subscription_rigidname))

    return [cloud_subscription_rigidname]


def get_sul_config_file_info(config_input_path):
    config = json.load(open(config_input_path, 'r'))
    logger.info("Original value: {}".format(config))
    cloud_subscription_rigidname = config["primarySubscription"]["rigidName"]

    return [cloud_subscription_rigidname]


def get_cloud_time():
    client = getClient()
    return client.getCloudTime()


def safe_makedir(p):
    try:
        os.makedirs(p)
    except EnvironmentError:
        pass


def dump_Appserver_Log(APPSERVER_LINES=2000):
    """
    function __dumpAppserverLog()
{
    [[ -n $APPSERVER_LINES ]] || APPSERVER_LINES=2000
    (( $APPSERVER_LINES == 0 )) && return
    (( $NOT_A_TEST )) && return

    if [[ $APPSERVER_PORT ]]
    then
        ssh-keygen -f "/root/.ssh/known_hosts" \
            -R "[sandbox.sophos]:$APPSERVER_PORT" 2>/dev/null
    else
        ssh-keygen -f "/root/.ssh/known_hosts" \
            -R sandbox.sophos 2>/dev/null
    fi

    local login="core@sandbox.sophos"
    if [[ $CLOUD_SERVER == "nova" ]]
    then
    login="pair@sandbox.sophos"
    elif [[ $CLOUD_SERVER == "aws" ]]
    then
        login="ubuntu@sandbox.sophos"
    fi

    local APPSERVER_KEYFILE=$SUP/CloudAutomation/rsa-key-appserver-log-sandbox.pem
    chmod 0600 $APPSERVER_KEYFILE
    print_header "Sandbox - Appserver Log"
    {
        ssh ${APPSERVER_PORT:+-p $APPSERVER_PORT} \
        -o BatchMode=yes \
        -o StrictHostKeyChecking=no \
        -i $APPSERVER_KEYFILE \
        $login \
        </dev/null 2>&1
    } | tail -n $APPSERVER_LINES

    print_divider

}

    :return:
    """
    if OPTIONS.CLOUD_SERVER != "nova":
        return

    APPSERVER_PORT = os.environ.get("APPSERVER_PORT", None)

    if APPSERVER_PORT:
        SSH_LINE="[sandbox.sophos]:{}".format(APPSERVER_PORT)
    else:
        SSH_LINE="sandbox.sophos"

    if os.path.isfile("/root/.ssh/known_hosts"):
        subprocess.check_call(["ssh-keygen", "-f", "/root/.ssh/known_hosts", "-R", SSH_LINE])

    login = OPTIONS.login

    SUPPORTFILEPATH = os.path.join(os.path.dirname(__file__), "..", "SupportFiles")
    assert(os.path.isdir(SUPPORTFILEPATH))
    APPSERVER_KEYFILE=os.path.join(SUPPORTFILEPATH, "nova", "rsa-key-appserver-log-sandbox.pem")
    assert(os.path.isfile(APPSERVER_KEYFILE))
    os.chmod(APPSERVER_KEYFILE, 0o600)

    command = ["ssh",
               "-o", "BatchMode=yes",
               "-o", "StrictHostKeyChecking=no",
               "-i", APPSERVER_KEYFILE,
               ]
    if APPSERVER_PORT:
        command.append("-p")
        command.append(APPSERVER_PORT)
    command.append(login)

    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    stdout = process.communicate()[0]
    lines = stdout.decode("utf-8").splitlines()
    lines = lines[-APPSERVER_LINES:]
    logger.info("\n".join(lines))
    return lines


def dump_central_version():
    global OPTIONS
    if OPTIONS.CLOUD_SERVER != "nova":
        return

    client = getClient()
    version = client.getCloudVersion()
    logger.info("Central version = {}".format(version))


def set_default_credentials():
    client = getClient()
    client.setUpdateCreds("regruser", "regrABC123pass", "https://localhost:1233")


def set_ostia_credentials():
    client = getClient()
    client.setUpdateCreds("mtr_user_vut", "password", "https://ostia.eng.sophos/latest/sspl-warehouse/master/")


def set_ostia_ga_credentials():
    client = getClient()
    client.setUpdateCreds("ga_mtr_user", "password", "https://ostia.eng.sophos/latest/sspl-warehouse/feature-GA-milestone/")


def check_central_clock():
    global OPTIONS
    if OPTIONS.CLOUD_SERVER != "nova":
        return

    centralTime = get_cloud_time()
    assert centralTime is not None
    localtime = time.time()
    diff = centralTime - localtime

    if abs(diff) > 2:
        logger.error("Central is {} seconds off localtime".format(diff))
    else:
        logger.info("Central time diff = {}".format(diff))


def main(argv):
    print(dump_Appserver_Log())
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
