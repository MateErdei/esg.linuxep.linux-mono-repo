#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import subprocess
import pwd
import time
from robot.api import logger


def _get_first_full_path_executable( list_of_execs):
    for executable in list_of_execs:
        try:
            output = subprocess.check_output(["which", executable])
            if len(output):
                # remove end line
                return output[:-1]
            continue
        except Exception as ex:
            logger.info(str(ex))
    return None


def add_user(user, set_home=False):
    add_user_commands = ['useradd', 'adduser']
    add_user_command = _get_first_full_path_executable(add_user_commands)

    if add_user_command is not None:
        try:
            if set_home:
                subprocess.check_output(["sudo", add_user_command, '-m',  user])
            else:
                subprocess.check_output(["sudo", add_user_command,  user])
        except Exception as ex:
            logger.debug(str(ex))
            raise AssertionError("Command to add user failed")
    else:
        raise AssertionError("No add user command available on machine")
    if set_home:
        sshpath = os.path.join('/home', user, '.ssh')
        os.mkdir(sshpath)
        uid = pwd.getpwnam(user).pw_uid
        os.chown(sshpath, uid, 0)
        os.chmod(sshpath, 0o700)


def delete_user(user, allow_failure):
    try:
        pwd.getpwnam(user)
    except KeyError:
        # user does not exist
        return

    del_user_commands = ['userdel', 'deluser']
    del_user_command = _get_first_full_path_executable(del_user_commands)

    if del_user_command is not None:
        try:
            subprocess.check_output(["sudo", del_user_command, '-r', '-f', user])
        except Exception as ex:
            logger.debug(str(ex))
            if not allow_failure:
                raise AssertionError("Command to delete user failed")
    else:
        raise AssertionError("No delete user command available on machine")


def set_user_password(user, password):
    passwd_command = _get_first_full_path_executable(['passwd'])
    if passwd_command is None:
        raise AssertionError("No passwd command found to change user password")
    p = subprocess.Popen([passwd_command, user], stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.STDOUT)

    output = p.communicate("{}\n{}".format(password, password))
    if p.returncode != 0:
        raise AssertionError("Failed to set passowrd for user {}.\n{}".format(user, output))


def add_user_for_test( name='userfortest', passwd='linuxpassword'):
    delete_user(name, True)
    add_user(name, set_home=True)
    set_user_password(name, passwd)


def delete_user_for_test(name='userfortest'):
    attempts = 0
    while True:
        try:
            attempts += 1
            delete_user(name, False)
            return
        except Exception as ex:
            if attempts > 10:
                raise
            time.sleep(1)
            logger.info( "Attempt to delete user failed with: {}".format(str(ex)))
