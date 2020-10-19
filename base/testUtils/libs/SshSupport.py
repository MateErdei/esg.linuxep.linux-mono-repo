#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import shutil
import paramiko
from robot.api import logger
import subprocess
from subprocess import Popen, PIPE, STDOUT

InvalidKey = b"""-----BEGIN RSA PRIVATE KEY-----
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
-----END RSA PRIVATE KEY-----
"""

# password linux
private_key_with_password=b"""-----BEGIN RSA PRIVATE KEY-----
ProcessUtils-Type: 4,ENCRYPTED
DEK-Info: AES-128-CBC,ED92BFAD86E6100E3708A967DF755FEB

qJ6OVb+qda0szxXPkUxi0THdbHSVdSPVaad/xayX9OLiuTAjqA4raadnO80S37VW
k8b8Dfn70GhuFAwZFvw/TTd8FOtvWSUv3cftBfCoPSkHTbyfu2bnfI2peiZHWtSF
F6uzlysUFzPiVC4LnuEU5r3zak99iOf6UOWJY5FaJSQRUGodxRfNUwIuK7eaq4Pv
6lULIDiCEbhAClR5tETJxXpEwHqQD4fFr1LdMIbhHjaxKlhwFieXJ4yESDqm68n5
JbNShXL0WJMXnm8ZKAFS/tUWfcX8mevSm9Xfjd+HQF8IVUenogeezHLhPiPR2UfW
Ee8iiT5eDCAYy70Cvplh3AfYrXkd/+mkL22FDdEDBa8GGTfh5s2JFDAzexqqQLdt
+/K1ndXas5gCCehulbTmS7t9Fl6dSYNfgkPH0pxL9ohOz19w37LWpdLXWcaB9RJO
2q2SgoWRsC2TXY8yhn1E5pHcg4L5DcbRryBEau+38jV3Y/PRwu4+0s75qAoNTdwz
Kt3KnGJ0iRIDvvl0+PNhPGXxI0/yrtLE7wWn2F+7bGZm3s8zUJUByRIyyozZzw6X
SKw3p2QfPtL2B+OQD6txDho4MMkHqdmA7pvEwWGcdp6V+3Mr/zsge1YiX7JBc1qK
zwjeVZ1GKZcNDXgX90HaUj+/Y0HzdltokN0gwAFMlP6SLwYASdo3vFQTe5gHPAvb
klIl0xtJ1wqO15u8WbUQXHvOe1Tl/qxyI+zznxqLPYa8TtGgwrDMmePa9FhYAL5g
+nvtQvFdiDMXEFG7lFr3+N+cSZ5QlXHmlBVVzoK9n1sZD04XMUDk4GUCypL2LfYq
pfOKeiRJuYBeVzueow7CdY8mvxhSlwgSyq8Y008ir6gxEdAAAl2+Ir7TZEKUya9U
g+S2cPA/MMkXrEvyIrWSppT2RcVag3fwpJjU0g2E0bV02Hn0oihkZ6wVNYYdL5Vd
unp+DcBXTQ9iv9KUaMLlYp5LQWYyDqDdGGOV5aLRaW3IGUifJaiVknDjzb7Bx0mL
B2QHXDjOTDXHHiKojuoB7EvnaqGyul0+ol/k6pyndlHCyQF7Bd4jVvNVQjAs35bf
4NfhnRl1QwtC/UQhvJrBFduad8T/PmenRpNcwLtjljagqjO/og4EaMciNwyiuPzG
x29HvpHTdxBGkR/PnOLizHiq05Pd/T132E1Agvrz0lhqSmeHCfHKVcUEt8h2qUPb
3ufrBEDyb1jZQWs8+ZlaXf8bhxIdrs/7+qa9q8TJ7JK9YVA35ohsZ5spVcUYPW2H
wtlSyRaWT6st93Cv0oCkLgJDDV/bAaJGmajtugL7FmHuQl0ZK22Wnl2ZQjoQxdMy
ThNu7Dyp0gOzcE0Vjer0m8Cbwez1N358cnM7e8X7u5269WXAqtpSJbNytIWtpMvD
Cz4+SCVbCBnXVIz76rukXSqah0ui8/GGkiyD0zFJSBwb7PNeNKUViyWp3Y4Lq2v0
9UFiJEZQOyd995wiDjftHg3xEMracmwUWLT2K6C8V3bGFWi4jE3WkxAYM8tf0Ynm
wp0f5S1HQzZLvOLtZ0F8YlfwdITHjDTiy3nwcpsMCy0S18jmS5334pLG+zPnAEdW
-----END RSA PRIVATE KEY-----
"""
public_key_with_password=b"""ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQCsrAo/xqUCmxTot46zPjaU22mz1eB6Pl51+vQ8xUyY9QxkHqWQ1URbGpV/8gc8CJwDJNVvtgKLgUDy3RLiHIxbBO6R+6ZX8c2oZb0K5XjUdfIS9X1hXWImiiGhVnH420kRPlO3BWmoVmB1A7oXeu7Fu++LaCPJOQCLRtZq0mFoT9GOj7xaAmnglXb1smx6jEmnOGBJXd3l+Ma9CtXju55+Emu0t83RUpMSqgRERsJiC2BS+Inx/eUbKjYcYga+l16yVO1C/gH9Mhhdlv2aeJ777ORp6bu6LNF96oalOoN3rOKrCAwaA4KKoE7FtoUAT9bHS2In0sCRfEDC7BAYOGQ7 pair@harvey
"""

private_key_no_password=b"""-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAv1d3jSJPX+nSM1S2iEaPpRIkoeKIuiO429CzWK1aQiXWTOOz
DNXoAqPIDKEbKsjJnvAcIWYrQXYIIbkWoC3i1vFLxOMJmOxq0ytFeLcLnmw7M83V
Th28EEo9uJOyxF5izOrLPFQjyGc6a2xU5hM3HG1IOagjIxV1cfMeppdh3WuehCPA
BruLXCRLc5FHNxoimlJxfdOT2RshiiR09i0GAr0H62cKKtmBzW3KeSC7d0DUSnto
PcxLoukWBmqoxu5dkQRjH+40iqmdbW9yuCy1Kj7YmeEl9rglmPo4mxec50Wx2Txz
F0WsmcrTm8jg0+z7pCII0eGoEH2ARKckAycvuQIDAQABAoIBADOWHFhDqMy+GHrY
WwHwrCef9ZpjneM5YEx7Pu8ohY4R7235cbuDLDMR/wxQnydwzNL5/0uN5RmX/edB
pHT5ChBTK89gOqUtEDvwjaFXEF3c5jU4SErwP1TQ/0T1JRxGKjL/Fl4kfSU5l4mA
wn7pLVGPsQpCZ368+VYIavoVFECCnHmlMCyJM1uGmdKIUsBEj4TCY25+S3vxS9Fu
4g47CFcNB19Goxn+VYPkRwgd0djwQ7Yh4Z6S0/SfOLF0U5VdGhLfjze5fsPB1Diw
YAGoW3mEMrs7YJzlSB9xgErKdJfuOGdL+wN60sQbwmN6m0Je7dLH8t7UCGhqwMzu
9nJYOgECgYEA8o++u/oUJRiIDep47x5Q7vFN7fMB14+IsSytsLUtyoUosYMaKCOL
meRrheVSLQLiZRTFy/ngiNT0qLi68BjN0IitUMXfMS//UYoud+gAeEaX300+YBG1
jGEkE/X3GhD+wzNStJB/PqbjofKr65sVB6A2zhD5897WwWlBhtRMS/kCgYEAyfFF
GZVkcu5fyeZQHw+YesQRvTzf0Zx6sgvLtN2EQkjV3RM1VVlcmsE9ki/3u9Yms56V
jPVt7w+SQwQKUYLPhiacu1fjtsNz+pHkMuX1Xg49N7Ic3wOReIkjRSOLTSal+RKL
HUaMnb919T1NmaAHbf/dhz4LhDN1yQSik6UhccECgYEAn4ThTwZcETlc4Kn+9mLd
nwaa1Y4m+/itULetUrUUdEe2R2eM4DDgMkkCXYA+1tM6yHcszcR7YgGLFRrH9faa
BDaepKw86EBEjP9vJ/QvuunSH0zRZKTA3J1V+Lqd00qyAPXTwmP1CuL+eRb68WuA
HQ/Eeyi2+Sbn3TK3oVAlDSECgYAx1neFPtdRff7p5wsy/zhUY/s0xsc+Be5J7ptR
gbHYYf1V2B27eJhgIPy/DVOIaeuXPLYP2apN63vfSin5v9zTcMgRfDlYq5f96O92
mEYb9kupaS2y5ECMjNvFfmYsnjMr6yWmDfk6HTxRT9XM6i+rOBBGBkv1TnXtLFWr
Nn+wgQKBgQDnlUmoxLnQnc4KtGHBeuWzOlOnWNKqdosB0rn6fFsWbE2epcTrtUZN
PZvWz22V4BKlSlv4hqLfoZO2t/e9XjhoXVY2HYy3OkahwbMXYbAWf5ykvnQnfxwU
h8zQW6DyyovMhOuEOtsx0H8h7GW/pgKR/c4csBjbhpmFbeo4/LFU+w==
-----END RSA PRIVATE KEY-----
"""
public_key_no_password=b"""ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQC/V3eNIk9f6dIzVLaIRo+lEiSh4oi6I7jb0LNYrVpCJdZM47MM1egCo8gMoRsqyMme8BwhZitBdgghuRagLeLW8UvE4wmY7GrTK0V4twuebDszzdVOHbwQSj24k7LEXmLM6ss8VCPIZzprbFTmEzccbUg5qCMjFXVx8x6ml2Hda56EI8AGu4tcJEtzkUc3GiKaUnF905PZGyGKJHT2LQYCvQfrZwoq2YHNbcp5ILt3QNRKe2g9zEui6RYGaqjG7l2RBGMf7jSKqZ1tb3K4LLUqPtiZ4SX2uCWY+jibF5znRbHZPHMXRayZytObyODT7PukIgjR4agQfYBEpyQDJy+5 pair@harvey
"""


def get_home(user):
    try_paths = ['/', '/home']
    for dir_path in try_paths:
        fullpath = os.path.join(dir_path, user)
        if os.path.isdir(fullpath):
            return fullpath
    raise KeyError("User path not found")


def get_ssh_entry_path(filename, username):
    filepath = os.path.join(get_home(username), '.ssh', filename)
    return filepath


def install_private_key( keypath, content ):
    if os.path.exists(keypath):
        logger.info("path exists: {}".format(keypath))
        return

    folderpath = os.path.dirname(keypath)

    if not os.path.exists(folderpath):
        logger.info("ssh folder does not exist: {}".format(folderpath))
        logger.info("creating ssh folder ")
        os.makedirs(folderpath)

    try:
        logger.info("Setting up the ssh key {}".format(keypath))
        with open(keypath, 'w') as f:
            f.write(content)

        os.chmod(keypath, 0o600)
        logger.info("File created and ssh key setup {}".format(keypath))
    except Exception as ex:
        logger.warn(ex.message)
        raise


def remove_key(keypath):
    if os.path.exists(keypath):
        os.remove(keypath)


def set_public_key_allowed(username, publickeycontent):
    authorized_keys = get_ssh_entry_path('authorized_keys', username)
    authorized_keys_bck = get_ssh_entry_path('authorized_keys.bkp', username)
    if os.path.exists(authorized_keys):
        shutil.move(authorized_keys, authorized_keys_bck)
    dir_path = os.path.dirname(authorized_keys)
    if not os.path.exists(dir_path):
        os.mkdir(dir_path)
    with open(authorized_keys, 'w') as filehandle:
        filehandle.write(publickeycontent)
    print('Set public key allowed at: {}'.format(authorized_keys))


def restore_authorized_keys(username):
    try:
        authorized_keys = get_ssh_entry_path('authorized_keys', username)
        authorized_keys_bck = get_ssh_entry_path('authorized_keys.bkp', username)
    except KeyError :
        # user does not exist
        return
    if os.path.exists(authorized_keys_bck):
        shutil.move(authorized_keys_bck, authorized_keys)


def authorize_key_with_password(username):
    restore_authorized_keys(username)
    set_public_key_allowed(username, public_key_with_password)


def authorize_key_without_password(username):
    restore_authorized_keys(username)
    set_public_key_allowed(username, public_key_no_password)


def create_private_key_with_password():
    key_path = '/tmp/key_pass_rsa'
    install_private_key(key_path, private_key_with_password)
    os.chmod(key_path, 0o600)
    return key_path


def create_private_key_without_password():
    key_path = '/tmp/key_nopass_rsa'
    install_private_key(key_path, private_key_no_password)
    os.chmod(key_path, 0o600)
    return key_path


def create_invalid_private_key():
    key_path = '/tmp/invalid_key_rsa'
    install_private_key(key_path, InvalidKey)
    os.chmod(key_path, 0o600)
    return key_path


def clear_private_keys():
    for key_path in ['/tmp/key_pass_rsa',  '/tmp/key_nopass_rsa', '/tmp/invalid_key_rsa']:
        remove_key(key_path)


def run_remote_ssh_command(user, key_file_path):
    """
    Run a ssh -C 'ls /tmp' using the user and key_file_path given.
    :param user: User to try the ssh connection
    :param key_file_path: private rsa file path
    :return: output of the result if considered not successful.
    """
    mark_file ="markfilefortest.txt"
    mark_filepath = '/tmp/{}'.format(mark_file)
    with open(mark_filepath, 'w') as file_handler:
        file_handler.write(b"anything.")

    try:
        # the option StrictHostKeyChecking is set to know to avoid requiring to edit the .ssh/known_hosts for the test
        p = Popen(['ssh', '-i', key_file_path, '{}@localhost'.format(user), '-oBatchMode=yes', '-oPasswordAuthentication=no', "-oStrictHostKeyChecking=no" , '-C', 'ls /tmp'], stdout=PIPE, stderr=STDOUT)
        output = p.communicate()[0]
        if p.returncode == 0 and mark_file in output:
            return ""
        else:
            return output
    finally:
        os.remove(mark_filepath)


def run_remote_ssh_command_using_password(user, password):
    sshpass_path_list = [candidate for candidate in ['/usr/bin/sshpass', '/bin/sshpass'] if os.path.exists(candidate)]
    if len(sshpass_path_list) == 0:
        raise AssertionError("Sshpass not available in this machine. ")

    sshpass_path = sshpass_path_list[0]

    mark_file ="markfilefortest.txt"
    mark_filepath = '/tmp/{}'.format(mark_file)
    with open(mark_filepath, 'w') as file_handler:
        file_handler.write(b"anything.")

    try:
        # the option StrictHostKeyChecking is set to know to avoid requiring to edit the .ssh/known_hosts for the test
        p = Popen([sshpass_path, '-p', password, 'ssh', '{}@localhost'.format(user), "-oStrictHostKeyChecking=no", '-C', 'ls /tmp'], stdout=PIPE, stderr=STDOUT)
        output = p.communicate()[0]
        if p.returncode == 0 and mark_file in output:
            return ""
        else:
            return output
    finally:
        os.remove(mark_filepath)


def check_ssh_connection(expectFailure, user='root', password='', key_file_path=''):
    """
    Check that an ssh connection can be established.

    :param expectFailure: true or false. If expectFailure is true, it will raise AssertionError when it does not fail.
    :param user:  name of the user.
    :param password:  the password to try or empty to not use password
    :param key_file_path: path of the private key or empty to not use key on the connection
    :return: None on success. Raise AssertionError on failure
    """
    if password == "" and not os.path.exists(key_file_path):
        raise AssertionError("Invalid option. Either password or key_file_path is to be given")

    tailpath = [candidate for candidate in ['/usr/bin/tail', '/bin/tail'] if os.path.exists(candidate)][0]
    handle = subprocess.Popen([tailpath,'-F','/var/log/audit/audit.log'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)


    client = paramiko.client.SSHClient()
    client.load_system_host_keys()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    failed = False
    try:
        if os.path.exists(key_file_path):
            if password != "":
                client.connect(hostname="localhost", username=user, port=22, password=password, key_filename=key_file_path)
                print("Successful connection")
                client.close()
            else:
                client.connect(hostname="localhost", username=user, port=22, key_filename=key_file_path)
                print("Successful connection")
                client.close()
        else:
            client.connect(hostname="localhost", username=user, port=22, password=password)
            print("Successful connection")
            client.close()
    except paramiko.ssh_exception.AuthenticationException as ex:
        logger.info(str(ex))
        print(str(ex))
        failed = True
    except paramiko.ssh_exception.SSHException as ex:
        failed = True
        message = str(ex)
        logger.info(message)
        if "Error reading SSH protocol banner" in message:
            raise AssertionError("Unexpected Ssh exception: {}".format(message))
    finally:
        handle.kill()
        output = handle.communicate()[0]
        if failed and not expectFailure:
            logger.info(output)

    if expectFailure and not failed:
        raise AssertionError("Ssh success. But expected it to fail.")
    if expectFailure and failed:
        return
    if not expectFailure and not failed:
        return
    if not expectFailure and failed:
        raise AssertionError("Ssh connection failed but it was expected to not fail")


def simulate_failed_ssh_using_password(user='root', password='norealpassword'):
    check_ssh_connection(True, user, password=password)


def simulate_successful_ssh_using_password(user, password):
    check_ssh_connection(False, user, password)


def simulate_failed_ssh_using_key_and_password(user, password, key_file_path):
    check_ssh_connection(True, user, password, key_file_path)


def simulate_successful_ssh_using_key_and_password(user, password, key_file_path):
    check_ssh_connection(False, user, password, key_file_path)


def simulate_successful_remote_command(user, key_file_path):
    output = run_remote_ssh_command(user, key_file_path)
    if output == "":
        return
    else:
        raise AssertionError("Remote Command failed.\n" + output)


def simulate_failed_remote_command(user, key_file_path):
    output = run_remote_ssh_command(user, key_file_path)
    if output == "":
        raise AssertionError("Expected remote command to fail. But it succeeded.")


def simulate_successful_remote_command_for_password(user, password):
    output = run_remote_ssh_command_using_password(user, password)
    if output == "":
        return
    else:
        raise AssertionError("Remote Command failed.\n" + output)


def simulate_failed_remote_command_for_password(user, password):
    output = run_remote_ssh_command_using_password(user, password)
    if output == "":
        raise AssertionError("Expected remote command to fail. But it succeeded.")


def simulate_failed_ssh_using_public_key():
    invalid_key = create_invalid_private_key()
    print('KeyPath: {}'.format(invalid_key))
    authorize_key_without_password('root')
    try:
        check_ssh_connection(True, 'root', key_file_path=invalid_key)
    finally:
        restore_authorized_keys('root')
        clear_private_keys()
