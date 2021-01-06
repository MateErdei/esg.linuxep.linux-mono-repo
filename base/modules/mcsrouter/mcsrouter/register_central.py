#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
register_central Module
"""



import argparse
import builtins
import glob
import logging
import logging.handlers
import os
import shutil
import subprocess
import sys
import time

from . import mcs as MCS
from .mcsclient import mcs_connection
from .mcsclient import mcs_exception
from .utils import config as utils_config
from .utils import path_manager
from .utils import sec_obfuscation
from .utils.byte2utf8 import to_utf8
from .utils.get_ids import get_gid, get_uid
from .utils.logger_utcformatter import UTCFormatter

LOGGER = logging.getLogger(__name__)

builtins.__dict__['REGISTER_MCS'] = True


def safe_mkdir(directory):
    """
    safe_mkdir
    """
    if not os.path.exists(directory):
        # Use exist_ok option to handle race condition where directory is created after checking
        # it doesn't exist
        old_mask = os.umask(0o002)
        os.makedirs(directory, exist_ok=True)
        os.umask(old_mask)

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
    console_formarter = logging.Formatter("%(levelname)7s: %(message)s")
    stream_handler.setFormatter(console_formarter)
    stream_handler.setLevel(logging.WARN)
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
        except mcs_exception.MCSCACertificateException as exception:
            LOGGER.fatal(str(exception))
            break
        except mcs_exception.MCSConnectionFailedException as ex:
            url = config.get("MCSURL")
            logger.warning("Connection failure. Reason: {}".format(str(ex)))
            logger.error(
                "Failed to connect to Sophos Central: Check URL: {}.".format(url))
            ret = 4
            break
        except mcs_connection.MCSHttpException as exception:
            if exception.error_code() == 401:
                logger.debug("HTTPException {}".format(exception))
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


def add_options_to_policy(relays, proxycredentials):
    """
    add_options_to_policy
    """
    if relays is None and proxycredentials is None:
        return

    policy_config = utils_config.Config(
        path_manager.mcs_policy_config(),
        mode=0o600,
        user_id=get_uid("sophos-spl-local"),
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
        cred_encoded = proxycredentials.encode('utf-8')
        obfuscated = sec_obfuscation.obfuscate(
            sec_obfuscation.ALGO_AES256,
            cred_encoded)
        policy_config.set("mcs_policy_proxy_credentials", to_utf8(obfuscated))

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
            "{}/update_report*.json".format(path_manager.update_report_path())):
        safe_delete(file_to_remove)
    safe_delete(os.path.join(path_manager.update_report_path(), "update_config.json"))


def stop_mcs_router():
    """
    stop_mcs_router
    """
    output = to_utf8(subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"]))
    if "SubState=dead" not in output:
        LOGGER.info("Stop mcsrouter")
        subprocess.call([path_manager.wdctl_bin_path(), "stop", "mcsrouter"])


def start_mcs_router():
    """
    start_mcs_router
    """
    output = to_utf8(subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"]))
    if "SubState=dead" not in output:
        LOGGER.info("Start mcsrouter")
        subprocess.call([path_manager.wdctl_bin_path(), "start", "mcsrouter"])

def restart_management_agent():
    """
    restart_management_agent
    """
    output = to_utf8(subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"]))
    if "SubState=dead" not in output:
        LOGGER.info("Restart managementagent")
        subprocess.call([path_manager.wdctl_bin_path(), "stop", "managementagent"])
        subprocess.call([path_manager.wdctl_bin_path(), "start", "managementagent"])


def restart_update_scheduler():
    """
    restart_update_scheduler
    """
    output = to_utf8(subprocess.check_output(
        ["systemctl", "show", "-p", "SubState", "sophos-spl"]))
    if "SubState=dead" not in output:
        LOGGER.info("Restart update scheduler")
        update_scheduler = "updatescheduler"
        subprocess.call([path_manager.wdctl_bin_path(),
                         "stop", update_scheduler])
        subprocess.call([path_manager.wdctl_bin_path(),
                         "start", update_scheduler])


def inner_main(argv):
    """
    inner_main
    """
    # pylint: disable=too-many-branches, too-many-statements

    stop_mcs_router()
    ret = 1
    usage = "Usage: register_central <MCS-Token> <MCS-URL> | register_central [options]"
    parser = argparse.ArgumentParser(usage=usage)
    parser.add_argument(
        "--deregister",
        dest="deregister",
        action="store_true",
        default=False)
    parser.add_argument(
        "--reregister",
        dest="reregister",
        action="store_true",
        default=False)
    parser.add_argument(
        "--messagerelay",
        dest="messagerelay",
        action="store",
        default=None)
    parser.add_argument(
        "--proxycredentials",
        dest="proxycredentials",
        action="store",
        default=None)
    parser.add_argument('token', help='MCS Token to register with Central',
                        nargs='?', default=None)
    parser.add_argument('url', help='MCS URL to use to register with Central',
                        nargs='?', default=None)

    options = parser.parse_args(argv[1:])
    if options.proxycredentials is None:
        options.proxycredentials = os.environ.get("PROXY_CREDENTIALS", None)

    token = options.token
    url = options.url

    if options.deregister:
        pass
    elif options.reregister:
        pass
    elif token is None or url is None:
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
            if os.path.isfile(path_manager.ca_env_override_flag_path()):
                LOGGER.warning("Using %s as certificate CA", ca_file_env)
                config.set("CAFILE", ca_file_env)
            else:
                LOGGER.warning(
                    "Cannot use {} as certificate CA without override flag: {}".format(
                        ca_file_env,
                        path_manager.ca_env_override_flag_path()))

        config.save()

    elif options.reregister:
        # Need to get the URL and Token from the policy file, in case they have been
        # updated by Central
        policy_config = utils_config.Config(
            path_manager.mcs_policy_config(),
            config,
            mode=0o600,
            user_id=get_uid("sophos-spl-local"),
            group_id=get_gid("sophos-spl-group")
        )
        try:
            token = policy_config.get("MCSToken")
            url = policy_config.get("MCSURL")
        except KeyError:
            LOGGER.warning("Reregister requested, but not already registered")
            return 2

    elif options.deregister:
        LOGGER.warning("Deregistering from Sophos Central")
        client_config = utils_config.Config(
            path_manager.sophosspl_config(),
            mode=0o600,
            user_id=get_uid("sophos-spl-local"),
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
            LOGGER.info("Saving Sophos Central credentials")
            config.save()

            # cleanup RMS files
            safe_delete(path_manager.sophos_config_file())

            # cleanup console config layers
            if not options.reregister:
                # Only remove the configs if we are doing a new registration
                remove_console_configuration()

            remove_all_update_reports_and_config()
            start_mcs_router()
            restart_management_agent()
            restart_update_scheduler()
            message = "Now managed by Sophos Central"
            LOGGER.info(message)
            print(message)

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
