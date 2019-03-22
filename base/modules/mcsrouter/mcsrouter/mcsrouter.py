#!/usr/bin/env python
"""
mcsrouter Module
"""

from __future__ import print_function, division, unicode_literals

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

import utils.path_manager as path_manager
import utils.config
import sophos_https


LOGGER = logging.getLogger(__name__ if __name__ !=
                           "__main___" else "mcsrouter")
LOG_LEVEL_DEFAULT = "INFO"

__builtin__.__dict__['REGISTER_MCS'] = False


class PidFile(object):
    """
    PidFile
    """

    def __safe_make_dirs(self, path):
        """
        __safe_make_dirs
        """
        try:
            os.makedirs(path)
        except EnvironmentError as exception:
            if exception.errno == 17:
                return
            else:
                raise

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
            except Exception:
                pass  # ignore error closing stdin
            # Returns
        else:
            # Exit parent (the first child) of the second child.
            os._exit(0)
    else:
        return pid

    return 0


def daemonise():
    """
    daemonise
    """
    pid = create_daemon()

    if isinstance(pid, tuple):
        # Error
        return pid
    elif pid != 0:
        # parent
        try:
            file_descriptor = os.open("/dev/null", os.O_RDONLY)
            if file_descriptor != 0:
                os.dup2(file_descriptor, 0)
                os.close(file_descriptor)
        except Exception:
            pass  # ignore error closing stdin

        os._exit(0)


class SophosLogging(object):
    """
    SophosLogging
    """

    def __init__(self, config, install_dir):
        """
        __init__
        """
        path_manager.INST = install_dir
        log_config = config.get_default(
            "LOGCONFIG", path_manager.log_conf_file())
        if os.path.isfile(log_config):
            logging.config.fileConfig(log_config)
            return

        # Configure directly
        log_level_string = config.get_default(
            "LOGLEVEL", LOG_LEVEL_DEFAULT).upper()
        log_level = getattr(logging, log_level_string, logging.INFO)
        log_file = config.get_default("LOGFILE", path_manager.mcs_router_log())

        root_LOGGER = logging.getLogger()
        root_LOGGER.setLevel(log_level)

        formatter = logging.Formatter(
            "%(asctime)s %(levelname)s %(name)s: %(message)s")
        file_handler = logging.handlers.RotatingFileHandler(
            log_file, maxBytes=1024 * 1024, backupCount=5)
        file_handler.setFormatter(formatter)

        root_LOGGER.addHandler(file_handler)

        if config.get_default("CONSOLE", "0") == "1":
            stream_handler = logging.StreamHandler()
            stream_handler.setFormatter(formatter)
            root_LOGGER.addHandler(stream_handler)

        envelope_file = config.get_default(
            "ENVELOPE_LOG", path_manager.mcs_envelope_log())

        envelope_LOGGER = logging.getLogger("ENVELOPES")
        envelope_LOGGER.propagate = False

        if envelope_file == "":
            envelope_LOGGER.setLevel(logging.CRITICAL)
        else:
            envelope_LOGGER.setLevel(log_level)

            envelope_file_handler = logging.handlers.RotatingFileHandler(
                envelope_file, maxBytes=1024 * 1024, backupCount=3)

            envelopeFormatter = logging.Formatter("%(asctime)s: %(message)s")
            envelope_file_handler.setFormatter(envelopeFormatter)

            envelope_LOGGER.addHandler(envelope_file_handler)

        LOGGER.debug("Logging to %s", log_file)
        sophos_https.LOGGER = logging.getLogger("sophos_https")

    def shutdown(self):
        """
        shutdown
        """
        logging.shutdown()


class MCSRouter(object):
    """
    MCSRouter
    """

    def __init__(self, config, install_dir):
        """
        __init__
        """
        self.__m_config = config
        self.__m_install_dir = install_dir

    def __safe_run_forever(self, proc):
        """
        __safe_run_forever
        """
        while True:
            # Clean exits do exit
            try:
                return proc.run()
            except Exception:
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
        config = self.__m_config
        install_dir = self.__m_install_dir
        proc = None
        LOGGER.info("Starting mcsrouter")

        # Turn off core files
        try:
            if config.get_default("SaveCore", "0") == "1":
                LOGGER.info("Enabling saving core files")
                resource.setrlimit(
                    resource.RLIMIT_CORE,
                    (resource.RLIM_INFINITY,
                     resource.RLIM_INFINITY))
            else:
                LOGGER.info("Disabling core files")
                resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        except ValueError:
            LOGGER.warning("Unable to set core file resource limit")

        import mcs
        proc = mcs.MCS(config, install_dir)

        assert proc is not None
        ret = self.__safe_run_forever(proc)
        LOGGER.warning("Exiting mcsrouter")
        return ret


def create_configuration(argv):
    """
    create_configuration
    """
    config = utils.config.Config(path_manager.mcs_router_conf())
    config.set_default("LOGLEVEL", LOG_LEVEL_DEFAULT)

    for arg in argv[1:]:
        if arg.startswith("--"):
            arg = arg[2:]
        if "=" in arg:
            (key, value) = arg.split("=", 1)
            config.set(key, value)
        elif arg == "-v":
            config.set("LOGLEVEL", "DEBUG")
        elif arg == "console":
            config.set("CONSOLE", "1")
        elif arg == "daemon":
            config.set("DAEMON", "1")
        elif arg == "no-daemon":
            config.set("DAEMON", "0")
    return config


def clear_tmp_directory():
    """
    clear_tmp_directory
    """
    temp_dir = path_manager.temp_dir()
    if os.path.exists(temp_dir):
        for files in os.listdir(temp_dir):
            try:
                os.unlink(files)
            except (OSError, IOError):
                pass


def main(argv):
    """
    main
    """
    install_dir = os.environ.get("INST_DIR", None)
    if install_dir is None:
        # Go one directory up from the script's location
        arg0 = sys.argv[0]
        script_dir = os.path.dirname(os.path.realpath(arg0))
        install_dir = os.path.abspath(os.path.join(script_dir, ".."))
    path_manager.INST = install_dir
    clear_tmp_directory()
    os.umask(0o177)
    config = create_configuration(argv)

    if config.get_default("DAEMON", "1") == "1":
        daemonise()

    sophos_logging = SophosLogging(config, install_dir)
    LOGGER.info("Started with install directory set to " + install_dir)
    pid_file = PidFile(install_dir)
    try:
        mgmt = MCSRouter(config, install_dir)
        return mgmt.run()
    except Exception:
        LOGGER.critical(
            "Caught exception at top-level; exiting.",
            exc_info=True)
        raise
    finally:
        pid_file.exit()
        sophos_logging.shutdown()


if __name__ == '__main__':
    sys.exit(main(sys.argv))
