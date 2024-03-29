#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

"""
mcs_router Module
"""
#pylint: disable=no-self-use, too-few-public-methods

import builtins
import configparser
import fcntl
import logging
import logging.handlers
import os
import signal
import sys
import time
import gc

from . import sophos_https
from .mcsclient import mcs_exception, config_exception
from .utils import path_manager
from .utils.logger_utcformatter import UTCFormatter
from .utils import signal_handler

LOGGER = logging.getLogger(__name__ if __name__ !=
                           "__main___" else "mcsrouter")
LOG_LEVEL_DEFAULT = "INFO"

builtins.__dict__['REGISTER_MCS'] = False


class PidFile:
    """
    PidFile
    """
    def __init__(self, install_dir):
        """
        __init__
        """
        path_manager.INST = install_dir
        self.__m_pid_file_path = path_manager.mcs_router_pid_file()
        self.__safe_make_dirs(os.path.dirname(self.__m_pid_file_path))
        if os.path.isfile(self.__m_pid_file_path):
            LOGGER.warning("Previous mcsrouter not shutdown cleanly")

        self.__m_pid_file = open(self.__m_pid_file_path, "w")
        try:
            fcntl.lockf(self.__m_pid_file, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except IOError:
            LOGGER.error("mcsrouter already running")
            raise

        # Set close on exec so that pid file is closed in child processes
        flags = fcntl.fcntl(self.__m_pid_file, fcntl.F_GETFD)
        fcntl.fcntl(self.__m_pid_file, fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)

        self.__m_pid_file.write("%d\n" % (os.getpid()))
        self.__m_pid_file.flush()

        self.__m_pid = os.getpid()

    def __safe_make_dirs(self, path):
        """
        __safe_make_dirs
        """
        os.makedirs(path, exist_ok=True)

    def exit(self):
        """
        exit
        """
        fcntl.lockf(self.__m_pid_file, fcntl.LOCK_UN)
        self.__m_pid_file.close()
        if self.__m_pid == os.getpid():
            os.unlink(self.__m_pid_file_path)
        else:
            LOGGER.warning("not removing mcsrouter pid_file")


def create_daemon():
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
    except OSError as exception:
        # ERROR (return a tuple)
        return (exception.errno, exception.strerror)

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
        except OSError as exception:
            # ERROR (return a tuple)
            return (exception.errno, exception.strerror)

        if pid == 0:      # The second child.
            # Set secure permissions
            os.umask(0o177)
            # Close file handles
            try:
                file_descriptor = os.open("/dev/null", os.O_RDONLY)
                if file_descriptor != 0:
                    os.dup2(file_descriptor, 0)
                    os.close(file_descriptor)
            except IOError:
                pass  # ignore error closing stdin
            # Returns
        else:
            # Exit parent (the first child) of the second child.
            os._exit(0) #pylint: disable=protected-access
    else:
        return pid

    return 0


class SophosLogging:
    """
    SophosLogging
    """
    def __init__(self, install_dir):
        """
        __init__
        """
        #pylint: disable=too-many-locals
        path_manager.INST = install_dir
        log_config = path_manager.log_conf_file()
        local_log_config = path_manager.local_log_config_file()

        # Configure log level from config file if present
        readable, log_level_string = self._extract_log_level(log_config)
        if os.path.isfile(local_log_config):
            _, local_log_level_string = self._extract_log_level(local_log_config, reading_local_file=True)
            if local_log_level_string is not None:
                    log_level_string = local_log_level_string

        log_level = getattr(logging, log_level_string, logging.INFO)
        log_file = path_manager.mcs_router_log()

        root_logger = logging.getLogger()
        root_logger.setLevel(log_level)


        formatter = UTCFormatter(
            "%(process)-7d [%(asctime)s.%(msecs)03d] %(levelname)7s "
            "[%(thread)10.10d] %(name)s <> %(message)s", "%Y-%m-%dT%H:%M:%S")
        file_handler = logging.handlers.RotatingFileHandler(
            log_file, maxBytes=1024 * 1024, backupCount=5)
        file_handler.setFormatter(formatter)
        root_logger.addHandler(file_handler)

        envelope_file = path_manager.mcs_envelope_log()

        envelope_logger = logging.getLogger("ENVELOPES")
        envelope_logger.propagate = False

        if envelope_file == "":
            envelope_logger.setLevel(logging.CRITICAL)
        else:
            envelope_logger.setLevel(log_level)

            envelope_file_handler = logging.handlers.RotatingFileHandler(
                envelope_file, maxBytes=1024 * 1024, backupCount=3)

            envelope_formatter = UTCFormatter(
                "%(process)-7d [%(asctime)s.%(msecs)03d] %(levelname)7s "
                "[%(thread)10.10d] %(name)s <> %(message)s", "%Y-%m-%dT%H:%M:%S")
            envelope_file_handler.setFormatter(envelope_formatter)

            envelope_logger.addHandler(envelope_file_handler)

        if not readable:
            LOGGER.info("Log config file exists but is either empty or cannot be read.")
        root_logger_level = root_logger.getEffectiveLevel()
        LOGGER.info("Logging level: %s", str(root_logger_level))
        LOGGER.info("Logging to %s", log_file)
        sophos_https.LOGGER = logging.getLogger("sophos_https")

    def _extract_log_level(self, log_config, reading_local_file=False):
        readable = False
        if not reading_local_file:
            log_level_string = LOG_LEVEL_DEFAULT
        else:
            log_level_string = None

        try:
            if os.path.isfile(log_config):
                config_parser = configparser.ConfigParser()
                readable = config_parser.read(log_config)
                config_sections = config_parser.sections()
                for section in config_sections:
                    if section == 'mcs_router' and config_parser.has_option(section, 'VERBOSITY'):
                        log_level_string = config_parser.get('mcs_router', 'VERBOSITY')
                        break
                    elif section == 'global' and config_parser.has_option(section, 'VERBOSITY'):
                        log_level_string = config_parser.get('global', 'VERBOSITY')
                if log_level_string == 'WARN':
                    log_level_string = 'WARNING'
        except Exception as ex: # pylint: disable=broad-except
            print("Failed to parse log configuration: {}".format(ex), file=sys.stderr)
        return readable, log_level_string

    def shutdown(self):
        """
        shutdown
        """
        logging.shutdown()


class MCSRouter:
    """
    MCSRouter
    """

    def __init__(self, install_dir):
        """
        __init__
        """
        self.__m_install_dir = install_dir

    def __safe_run_forever(self, proc):
        """
        __safe_run_forever
        """
        while True:
            # Clean exits do exit
            try:
                return proc.run()
            except mcs_exception.MCSCACertificateException as exception:
                LOGGER.fatal(str(exception))
                break
            except config_exception.ConfigException:
                # it is handled in main
                raise
            except Exception: # pylint: disable=broad-except
                # Deliberately catch everything, so that we re-run mcs_router on failures
                # rather than crash
                LOGGER.critical(
                    "Caught exception at top-level; re-running.",
                    exc_info=True)
                count = gc.collect()
                LOGGER.error("GC collected %d objects", count)
                time.sleep(60)

        # noinspection PyUnreachableCode
        return 100

    def run(self):
        """
        run
        """
        proc = None
        LOGGER.info("Starting mcsrouter")
        if not hostfile_has_read_permission():
            LOGGER.warning("/etc/hosts does not have read permissions. Will have issues resolving hostnames.")

        from . import mcs
        proc = mcs.MCS(self.__m_install_dir)

        assert proc is not None
        ret = self.__safe_run_forever(proc)
        LOGGER.warning("Exiting mcsrouter")
        return ret

def hostfile_has_read_permission():
    try:
        return os.access("/etc/hosts", os.R_OK)
    except OSError:
        return False

def main():
    """
    main
    """
    signal_handler.setup_signal_handler()
    install_dir = os.environ.get("INST_DIR", None)
    if install_dir is None:
        # Go one directory up from the script's location
        arg0 = sys.argv[0]
        script_dir = os.path.dirname(os.path.realpath(arg0))
        install_dir = os.path.abspath(os.path.join(script_dir, ".."))
    path_manager.INST = install_dir
    os.umask(0o177)

    sophos_logging = SophosLogging(install_dir)
    LOGGER.info("Started with install directory set to {}".format(install_dir))
    pid_file = PidFile(install_dir)
    try:
        mgmt = MCSRouter(install_dir)
        return mgmt.run()
    except config_exception.ConfigException as exception:
        LOGGER.fatal( str(exception))
        return 1
    except Exception:
        LOGGER.critical(
            "Caught exception at top-level; exiting.",
            exc_info=True)
        raise
    finally:
        pid_file.exit()
        sophos_logging.shutdown()


if __name__ == '__main__':
    value = main()
    LOGGER.debug("mcsrouter main loop finished with exit code: {}".format(value))
    # for some weird reason, in some cases the sys.exit will not
    # really exit due to some deadlock on cleaning up and the executable
    # Performing a 'ps -elf' on the process will have the word: 'futex_?'.
    # Forcing a garbage collection seems to prevent this error case.
    # This has been first detected by the system test:
    # MCS Router Stops If MCS Certificate Cannot Be Read By Sophos-spl-user
    try:
        LOGGER.debug("run gc")
        gc.collect()
        LOGGER.debug("gc done")
    except Exception as ex:
        LOGGER.warning("Issue with garbage collection: {}".format(ex))
    sys.exit(value)
