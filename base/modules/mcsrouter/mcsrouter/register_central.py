#!/usr/bin/env python
"""
register_central Module
"""

from __future__ import print_function, division, unicode_literals

import glob
import os
import subprocess
import random
import sys
import time
import optparse
import errno
import __builtin__

import logging
import logging.handlers

from .utils import config as utils_config
from .utils.logger_utcformatter import UTCFormatter
from .utils.get_ids import get_gid, get_uid
from .utils.atomic_write import atomic_write
from .utils.timestamp import timestamp

from .mcsclient import mcs_exception
from .mcsclient import mcs_connection
from . import mcs as MCS
from .utils import path_manager

from .utils import sec_obfuscation

LOGGER = logging.getLogger(__name__)

__builtin__.__dict__['REGISTER_MCS'] = True


def safe_mkdir(directory):
    """
    safe_mkdir
    """
    if not os.path.exists(directory):
        # Use a try here to catch race condition where directory is created after checking
        # it doesn't exist
        try:
            old_mask = os.umask(0o002)
            os.makedirs(directory)
            os.umask(old_mask)
        except OSError as exception:
            if exception.errno != errno.EEXIST:
                raise
    return


def setup_logging():
    """
    setup_logging
    """
    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)

    formatter = UTCFormatter(
        "%(process)-7d [%(asctime)s.%(msecs)03d] %(levelname)7s [%(thread)10.10d] %(name)s <> %(message)s", "%Y-%m-%dT%H:%M:%S")

    log_file = path_manager.register_log()

    file_handler = logging.handlers.RotatingFileHandler(
        log_file, maxBytes=1024 * 1024, backupCount=5)
    file_handler.setFormatter(formatter)
    file_handler.setLevel(logging.INFO)
    root_logger.addHandler(file_handler)

    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)
    stream_handler.setLevel(logging.ERROR)
    root_logger.addHandler(stream_handler)


def create_dirs():
    """
    create_dirs
    """
    path_manager_module = path_manager
    paths = [
        path_manager_module.fragmented_policies_dir(),
        path_manager_module.lock_sophos(),
        path_manager_module.sophosspl_log_dir(),
        path_manager_module.sophos_etc_dir(),
        path_manager_module.policy_dir(),
        path_manager_module.event_dir(),
        path_manager_module.action_dir(),
        path_manager_module.status_dir()]
    for path in paths:
        safe_mkdir(path)


def get_target_system():
    """
    get_target_system
    """
    # For some reason Pylint can't find targetsystem even though it is okay
    # pylint: disable=no-name-in-module, no-member
    import mcsrouter.targetsystem
    return mcsrouter.targetsystem.target_system()


def cleanup():
    """
    cleanup
    """
    root_config = path_manager.root_config()
    sophosav_config = path_manager.sophosspl_config()
    for config_file in [root_config, sophosav_config]:
        try:
            os.unlink(config_file)
        except OSError:
            pass


def safe_delete(path):
    """
    safe_delete
    """
    try:
        os.unlink(path)
    except IOError:
        pass
    except OSError:
        pass


def register(config, inst, logger):
    """
    register
    """
    # Do a register operation so that we can be sure that we have connectivity
    print("Registering with Sophos Central")
    mcs = MCS.MCS(inst, config)

    count = 10
    ret = 0
    sleep = 2
    while True:
        try:
            mcs.startup()
            mcs.register()
            break
        except mcs_exception.MCSConnectionFailedException:
            url = config.get("MCSURL")
            print("ERROR: Failed to connect to Sophos Central: Check URL: %s" % url,
                  file=sys.stderr)
            logger.warning(
                "Failed to connect to Sophos Central: Check URL: %s",
                url,
                exc_info=True)
            ret = 4
            break
        except mcs_connection.MCSHttpException as exception:
            if exception.error_code() == 401:
                print("ERROR: Authentication error from Sophos Central: Check Token",
                      file=sys.stderr)
                logger.fatal(
                    "ERROR: Authentication error from Sophos Central: Check Token")
                ret = 6
                break
            else:
                logger.warning(
                    "ERROR: HTTP Error from Sophos Central (%d)" %
                    (exception.error_code()))
                logger.debug(str(exception.headers()))
                logger.debug(exception.body())

                count -= 1
                if count > 0:
                    time.sleep(sleep)
                    sleep += 2
                    logger.info("Retrying registration")
                    continue
                else:
                    print("ERROR: Repeated HTTP Errors from Sophos Central (%d)"
                          % (exception.error_code()), file=sys.stderr)
                    ret = 5
                    break
        except mcs_exception.MCSException as exception:
            logger.fatal("Registration failed with %s (%s)", str(exception), repr(exception))

    return ret


def remove_mcs_policy():
    """
    remove_mcs_policy
    """
    safe_delete(path_manager.mcs_policy_config())
    safe_delete(path_manager.mcs_policy_file())

class RandomGenerator(object):
    """
    RandomGenerator
    """
    # pylint: disable=too-few-public-methods

    def random_bytes(self, size):
        """
        random_bytes
        """
        # pylint: disable=no-self-use
        return bytearray(random.getrandbits(8) for _ in xrange(size))


def add_options_to_policy(relays, proxycredentials):
    """
    add_options_to_policy
    """
    if relays is None and proxycredentials is None:
        return

    policy_config = utils_config.Config(
        path_manager.mcs_policy_config(),
        mode=0o600,
        user_id=get_uid("sophos-spl-user"),
        group_id=get_gid("sophos-spl-group")
    )

    if relays is not None:
        for index, relay in enumerate(relays.split(";"), 1):
            priority_key = "message_relay_priority%d" % index
            port_key = "message_relay_port%d" % index
            address_key = "message_relay_address%d" % index
            id_key = "message_relay_id%d" % index

            address, priority, relay_id = relay.split(",")
            hostname, port = address.split(":")

            policy_config.set(address_key, hostname)
            policy_config.set(port_key, port)
            policy_config.set(priority_key, priority)
            policy_config.set(id_key, relay_id)

    if proxycredentials is not None:
        obfuscated = sec_obfuscation.obfuscate(
            sec_obfuscation.ALGO_AES256,
            proxycredentials,
            RandomGenerator())
        policy_config.set("mcs_policy_proxy_credentials", obfuscated)

    policy_config.save()


def remove_console_configuration():
    """
    remove_console_configuration
    """
    policy_dir = path_manager.policy_dir()
    for policy_file in os.listdir(policy_dir):
        file_path = os.path.join(policy_dir, policy_file)
        if os.path.isfile(file_path):
            safe_delete(file_path)


def remove_all_update_reports_and_config():
    """
    remove_all_update_reports_and_config
    """
    for file_to_remove in glob.glob(
            "{}/update_report*.json".format(path_manager.update_var_path())):
        safe_delete(file_to_remove)
    safe_delete(os.join(path_manager.update_var_path(), "update_config.json"))


def stop_mcs_router():
    """
    stop_mcs_router
    """
    output = subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"])
    if "SubState=dead" not in output:
        subprocess.call([path_manager.wdctl_bin_path(), "stop", "mcsrouter"])


def start_mcs_router():
    """
    start_mcs_router
    """
    output = subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"])
    if "SubState=dead" not in output:
        subprocess.call([path_manager.wdctl_bin_path(), "start", "mcsrouter"])


def restart_update_scheduler():
    """
    restart_update_scheduler
    """
    output = subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"])
    if "SubState=dead" not in output:
        update_scheduler = "updatescheduler"
        subprocess.call([path_manager.wdctl_bin_path(),
                         "stop", update_scheduler])
        subprocess.call([path_manager.wdctl_bin_path(),
                         "start", update_scheduler])

def request_ALC_event_to_be_sent():
    """
    Send an action to the updates schedular to resend last event if appropriate
    """
    action_name = "ALC_action_{}.xml".format(timestamp())
    action_path = os.path.join(path_manager.action_dir(), action_name)
    action_path_tmp = os.path.join(path_manager.temp_dir(), action_name)
    atomic_write(action_path, action_path_tmp, "ResendLastUpdateEvent")

def inner_main(argv):
    """
    inner_main
    """
    # pylint: disable=too-many-branches, too-many-statements

    stop_mcs_router()
    ret = 1
    usage = "Usage: register_central <MCS-Token> <MCS-URL> | register_central [options]"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option(
        "--deregister",
        dest="deregister",
        action="store_true",
        default=False)
    parser.add_option(
        "--reregister",
        dest="reregister",
        action="store_true",
        default=False)
    parser.add_option(
        "--messagerelay",
        dest="messagerelay",
        action="store",
        default=None)
    parser.add_option(
        "--proxycredentials",
        dest="proxycredentials",
        action="store",
        default=None)
    (options, args) = parser.parse_args(argv[1:])

    if options.proxycredentials is None:
        options.proxycredentials = os.environ.get("PROXY_CREDENTIALS", None)

    token = None
    url = None

    if options.deregister:
        pass
    elif options.reregister:
        pass
    elif len(args) == 2:
        (token, url) = args
    else:
        parser.print_usage()
        return 2

    config = utils_config.Config(
        path_manager.root_config(),
        mode=0o640,
        user_id=get_uid("root"),
        group_id=get_gid("sophos-spl-group")
    )

    # grep proxy from environment
    proxy = os.environ.get("https_proxy", None)
    if not proxy:
        proxy = os.environ.get("http_proxy", None)

    if proxy:
        config.set("proxy", proxy)

    if token is not None and url is not None:
        # Remove any MCS policy settings (which might override the URL & Token)
        remove_mcs_policy()

        # Add Message relays to mcs policy config
        add_options_to_policy(options.messagerelay, options.proxycredentials)

        config.set("MCSToken", token)
        config.set("MCSURL", url)

        ca_file_env = os.environ.get("MCS_CA", "")
        if ca_file_env and os.path.isfile(ca_file_env):
            LOGGER.warning("Using %s as certificate CA", ca_file_env)
            config.set("CAFILE", ca_file_env)

        config.save()

    elif options.reregister:
        # Need to get the URL and Token from the policy file, in case they have been
        # updated by Central
        policy_config = utils_config.Config(
            path_manager.mcs_policy_config(),
            config,
            mode=0o600,
            user_id=get_uid("sophos-spl-user"),
            group_id=get_gid("sophos-spl-group")
        )
        try:
            token = policy_config.get("MCSToken")
            url = policy_config.get("MCSURL")
        except KeyError:
            print("Reregister requested, but not already registered", file=sys.stderr)
            return 2

    elif options.deregister:
        print("Deregistering from Sophos Central")
        client_config = utils_config.Config(
            path_manager.sophosspl_config(),
            mode=0o600,
            user_id=get_uid("sophos-spl-user"),
            group_id=get_gid("sophos-spl-group")
        )
        client_config.set("MCSID", "reregister")
        client_config.set("MCSPassword", "")
        client_config.save()

        # cleanup console config layers
        remove_console_configuration()

        return 0

    ## register or reregister
    if token is not None and url is not None:
        ret = register(config, path_manager.install_dir(), LOGGER)

        if ret != 0:
            LOGGER.fatal("Failed to register with Sophos Central (%d)", ret)
            cleanup()
        else:
            # Successfully registered to Sophos Central
            print("Saving Sophos Central credentials")
            config.save()

            # cleanup RMS files
            safe_delete(path_manager.sophos_config_file())

            # cleanup console config layers
            if not options.reregister:
                # Only remove the configs if we are doing a new registration
                remove_console_configuration()

            remove_all_update_reports_and_config()
            start_mcs_router()
            restart_update_scheduler()
            print("Now managed by Sophos Central")

    return ret


def main(argv):
    """
    main
    """
    os.umask(0o177)
    # Create required directories
    create_dirs()
    setup_logging()
    try:
        return inner_main(argv)
    except Exception:
        LOGGER.exception("Exception while registering with SophosCentral")
        raise


if __name__ == '__main__':
    sys.exit(main(sys.argv))
