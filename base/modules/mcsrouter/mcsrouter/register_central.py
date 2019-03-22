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
from .mcsclient import mcs_exception
from .mcsclient import MCSConnection
from . import mcs as MCS
from .utils import path_manager

from .utils import SECObfuscation

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

# So don't need to add it
## Already in mcsrouter.zip

# ## munge sys.path to add mcsrouter_mcs.zip
# sys.path.append(os.path.join(INST,"engine","mcsrouter_mcs.zip"))


def setup_logging():
    """
    setup_logging
    """
    root_LOGGER = logging.getLogger()
    root_LOGGER.setLevel(logging.DEBUG)

    formatter = logging.Formatter(
        "%(asctime)s %(levelname)s %(name)s: %(message)s")

    log_file = path_manager.register_log()

    file_handler = logging.handlers.RotatingFileHandler(
        log_file, maxBytes=1024 * 1024, backupCount=5)
    file_handler.setFormatter(formatter)
    file_handler.setLevel(logging.INFO)
    root_LOGGER.addHandler(file_handler)

    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)
    debug = os.environ.get("MCS_DEBUG", None)
    if debug:
        stream_handler.setLevel(logging.DEBUG)
    else:
        stream_handler.setLevel(logging.ERROR)
    root_LOGGER.addHandler(stream_handler)


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
    import mcsrouter.targetsystem
    return mcsrouter.targetsystem.target_system()


def cleanup():
    """
    cleanup
    """
    root_config = path_manager.root_config()
    sophosav_config = path_manager.sophosspl_config()
    for configFile in [root_config, sophosav_config]:
        try:
            os.unlink(configFile)
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


def register(config, INST, logger):
    """
    register
    """
    # Do a register operation so that we can be sure that we have connectivity
    print("Registering with Sophos Central")
    mcs = MCS.MCS(config, INST)

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
            logger.fatal(
                "Failed to connect to Sophos Central: Check URL: %s",
                url,
                exc_info=True)
            ret = 4
            break
        except MCSConnection.MCSHttpException as exception:
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

    return ret


def remove_mcs_policy():
    """
    remove_mcs_policy
    """
    safe_delete(path_manager.mcs_policy_config())
    safe_delete(path_manager.mcs_policy_file())


def get_uid(uid_string):
    """
    get_uid
    """
    # Try as a integer
    try:
        return int(uid_string, 10)  # id specified in decimal
    except ValueError:
        pass

    # Try in the passwd database
    import pwd
    try:
        password_struct = pwd.getpwnam(uid_string)
        return password_struct.pw_uid
    except KeyError:
        # The text doesn't exist as a user
        pass
    return 0


def get_gid(gid_string):
    """
    get_gid
    """
    # Try as a integer
    try:
        return int(gid_string, 10)  # id specified in decimal
    except ValueError:
        pass

    # Try in the passwd database
    import grp
    try:
        group_struct = grp.getgrnam(gid_string)
        return group_struct.gr_gid
    except KeyError:
        # The text doesn't exist as a group
        pass
    return 0


class RandomGenerator(object):
    """
    RandomGenerator
    """

    def random_bytes(self, size):
        """
        random_bytes
        """
        return bytearray(random.getrandbits(8) for _ in xrange(size))


def add_options_to_policy(relays, proxycredentials):
    """
    add_options_to_policy
    """
    if relays is None and proxycredentials is None:
        return

    policy_config_path = path_manager.mcs_policy_config()
    policy_config = utils_config.Config(policy_config_path)

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
        obfuscated = SECObfuscation.obfuscate(
            SECObfuscation.ALGO_AES256,
            proxycredentials,
            RandomGenerator())
        policy_config.set("mcs_policy_proxy_credentials", obfuscated)

    policy_config.save()
    os.chown(
        policy_config_path,
        get_uid("sophos-spl-user"),
        get_gid("sophos-spl-group"))
    os.chmod(policy_config_path, 0o600)


def set_file_permissions():
    """
    set_file_permissions
    """
    mcs_config = path_manager.root_config()
    sspl_config = path_manager.sophosspl_config()

    uid = get_uid("sophos-spl-user")
    gid = get_gid("sophos-spl-group")
    os.chown(mcs_config, 0, gid)
    os.chmod(mcs_config, 0o640)
    os.chown(sspl_config, uid, gid)


def remove_console_configuration():
    """
    remove_console_configuration
    """
    policy_dir = path_manager.policy_dir()
    for policy_file in os.listdir(policy_dir):
        file_path = os.path.join(policy_dir, policy_file)
        if os.path.isfile(file_path):
            safe_delete(file_path)


def remove_all_update_reports():
    """
    remove_all_update_reports
    """
    for file in glob.glob(
            "{}/report*.json".format(path_manager.update_var_path())):
        os.remove(file)


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


def inner_main(argv):
    """
    inner_main
    """
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

    top_config = utils_config.Config(path_manager.mcs_router_conf())
    debug = os.environ.get("MCS_DEBUG", None)
    if debug:
        top_config.set("LOGLEVEL", "DEBUG")
        top_config.save()

    config = utils_config.Config(path_manager.root_config(), top_config)

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
            path_manager.mcs_policy_config(), config)
        try:
            token = policy_config.get("MCSToken")
            url = policy_config.get("MCSURL")
        except KeyError:
            print("Reregister requested, but not already registered", file=sys.stderr)
            return 2

    elif options.deregister:
        print("Deregistering from Sophos Central")
        client_config = utils_config.Config(path_manager.sophosspl_config())
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

            set_file_permissions()

            # cleanup RMS files
            safe_delete(path_manager.sophos_config_file())

            # cleanup last reported update event
            safe_delete(path_manager.get_update_last_event_file())

            # cleanup console config layers
            if not options.reregister:
                # Only remove the configs if we are doing a new registration
                remove_console_configuration()

            remove_all_update_reports()
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
    """
    __name__
    """
    sys.exit(main(sys.argv))
