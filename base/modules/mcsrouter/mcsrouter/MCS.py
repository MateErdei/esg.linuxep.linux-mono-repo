#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import print_function, division, unicode_literals

import errno
import os
import socket
import time
import httplib
import select
import logging
import random

logger = logging.getLogger(__name__)

import Computer
import adapters.AgentAdapter
import adapters.EventReceiver
import adapters.AppProxyAdapter
import adapters.MCSAdapter
import adapters.GenericAdapter
import mcsclient.MCSConnection
import mcsclient.MCSCommands
import mcsclient.MCSException
import mcsclient.StatusCache
import mcsclient.StatusEvent
import mcsclient.StatusTimer
import mcsclient.Events
import mcsclient.EventsTimer
import utils.Config
import utils.Timestamp
import utils.IdManager
import utils.SignalHandler
import utils.DirectoryWatcher
import utils.PluginRegistry

import utils.PathManager as PathManager


class CommandCheckInterval(object):
    DEFAULT_COMMAND_POLLING_INTERVAL = 20

    def __init__(self, config):
        self.__m_config = config
        self.__m_command_check_interval_minimum = 0
        self.__m_command_check_interval_maximum = 0

        self.__m_command_check_maximum_retry_number = self.__m_config.get_int(
            "COMMAND_CHECK_MAXIMUM_RETRY_NUMBER", 10)
        self.__m_command_check_base_retry_delay = self.__m_config.get_int(
            "COMMAND_CHECK_BASE_RETRY_DELAY", 20)
        self.__m_command_check_semi_permanent_error_retry_delay = self.__m_config.get_int(
            "COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY", self.__m_command_check_base_retry_delay * 2)

        self.set()

    def get(self):
        return self.__m_command_check_interval

    def __get_minimum(self):
        interval_min = self.__m_config.get_int(
            "COMMAND_CHECK_INTERVAL_MINIMUM",
            self.DEFAULT_COMMAND_POLLING_INTERVAL)
        if self.__m_command_check_interval_minimum != interval_min:
            self.__m_command_check_interval_minimum = interval_min
            logger.debug("COMMAND_CHECK_INTERVAL_MINIMUM=%d", interval_min)
        return interval_min

    def __get_maximum(self):
        interval_max = self.__m_config.get_int(
            "COMMAND_CHECK_INTERVAL_MAXIMUM",
            self.DEFAULT_COMMAND_POLLING_INTERVAL)
        if self.__m_command_check_interval_maximum != interval_max:
            self.__m_command_check_interval_maximum = interval_max
            logger.debug("COMMAND_CHECK_INTERVAL_MAXIMUM=%d", interval_max)
        return interval_max

    def set(self, val=None):
        if val is None:
            val = self.__m_command_check_base_retry_delay
        val = max(val, self.__get_minimum())
        val = min(val, self.__get_maximum())
        self.__m_command_check_interval = val
        return self.__m_command_check_interval

    def increment(self, val=None):
        if val is None:
            val = self.__m_command_check_base_retry_delay
        self.set(self.__m_command_check_interval + val)

    def set_on_error(self, error_count, transient=True):
        max_retry_number = self.__m_command_check_maximum_retry_number
        retry_number = min(error_count + 1, max_retry_number)
        if transient:
            base_retry_delay = self.__m_command_check_base_retry_delay
        else:
            # Semi-Permanent - From MCS spec section 12 - "Consequently, the
            # same back-off algorithm as for transient failures shall be used
            # but with a larger base delay."
            base_retry_delay = self.__m_command_check_semi_permanent_error_retry_delay

        # From MCS spec 12.1.1:
        ###     retry_delay = random(base_retry_delay * 2 ^ (retry_number - 1))
        # Where random is a function that generates a random number in the
        # range 0 to the value of its parameter
        retry_delay = random.uniform(
            0, base_retry_delay * (2 ** (retry_number - 1)))
        self.set(retry_delay)
        logger.info("[backoff] waiting %fs after %d failures",
                    self.__m_command_check_interval, error_count)


class MCS(object):
    def __init__(self, config, install_dir):
        PathManager.INST = install_dir

        self.__m_comms = None
        config.set_default(
            "MCSURL",
            "https://mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep")
        fixed_config = utils.Config.Config(
            filename=PathManager.root_config(),
            parent_config=config)
        self.__m_policy_config = utils.Config.Config(
            filename=PathManager.mcs_policy_config(),
            parent_config=fixed_config)
        self.__m_config = utils.Config.Config(
            filename=PathManager.sophosspl_config(),
            parent_config=self.__m_policy_config)
        config = self.__m_config

        status_latency = self.__m_config.get_int("STATUS_LATENCY", 30)
        logger.debug("STATUS_LATENCY=%d", status_latency)
        self.__m_status_timer = mcsclient.StatusTimer.StatusTimer(
            status_latency,
            self.__m_config.get_int("STATUS_INTERVAL", 60 * 60 * 24)
        )

        # Configure fragmented policy cache directory
        mcsclient.MCSCommands.FragmentedPolicyCommand.FRAGMENT_CACHE_DIR = PathManager.fragmented_policies_dir()

        # Create computer
        self.__m_computer = Computer.Computer()
        self.__m_mcs_adapter = adapters.MCSAdapter.MCSAdapter(
            install_dir, self.__m_policy_config, self.__m_config)

        self.__m_computer.add_adapter(self.__m_mcs_adapter)

        #~ apps = [ "ALC", "SAV", "HBT", "NTP", "SHS", "SDU", "UC", "MR" ]
        self.__plugin_registry = utils.PluginRegistry.PluginRegistry(
            install_dir)
        apps, ignored = self.__plugin_registry.added_and_removed_app_ids()

        for app in apps:
            self.__m_computer.add_adapter(
                adapters.GenericAdapter.GenericAdapter(
                    app, install_dir))

        # AppsProxy needs to report all AppIds apart from AGENT and APPSPROXY
        app_ids = self.__m_computer.get_app_ids()
        self.__m_computer.add_adapter(
            adapters.AppProxyAdapter.AppProxyAdapter(app_ids))

        self.__m_agent = adapters.AgentAdapter.AgentAdapter()
        self.__m_computer.add_adapter(self.__m_agent)

        # TODO: reload on MCS policy change (and any other configurables? Event
        # & status regulation?)
        self.__m_command_check_interval = CommandCheckInterval(config)

    def startup(self):
        """
        Connect and register if required
        """
        config = self.__m_config

        comms = mcsclient.MCSConnection.MCSConnection(
            config,
            install_dir=PathManager.install_dir()
        )

        self.__update_user_agent(comms)

        # Check capabilities
        capabilities = comms.capabilities()
        logger.info("Capabilities=%s", capabilities)
        # TODO parse and verify if we need something beyond baseline

        self.__m_comms = comms

    def __get_mcs_token(self):
        return self.__m_config.get_default("MCSToken", "unknown")

    def __update_user_agent(self, comms=None):
        if comms is None:
            comms = self.__m_comms
            if comms is None:
                return

        product_version_file = os.path.join(
            PathManager.install_dir(), "engine", "savVersion")
        try:
            product_version = open(product_version_file).read().strip()
        except EnvironmentError:
            product_version = "unknown"

        token = self.__get_mcs_token()

        comms.set_user_agent(
            mcsclient.MCSConnection.create_user_agent(
                product_version, token))

    def register(self):
        config = self.__m_config
        assert config is not None
        agent = self.__m_agent
        assert agent is not None
        comms = self.__m_comms
        assert comms is not None

        logger.info("Registering")
        status = agent.get_status_xml()
        token = config.get("MCSToken")
        (endpoint_id, password) = comms.register(token, status)
        config.set("MCSID", endpoint_id)
        config.set("MCSPassword", password)
        config.set("MCS_saved_token", token)
        config.save()

    def run(self):
        config = self.__m_config

        if config.get_default("MCSID") is None:
            logger.critical("Not registered: MCSID is not present")
            return 1
        if config.get_default("MCSPassword") is None:
            logger.critical("Not registered: MCSPassword is not present")
            return 2

        comms = self.__m_comms
        computer = self.__m_computer
        mcs_adapter = self.__m_mcs_adapter

        events = mcsclient.Events.Events()
        events_timer = mcsclient.EventsTimer.EventsTimer(
            config.get_int(
                "EVENTS_REGULATION_DELAY",
                5),
            # Mac appears to use 3
            config.get_int("EVENTS_MAX_REGULATION_DELAY", 60),
            config.get_int("EVENTS_MAX_EVENTS", 20)
        )

        event_receiver = adapters.EventReceiver.EventReceiver(
            PathManager.install_dir())

        def add_event(*event_args):
            events.add_event(*event_args)
            events_timer.event_added()
            logger.debug(
                "Next event update in %.2f s",
                events_timer.relative_time())

        def status_updated(reason=None):
            logger.debug(
                "Checking for status update due to %s",
                reason or "unknown reason")

            self.__m_status_timer.status_updated()
            logger.debug(
                "Next status update in %.2f s",
                self.__m_status_timer.relative_time())

        # setup signal handler before connecting
        utils.SignalHandler.setup_signal_handler()

        # setup a directory watcher for events and statuses
        directory_watcher = utils.DirectoryWatcher.DirectoryWatcher()

        directory_watcher.add_watch(
            PathManager.event_dir(),
            patterns=["*.xml"],
            ignore_delete=True)
        directory_watcher.add_watch(
            PathManager.status_dir(),
            patterns=["*.xml"])
        notify_pipe_file_descriptor = directory_watcher.notify_pipe_file_descriptor

        last_commands = 0

        running = True
        reregister = False
        error_count = 0

        # see if registerCentral.py --reregister has been called
        if config.get_default("MCSID") == "reregister":
            reregister = True

        try:
            while running:
                timeout = self.__m_command_check_interval.get()
                before_time = time.time()

                try:
                    if comms is None:
                        self.startup()
                        comms = self.__m_comms

                    if reregister:
                        logger.info("Re-registering with MCS")
                        self.register()
                        reregister = False
                        error_count = 0
                        # If re-registering due to a de-dupe from Central,
                        # clear cache and re-send status.
                        computer.clear_cache()
                        status_updated(reason="reregistration")

                    # Check for any new app_ids 'for newly installed plugins'
                    added_apps, removed_apps = self.__plugin_registry.added_and_removed_app_ids()
                    if added_apps or removed_apps:
                        if added_apps:
                            logger.info(
                                "New AppIds found to register for: " +
                                ' ,'.join(added_apps))
                        if removed_apps:
                            logger.info(
                                "AppIds not supported anymore: " +
                                ' ,'.join(removed_apps))
                            # Not removing adapters if plugin uninstalled -
                            # this will cause Central to delete commands
                        for app in added_apps:
                            self.__m_computer.add_adapter(
                                adapters.GenericAdapter.GenericAdapter(
                                    app, PathManager.install_dir()))
                        app_ids = [app for app in self.__m_computer.get_app_ids() if app not in [
                            'APPSPROXY', 'AGENT']]
                        logger.info(
                            "Reconfiguring the APPSPROXY to handle: " +
                            ' '.join(app_ids))
                        self.__m_computer.remove_adapter_by_app_id('APPSPROXY')
                        self.__m_computer.add_adapter(
                            adapters.AppProxyAdapter.AppProxyAdapter(app_ids))

                    if time.time() > last_commands + self.__m_command_check_interval.get():
                        logger.debug("Checking for commands")
                        commands = comms.query_commands(computer.get_app_ids())
                        last_commands = time.time()

                        mcs_token_before_commands = self.__get_mcs_token()

                        if computer.run_commands(
                                commands):  # To run any pending commands as well
                            status_updated(reason="applying commands")
                            mcs_token_after_commands = self.__get_mcs_token()
                            if mcs_token_before_commands != mcs_token_after_commands:
                                self.__update_user_agent()

                        if len(commands) > 0:
                            logger.debug("Got commands; resetting interval")
                            self.__m_command_check_interval.set()
                        else:
                            logger.debug("No commands; increasing interval")
                            self.__m_command_check_interval.increment()
                        error_count = 0

                        logger.debug(
                            "Next command check in %.2f s",
                            self.__m_command_check_interval.get())

                    timeout = self.__m_command_check_interval.get() - (time.time() - last_commands)

                    # Check to see if any adapters have new status
                    if computer.has_status_changed() or mcs_adapter.has_new_status():
                        status_updated(
                            reason="adapter reporting status change")

                    # get all pending events
                    for app_id, event_time, event in event_receiver.receive():
                        logger.info("queuing event for %s", app_id)
                        add_event(app_id, event, utils.Timestamp.timestamp(
                            event_time), 10000, utils.IdManager.id())

                    # send status
                    if error_count > 0:
                        pass  # Not sending status while in error state
                    elif self.__m_status_timer.send_status():
                        status_event = mcsclient.StatusEvent.StatusEvent()
                        changed = computer.fill_status_event(status_event)
                        if changed:
                            logger.debug("Sending status")
                            try:
                                comms.send_status_event(status_event)
                                self.__m_status_timer.status_sent()
                            except Exception:
                                self.__m_status_timer.error_sending_status()
                                raise
                        else:
                            logger.debug(
                                "Not sending status as nothing changed")
                            # Don't actually need to send status
                            self.__m_status_timer.status_sent()

                        logger.debug(
                            "Next status update in %.2f s",
                            self.__m_status_timer.relative_time())

                    # send events
                    if error_count > 0:
                        pass  # Not sending events while in error state
                    elif events_timer.send_events():
                        logger.debug("Sending events")
                        try:
                            comms.send_events(events)
                            events_timer.events_sent()
                            events.reset()
                        except Exception:
                            events_timer.error_sending_events()
                            raise

                except socket.error:
                    logger.exception("Got socket error")
                    error_count += 1
                    self.__m_command_check_interval.set_on_error(error_count)
                except mcsclient.MCSConnection.MCSHttpUnauthorizedException as exception:
                    logger.warning("Lost authentication with server")
                    header = exception.headers().get(
                        "www-authenticate", None)  # HTTP headers are case-insensitive
                    if header == 'Basic realm="register"':
                        reregister = True

                    else:
                        logger.error(
                            "Received Unauthenticated without register header=%s",
                            str(header))

                    error_count += 1
                    self.__m_command_check_interval.set_on_error(
                        error_count, transient=False)
                except mcsclient.MCSConnection.MCSHttpException as exception:
                    logger.exception("Got http error from MCS")
                    error_count += 1
                    transient = True
                    if exception.errorCode() == 400:
                        # From MCS spec section 12 - HTTP Bad Request is
                        # semi-permanent
                        transient = False

                    self.__m_command_check_interval.set_on_error(
                        error_count, transient)
                except mcsclient.MCSException.MCSConnectionFailedException:
                    # Already logged from mcsclient
                    #~ logger.exception("Got connection failed exception")
                    error_count += 1
                    self.__m_command_check_interval.set_on_error(error_count)
                except httplib.BadStatusLine as exception:
                    after_time = time.time()
                    bad_status_line_delay = after_time - before_time
                    if bad_status_line_delay < 1.0:
                        logger.debug(
                            "HTTPS connection closed (httplib.BadStatusLine %s) (took %f seconds)",
                            str(exception),
                            bad_status_line_delay,
                            exc_info=False)
                    else:
                        logger.error(
                            "HTTPS connection closed (httplib.BadStatusLine %s) (took %f seconds)",
                            str(exception),
                            bad_status_line_delay,
                            exc_info=False)

                    timeout = 10
                    error_count += 1
                    self.__m_command_check_interval.set_on_error(error_count)

                if error_count == 0 and not reregister:
                    # Only count the timers for events and status if we don't
                    # have any errors
                    timeout = min(
                        self.__m_status_timer.relative_time(), timeout)
                    timeout = min(events_timer.relative_time(), timeout)

                # Avoid busy looping and negative timeouts
                timeout = max(0.5, timeout)

                try:
                    before = time.time()
                    ready_to_read, ready_to_write, in_error = \
                        select.select(
                            [utils.SignalHandler.sig_term_pipe[0], notify_pipe_file_descriptor],
                            [],
                            [],
                            timeout)
                    after = time.time()
                except select.error as exception:
                    if exception.args[0] == errno.EINTR:
                        logger.debug("Got EINTR")
                        continue
                    else:
                        raise

                if utils.SignalHandler.sig_term_pipe[0] in ready_to_read:
                    logger.info("Exiting MCS")
                    running = False
                    break
                elif notify_pipe_file_descriptor in ready_to_read:
                    logger.debug("Got directory watch notification")
                    # flush the pipe
                    while True:
                        try:
                            if os.read(
                                    notify_pipe_file_descriptor,
                                    1024) is None:
                                break
                        except OSError as err:
                            if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                                break
                            else:
                                raise
                elif (after - before) < timeout:
                    logger.debug(
                        "select exited with no event after=%f before=%f delta=%f timeout=%f",
                        after,
                        before,
                        after - before,
                        timeout)

        finally:
            if self.__m_comms is not None:
                self.__m_comms.close()
                self.__m_comms = None

        return 0
