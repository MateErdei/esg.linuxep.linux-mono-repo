#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import shutil
import subprocess

import time

import tempfile

from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn
import robot.libraries.BuiltIn

def run_proc_with_safe_output(args):
    logger.debug('Run Command: {}'.format(args))
    with tempfile.TemporaryFile(dir=os.path.abspath('.')) as tmpfile:
        p = subprocess.Popen(args, stdout=tmpfile, stderr=tmpfile)
        p.wait()
        tmpfile.seek(0)
        output = tmpfile.read().decode()
        return output, p.returncode

def unmount_all_comms_component_folders(skip_stop_proc=False):
    def _umount_path(fullpath):
        stdout, code = run_proc_with_safe_output(['umount', fullpath])
        if 'not mounted' in stdout:
            return
        if code != 0:
            logger.info(stdout)

    def _stop_commscomponent():
        stdout, code = run_proc_with_safe_output(["/opt/sophos-spl/bin/wdctl", "stop", "commscomponent"])
        if code != 0 and not 'Watchdog is not running' in stdout:
            logger.info(stdout)

    if not os.path.exists('/opt/sophos-spl/bin/wdctl'):
        return
    # stop the comms component as it could be holding the mounted paths and
    # would not allow them to be unmounted.
    counter = 0
    while not skip_stop_proc and counter < 5:
        counter = counter + 1
        stdout, errcode = run_proc_with_safe_output(['pidof', 'CommsComponent'])
        if errcode == 0:
            logger.info("Commscomponent running {}".format(stdout))
            _stop_commscomponent()
            time.sleep(1)
        else:
            logger.info("Skip stop comms componenent")
            break

    dirpath = '/opt/sophos-spl/var/sophos-spl-comms/'

    mounted_entries = ['etc/resolv.conf', 'etc/hosts', 'usr/lib', 'usr/lib64', 'lib',
                       'etc/ssl/certs', 'etc/pki/tls/certs', 'etc/pki/ca-trust/extracted', 'base/mcs/certs','base/remote-diagnose/output']
    for entry in mounted_entries:
        try:
            fullpath = os.path.join(dirpath, entry)
            if not os.path.exists(fullpath):
                continue
            _umount_path(fullpath)
            if os.path.isfile(fullpath):
                os.remove(fullpath)
            else:
                shutil.rmtree(fullpath)
        except Exception as ex:
            logger.error(str(ex))
