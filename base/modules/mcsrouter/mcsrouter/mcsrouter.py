#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import gc
import sys
import os
import fcntl
import logging
import logging.handlers
import resource
import signal
import time
import __builtin__

logger = logging.getLogger(__name__)
LOG_LEVEL_DEFAULT="INFO"

__builtin__.__dict__['REGISTER_MCS'] = False

import utils.Config
import SophosHTTPS

class PidFile(object):
    def __safemakedirs(self, path):
        try:
            os.makedirs(path)
        except EnvironmentError as e:
            if e.errno == 17:
                return
            else:
                raise

    def __init__(self, installDir):
        self.__m_pidfilePath = os.path.join(installDir,"var","lock-sophosspl","mcsrouter.pid")
        self.__safemakedirs(os.path.dirname(self.__m_pidfilePath))
        if os.path.isfile(self.__m_pidfilePath):
            logger.warning("Previous mcsrouter not shutdown cleanly")

        self.__m_pidfile = open(self.__m_pidfilePath,"w")
        try:
            fcntl.lockf(self.__m_pidfile,fcntl.LOCK_EX | fcntl.LOCK_NB)
        except IOError:
            logger.error("mcsrouter already running")
            raise

        ## Set close on exec so that pid file is closed in child processes
        flags = fcntl.fcntl(self.__m_pidfile, fcntl.F_GETFD)
        fcntl.fcntl(self.__m_pidfile, fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)

        self.__m_pidfile.write("%d\n"%(os.getpid()))
        self.__m_pidfile.flush()

        self.__m_pid = os.getpid()

    def exit(self):
        fcntl.lockf(self.__m_pidfile,fcntl.LOCK_UN)
        self.__m_pidfile.close()
        if self.__m_pid == os.getpid():
            os.unlink(self.__m_pidfilePath)
        else:
            logger.warning("not removing mcsrouter pidfile")

def createDaemon():
    """Detach a process from the controlling terminal and run it in the
    background as a daemon.
    """

    try:
        # Fork a child process so the parent can exit.  This will return control
        # to the command line or shell.  This is required so that the new process
        # is guaranteed not to be a process group leader.  We have this guarantee
        # because the process GID of the parent is inherited by the child, but
        # the child gets a new PID, making it impossible for its PID to equal its
        # PGID.
        pid = os.fork()
    except OSError as e:
        return (e.errno, e.strerror)     # ERROR (return a tuple)

    if pid == 0:       # The first child.

        # Next we call os.setsid() to become the session leader of this new
        # session.  The process also becomes the process group leader of the
        # new process group.  Since a controlling terminal is associated with a
        # session, and this new session has not yet acquired a controlling
        # terminal our process now has no controlling terminal.  This shouldn't
        # fail, since we're guaranteed that the child is not a process group
        # leader.
        os.setsid()

        # When the first child terminates, all processes in the second child
        # are sent a SIGHUP, so it's ignored.
        signal.signal(signal.SIGHUP, signal.SIG_IGN)

        try:
            # Fork a second child to prevent zombies.  Since the first child is
            # a session leader without a controlling terminal, it's possible for
            # it to acquire one by opening a terminal in the future.  This second
            # fork guarantees that the child is no longer a session leader, thus
            # preventing the daemon from ever acquiring a controlling terminal.
            pid = os.fork()        # Fork a second child.
        except OSError as e:
            return (e.errno, e.strerror)  # ERROR (return a tuple)

        if pid == 0:      # The second child.
            # Set secure permissions
            os.umask(0o177)
            # Close file handles
            try:
                fd = os.open("/dev/null", os.O_RDONLY)
                if fd != 0:
                    os.dup2(fd,0)
                    os.close(fd)
            except Exception:
                pass ## ignore error closing stdin
            ## Returns
        else:
            os._exit(0)      # Exit parent (the first child) of the second child.
    else:
        return pid

    return 0

def daemonise():
    pid = createDaemon()

    if isinstance(pid,tuple):
        ## Error
        return pid
    elif pid != 0:
        ## parent
        try:
            fd = os.open("/dev/null", os.O_RDONLY)
            if fd != 0:
                os.dup2(fd,0)
                os.close(fd)
        except Exception:
            pass ## ignore error closing stdin

        os._exit(0)

class SophosLogging(object):
    def __init__(self, config, installDir):
        logconfig = config.getDefault("LOGCONFIG",os.path.join(installDir,"etc","mcsrouter.log.conf"))
        if os.path.isfile(logconfig):
            logging.config.fileConfig(logconfig)
            return

        ## Configure directly
        loglevelStr = config.getDefault("LOGLEVEL",LOG_LEVEL_DEFAULT).upper()
        loglevel = getattr(logging, loglevelStr, logging.INFO)
        logfile = config.getDefault("LOGFILE",os.path.join(installDir,"logs", "base","sophosspl","mcsrouter.log"))

        rootLogger = logging.getLogger()
        rootLogger.setLevel(loglevel)

        formatter = logging.Formatter("%(asctime)s %(levelname)s %(name)s: %(message)s")
        fileHandler = logging.handlers.RotatingFileHandler(logfile,maxBytes=1024*1024,backupCount=5)
        fileHandler.setFormatter(formatter)

        rootLogger.addHandler(fileHandler)


        if config.getDefault("CONSOLE","0") == "1":
            streamHandler = logging.StreamHandler()
            streamHandler.setFormatter(formatter)
            rootLogger.addHandler(streamHandler)

        envelopeFile = config.getDefault("ENVELOPE_LOG",
            os.path.join(installDir,"logs","base","sophosspl","mcs_envelope.log"))

        envelopeLogger = logging.getLogger("ENVELOPES")
        envelopeLogger.propagate = False


        if envelopeFile == "":
            envelopeLogger.setLevel(logging.CRITICAL)
        else:
            envelopeLogger.setLevel(logging.INFO)

            envelopeFileHandler = logging.handlers.RotatingFileHandler(envelopeFile,maxBytes=1024*1024,backupCount=3)

            envelopeFormatter = logging.Formatter("%(asctime)s: %(message)s")
            envelopeFileHandler.setFormatter(envelopeFormatter)

            envelopeLogger.addHandler(envelopeFileHandler)

        logger.debug("Logging to %s",logfile)
        SophosHTTPS.logger = logging.getLogger("SophosHTTPS")

    def shutdown(self):
        logging.shutdown()


class MCSRouter(object):
    def __init__(self, config, installDir):
        self.__m_config = config
        self.__m_installDir = installDir

    def __safeRunForever(self, proc):
        while True:
            ## Clean exits do exit
            try:
                return proc.run()
            except Exception:
                logger.critical("Caught exception at top-level; re-running.",exc_info=True)
                count = gc.collect()
                logger.error("GC collected %d objects",count)
                time.sleep(60)

        return 100

    def run(self):
        config = self.__m_config
        installDir = self.__m_installDir
        proc = None
        logger.info("Starting mcsrouter")

        ## Turn off core files
        try:
            if config.getDefault("SaveCore","0") == "1":
                logger.info("Enabling saving core files")
                resource.setrlimit(resource.RLIMIT_CORE,(resource.RLIM_INFINITY,resource.RLIM_INFINITY))
            else:
                logger.info("Disabling core files")
                resource.setrlimit(resource.RLIMIT_CORE,(0,0))
        except ValueError:
            logger.warning("Unable to set core file resource limit")

        import MCS
        proc = MCS.MCS(config,installDir)

        assert proc is not None
        ret = self.__safeRunForever(proc)
        logger.warning("Exiting mcsrouter")
        return ret

def createConfiguration(installDir, argv):
    config = utils.Config.Config(os.path.join(installDir,"etc","mcsrouter.conf"))
    config.setDefault("LOGLEVEL",LOG_LEVEL_DEFAULT)

    for arg in argv[1:]:
        if arg.startswith("--"):
            arg = arg[2:]
        if "=" in arg:
            (key,value) = arg.split("=",1)
            config.set(key,value)
        elif arg == "-v":
            config.set("LOGLEVEL","DEBUG")
        elif arg == "console":
            config.set("CONSOLE","1")
        elif arg == "daemon":
            config.set("DAEMON","1")
        elif arg == "no-daemon":
            config.set("DAEMON","0")
    return config
    
def clearTmpDirectory(installDir):
    tempDir = os.path.join(installDir, "tmp")
    if os.path.exists(tempDir):
        for files in os.listdir(tempDir):
            try:
                os.unlink(files)
            except (OSError, IOError):
                pass

def main(argv):
    logger.setLevel(10)

    installDir = os.environ.get("INST",None)
    if installDir is None:
        # Go one directory up from the script's location
        arg0 = sys.argv[0]
        scriptDir = os.path.dirname(os.path.realpath(arg0))
        installDir = os.path.abspath(os.path.join(scriptDir, ".."))
    logger.info("Started with install directory set to " + installDir)
    clearTmpDirectory(installDir)
    os.umask(0o177)
    config = createConfiguration(installDir, argv)

    if config.getDefault("DAEMON","1") == "1":
        daemonise()

    sophosLogging = SophosLogging(config,installDir)
    pidfile = PidFile(installDir)
    try:
        mgmt = MCSRouter(config,installDir)
        return mgmt.run()
    except Exception:
        logger.critical("Caught exception at top-level; exiting.",exc_info=True)
        raise
    finally:
        pidfile.exit()
        sophosLogging.shutdown()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
