#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import random
import sys
import time
import optparse
import errno
import __builtin__

import logging
import logging.handlers
logger = logging.getLogger(__name__)

__builtin__.__dict__['REGISTER_MCS'] = True

def safeMkdir(directory):
    if not os.path.exists(directory):
        # Use a try here to catch race condition where directory is created after checking
        # it doesn't exist
        try:
            oldmask = os.umask(0o002)
            os.makedirs(directory)
            os.umask(oldmask)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
    return

INST = os.environ['INST_DIR']
BASE_DIR = os.environ['BASE_DIR']

## So don't need to add it
## Already in mcsrouter.zip

# ## munge sys.path to add mcsrouter_mcs.zip
# sys.path.append(os.path.join(INST,"engine","mcsrouter_mcs.zip"))

from .utils import Config as utilsConfig
from .mcsclient import MCSException
from .mcsclient import MCSConnection
from . import MCS
#~ import SECObfuscation

def setupLogging():
    rootLogger = logging.getLogger()
    rootLogger.setLevel(logging.DEBUG)

    formatter = logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s")

    logfile = os.path.join(INST, "logs", "base", "registerCentral.log")

    fileHandler = logging.handlers.RotatingFileHandler(logfile,maxBytes=1024*1024,backupCount=5)
    fileHandler.setFormatter(formatter)
    fileHandler.setLevel(logging.INFO)
    rootLogger.addHandler(fileHandler)

    streamHandler = logging.StreamHandler()
    streamHandler.setFormatter(formatter)
    debug = os.environ.get("MCS_DEBUG",None)
    if debug:
        streamHandler.setLevel(logging.DEBUG)
    else:
        streamHandler.setLevel(logging.ERROR)
    rootLogger.addHandler(streamHandler)

def createDirs(INST):
    safeMkdir(os.path.join(INST, "var", "cache", "mcs_fragmented_policies"))
    safeMkdir(os.path.join(INST, "var", "lock-sophosspl"))

    safeMkdir(os.path.join(INST, "logs", "base", "sophosspl"))

    safeMkdir(os.path.join(INST, "etc", "sophosspl"))

    paths = ["action",  "event", "policy", "status" ]
    for path in paths:
        safeMkdir(os.path.join(INST,"base","mcs", path))

def getTargetSystem():
    import mcsrouter.targetsystem
    return mcsrouter.targetsystem.TargetSystem()

def getInstallationLibPath():
    installDir = INST
    return os.path.join(installDir,"base","lib")+":"+os.path.join(installDir,"base","lib64")

def cleanup():
    rootConfig = os.path.join(INST,"etc","mcs.config")
    sophosavConfig = os.path.join(INST,"etc","sophosspl","mcs.config") 
    for configFile in [rootConfig, sophosavConfig]:
        try:
            os.unlink(configFile)
        except OSError:
            pass

def safeDelete(path):
    try:
        os.unlink(path)
    except IOError:
        pass
    except OSError:
        pass

def register(config, INST, logger):
    ## Do a register operation so that we can be sure that we have connectivity
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
        except MCSException.MCSConnectionFailedException as e:
            url = config.get("MCSURL")
            print("ERROR: Failed to connect to Sophos Central: Check URL: %s"%url, file=sys.stderr)
            logger.fatal("Failed to connect to Sophos Central: Check URL: %s", url)
            ret = 4
            break
        except MCSConnection.MCSHttpException as e:
            if e.errorCode() == 401:
                print("ERROR: Authentication error from Sophos Central: Check Token",file=sys.stderr)
                logger.fatal("ERROR: Authentication error from Sophos Central: Check Token")
                ret = 6
                break
            else:
                logger.warning("ERROR: HTTP Error from Sophos Central (%d)"%(e.errorCode()))
                logger.debug(str(e.headers()))
                logger.debug(e.body())

                count -= 1
                if count > 0:
                    time.sleep(sleep)
                    sleep += 2
                    logger.info("Retrying registration")
                    continue
                else:
                    print("ERROR: Repeated HTTP Errors from Sophos Central (%d)"%(e.errorCode()),file=sys.stderr)
                    ret = 5
                    break

    return ret

def removeMCSPolicy():
    safeDelete(os.path.join(INST,"base","etc","sophosspl","mcs_policy.config"))
    safeDelete(os.path.join(INST,"base","mcs","policy","MCS-25_policy.xml"))

def getUID(uidText):
    """
    """
    ## Try as a integer
    try:
        return int(uidText,10) ## id specified in decimal
    except ValueError:
        pass

    ## Try in the passwd database
    import pwd
    try:
        pwstruct = pwd.getpwnam(uidText)
        return pwstruct.pw_uid
    except KeyError:
        ## The text doesn't exist as a user
        pass
    return 0

def getGID(gidText):
    """
    """
    ## Try as a integer
    try:
        return int(gidText,10) ## id specified in decimal
    except ValueError:
        pass

    ## Try in the passwd database
    import grp
    try:
        grstruct = grp.getgrnam(gidText)
        return grstruct.gr_gid
    except KeyError:
        ## The text doesn't exist as a group
        pass
    return 0

class RandomGenerator(object):
    def randomBytes(self, size):
        return bytearray(random.getrandbits(8) for _ in xrange(size))

def addOptionsToPolicy(relays, proxy_credentials):
    if relays is None and proxy_credentials is None:
        return

    policy_config_path = os.path.join(INST,"base","etc","sophosspl","mcs_policy.config")
    policy_config = utilsConfig.Config(policy_config_path)

    if relays is not None:
        for index, relay in enumerate(relays.split(";"), 1):
            priorityKey = "message_relay_priority%d"%index
            portKey = "message_relay_port%d"%index
            addressKey = "message_relay_address%d"%index
            idKey = "message_relay_id%d"%index

            address, priority, relayid = relay.split(",")
            hostname, port = address.split(":")

            policy_config.set(addressKey,hostname)
            policy_config.set(portKey,port)
            policy_config.set(priorityKey,priority)
            policy_config.set(idKey, relayid)

    if proxy_credentials is not None:
        obfuscated = SECObfuscation.obfuscate(SECObfuscation.ALGO_AES256,proxy_credentials,RandomGenerator())
        policy_config.set("mcs_policy_proxy_credentials",obfuscated)


    policy_config.save()
    os.chown(policy_config_path, getUID("sophosspl"), getGID("sophosspl"))
    os.chmod(policy_config_path, 0o600)


def removeConsoleConfiguration():
    policy_dir = os.path.join(INST,"policy")
    for policy_file in os.listdir(policy_dir):
        file_path = os.path.join(INST,"policy", policy_file)
        if os.path.isfile(file_path):
            safeDelete(file_path)

def innerMain(argv):
    ret = 1
    usage = "Usage: registerCentral <MCS-Token> <MCS-URL> | registerCentral [options]"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("--deregister", dest="deregister", action="store_true", default=False)
    parser.add_option("--reregister", dest="reregister", action="store_true", default=False)
    parser.add_option("--messagerelay", dest="messagerelay", action="store", default=None)
    parser.add_option("--proxycredentials", dest="proxycredentials", action="store", default=None)
    (options, args) = parser.parse_args(argv[1:])

    if options.proxycredentials is None:
        options.proxycredentials = os.environ.get("PROXY_CREDENTIALS",None)

    token = None
    url = None

    if options.deregister:
        pass
    elif options.reregister:
        pass
    elif len(args) == 2:
        ( token, url ) = args
    else:
        parser.print_usage()
        return 2

    topconfig = utilsConfig.Config(os.path.join(INST,"base","etc","mcsrouter.conf"))
    debug = os.environ.get("MCS_DEBUG",None)
    if debug:
        topconfig.set("LOGLEVEL","DEBUG")
        topconfig.save()

    config = utilsConfig.Config(os.path.join(INST,"base","etc","mcs.config"),topconfig)

    ## grep proxy from environment
    proxy = os.environ.get("https_proxy",None)
    if not proxy:
        proxy = os.environ.get("http_proxy",None)

    if proxy:
        config.set("proxy",proxy)


    if token is not None and url is not None:
        ## Remove any MCS policy settings (which might override the URL & Token)
        removeMCSPolicy()

        ## Add Message relays to mcs policy config
        addOptionsToPolicy(options.messagerelay, options.proxycredentials)

        config.set("MCSToken",token)
        config.set("MCSURL",url)
        config.save()

    elif options.reregister:
        ## Need to get the URL and Token from the policy file, in case they have been
        ## updated by Central
        policy_config = utilsConfig.Config(os.path.join(INST,"base", "etc", "sophosspl", "mcs_policy.config"),config)
        try:
            token = policy_config.get("MCSToken")
            url = policy_config.get("MCSURL")
        except KeyError:
            print("Reregister requested, but not already registered", file=sys.stderr)
            return 2

    elif options.deregister:
        print("Deregistering from Sophos Central")
        clientconfig = utilsConfig.Config(os.path.join(INST, "base", "etc", "sophosspl", "mcs.config"))
        clientconfig.set("MCSID", "reregister")
        clientconfig.set("MCSPassword", "")
        clientconfig.save()

        ## cleanup console config layers
        removeConsoleConfiguration()

        return 0

    ## register or reregister
    if token is not None and url is not None:
        ret = register(config, INST, logger)

        if ret != 0:
            logger.fatal("Failed to register with Sophos Central (%d)", ret)
            cleanup()
        else:
            ## Successfully registered to Sophos Central
            print("Saving Sophos Central credentials")
            config.save()

            ## cleanup RMS files
            safeDelete(os.path.join(INST,"policy","sophos.config"))

            ## cleanup last reported update event
            safeDelete(os.path.join(INST, "base", "var", "run", "sophosspl", "update.last_event"))

            ## cleanup console config layers
            if not options.reregister:
                ## Only remove the configs if we are doing a new registration
                removeConsoleConfiguration()

            print("Now managed by Sophos Central")

    return ret

def main(argv):
    os.umask(0o177)
    # Create required directories
    createDirs(INST)
    setupLogging()
    try:
        return innerMain(argv)
    except Exception:
        logger.exception("Exception while registering with SophosCentral")
        raise


if __name__ == '__main__':
    sys.exit(main(sys.argv))
