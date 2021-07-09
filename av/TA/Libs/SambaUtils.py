#!/bin/env python3

import os
import subprocess

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

def Allow_Samba_To_Access_Share(directory):
    """
    Python replica for:

    [Arguments]  ${source}
    ${result} =   Run Process  semanage  fcontext  -a  -t  samba_share_t  ${source}(/.*)?
    Log  "semanage:${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"
    ${result} =   Run Process  restorecon  -Rv  ${source}
    Log  "restorecon:${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"
    ${result} =   Run Process  sestatus
    Log  "sestatus:${SPACE}stdout: \n${result.stdout} \n${SPACE}stderr: \n${result.stderr}"
    :param directory:
    :return:
    """
    if not os.path.isdir(directory):
        raise AssertionError("directory %s doesn't exist - can't apply context" % directory)

    # We have to --add if there isn't an entry, or --modify if there is!!

    for action in ("--modify", "--add"):
        command = ["semanage", "fcontext", action, "--type", "samba_share_t", directory+"(/.*)?"]
        try:
            p = subprocess.run(command,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT)
        except FileNotFoundError as ex:
            logger.debug("semanage not found: Probably not SELinux on system: %s" % str(ex))
            return
        except Exception as ex:
            logger.error("Failed to run semanage: %s" % str(ex))
            return
        if p.returncode == 0:
            break

        logger.info("semanage returned non-zero %d: %s" % (p.returncode, p.stdout))
        logger.info("Command: "+" ".join(command))
        # try add

    if p.returncode != 0:
        logger.error("Failed to add or modify semanage fcontext!")
        return

    subprocess.check_call(['restorecon', '-Rv', directory])

    p = subprocess.run(['sestatus'],
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    logger.info("sestatus: %s" % p.stdout)
