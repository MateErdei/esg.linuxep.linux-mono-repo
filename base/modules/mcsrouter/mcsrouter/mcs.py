#!/usr/bin/env python3
# Copyright (C) 2017-2019 Sophos Plc, Oxford, England.
# All rights reserved.
"""
MCS Module
"""



import errno
import http.client
import logging
import os
import random
import select
import socket
import time

from . import computer
from .adapters import agent_adapter
from .adapters import app_proxy_adapter
from .adapters import event_receiver
from .adapters import response_receiver
from .adapters import generic_adapter
from .adapters import livequery_adapter
from .adapters import mcs_adapter
from .mcsclient import config_exception
from .mcsclient import events as events_module
from .mcsclient import events_timer as events_timer_module
from .mcsclient import responses as responses_module
from .mcsclient import mcs_commands
from .mcsclient import mcs_connection
from .mcsclient import mcs_exception
from .mcsclient import status_event as status_event_module
from .mcsclient import status_timer
from .utils import config as config_module
from .utils import directory_watcher as directory_watcher_module
from .utils import id_manager
from .utils import path_manager
from .utils import plugin_registry
from .utils import signal_handler
from .utils import timestamp
from .utils.get_ids import get_gid, get_uid

LOGGER = logging.getLogger(__name__)


class CommandCheckInterval:
    """
    CommandCheckInterval
    """
    # seconds*minutes
    DEFAULT_MAX_POLLING_INTERVAL = 60*30
    DEFAULT_MIN_POLLING_INTERVAL = 20


    def __init__(self, config):
        """
        __init__
        """
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
        """
        get
        """
        return self.__m_command_check_interval

    def __get_minimum(self):
        """
        __get_minimum
        """
        interval_min = self.__m_config.get_int(
            "COMMAND_CHECK_INTERVAL_MINIMUM",
            self.DEFAULT_MIN_POLLING_INTERVAL)
        if self.__m_command_check_interval_minimum != interval_min:
            self.__m_command_check_interval_minimum = interval_min
            LOGGER.debug("COMMAND_CHECK_INTERVAL_MINIMUM=%d", interval_min)
        return interval_min

    def __get_maximum(self):
        """
        __get_maximum
        """
        interval_max = self.__m_config.get_int(
            "COMMAND_CHECK_INTERVAL_MAXIMUM",
            self.DEFAULT_MAX_POLLING_INTERVAL)
        if self.__m_command_check_interval_maximum != interval_max:
            self.__m_command_check_interval_maximum = interval_max
            LOGGER.debug("COMMAND_CHECK_INTERVAL_MAXIMUM=%d", interval_max)
        return interval_max

    def set(self, val=None):
        """
        set
        """
        if val is None:
            val = self.__m_command_check_base_retry_delay
        val = max(val, self.__get_minimum())
        val = min(val, self.__get_maximum())
        self.__m_command_check_interval = val
        return self.__m_command_check_interval

    def increment(self, val=None):
        """
        increment
        """
        if val is None:
            val = self.__m_command_check_base_retry_delay
        self.set(self.__m_command_check_interval + val)

    def set_on_error(self, error_count, transient=True):
        """
        set_on_error
        """
        max_retry_number = self.__m_command_check_maximum_retry_number
        number_of_retries = min(error_count -1, max_retry_number)

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
        delay_upper_bound = base_retry_delay * (2 ** (number_of_retries))
        delay_lower_bound = base_retry_delay * (2 ** (number_of_retries -1))
        retry_delay = random.uniform(
            delay_lower_bound, delay_upper_bound)
        self.set(retry_delay)
        LOGGER.info("[backoff] waiting up to %fs after %d failures",
                    delay_upper_bound, error_count)
        if delay_upper_bound > 3600:
            LOGGER.error("Failed to connect with Central for more than an hour")


class MCS:
    """
    MCS
    """
    # pylint: disable=too-many-instance-attributes

    def __init__(self, install_dir, config=config_module.Config()):
        """
        __init__
        """
        path_manager.INST = install_dir

        self.__m_comms = None

        fixed_config = config_module.Config(
            filename=path_manager.root_config(),
            parent_config=config,
            mode=0o640,
            user_id=get_uid("root"),
            group_id=get_gid("sophos-spl-group")
        )

        fixed_config.set_default(
            "MCSURL",
            "https://mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep")

        self.__m_policy_config = config_module.Config(
            filename=path_manager.mcs_policy_config(),
            parent_config=fixed_config,
            mode=0o600,
            user_id=get_uid("sophos-spl-user"),
            group_id=get_gid("sophos-spl-group")
        )
        self.__m_config = config_module.Config(
            filename=path_manager.sophosspl_config(),
            parent_config=self.__m_policy_config,
            mode=0o600,
            user_id=get_uid("sophos-spl-user"),
            group_id=get_gid("sophos-spl-group")
        )
        config = self.__m_config

        status_latency = self.__m_config.get_int("STATUS_LATENCY", 30)
        LOGGER.debug("STATUS_LATENCY=%d", status_latency)
        self.__m_status_timer = status_timer.StatusTimer(
            status_latency,
            self.__m_config.get_int("STATUS_INTERVAL", 60 * 60 * 24)
        )

        # Configure fragmented policy cache directory
        mcs_commands.FragmentedPolicyCommand.FRAGMENT_CACHE_DIR = \
            path_manager.fragmented_policies_dir()

        # Create computer
        self.__m_computer = computer.Computer()
        self.__m_mcs_adapter = mcs_adapter.MCSAdapter(
            install_dir, self.__m_policy_config, self.__m_config)

        self.__m_computer.add_adapter(self.__m_mcs_adapter)

        #~ apps = [ "ALC", "SAV", "HBT", "NTP", "SHS", "SDU", "UC", "MR" ]
        self.__plugin_registry = plugin_registry.PluginRegistry(
            install_dir)

        self.check_registry_and_update_apps()

        self.__m_agent = agent_adapter.AgentAdapter()
        self.__m_computer.add_adapter(self.__m_agent)

        # TODO: reload on MCS policy change (and any other configurables? Event
        # & status regulation?)
        self.__m_command_check_interval = CommandCheckInterval(config)

    def startup(self):
        """
        Connect and register if required
        """
        config = self.__m_config

        comms = mcs_connection.MCSConnection(
            config,
            install_dir=path_manager.install_dir()
        )

        self.__update_user_agent(comms)

        # Check capabilities
        capabilities = comms.capabilities()
        LOGGER.info("Capabilities=%s", capabilities)
        # TODO parse and verify if we need something beyond baseline

        self.__m_comms = comms

    def __get_mcs_token(self):
        """
        __get_mcs_token
        """
        return self.__m_config.get_default("MCSToken", "unknown")

    def __update_user_agent(self, comms=None):
        """
        __update_user_agent
        """
        if comms is None:
            comms = self.__m_comms
            if comms is None:
                return

        product_version_file = os.path.join(
            path_manager.install_dir(), "engine", "savVersion")
        try:
            product_version = open(product_version_file).read().strip()
        except EnvironmentError:
            product_version = "unknown"

        token = self.__get_mcs_token()

        comms.set_user_agent(
            mcs_connection.create_user_agent(
                product_version, token))

    def register(self):
        """
        register
        """
        config = self.__m_config
        assert config is not None
        agent = self.__m_agent
        assert agent is not None
        comms = self.__m_comms
        assert comms is not None

        LOGGER.info("Registering")
        status = agent.get_status_xml()
        token = config.get("MCSToken")
        (endpoint_id, password) = comms.register(token, status)
        config.set("MCSID", endpoint_id)
        config.set("MCSPassword", password)
        config.set("MCS_saved_token", token)
        config.save()

    def _get_directory_watcher(self):
        try:
            directory_watcher = directory_watcher_module.DirectoryWatcher()

            directory_watcher.add_watch(
                path_manager.event_dir(),
                patterns=["*.xml"],
                ignore_delete=True)
            directory_watcher.add_watch(
                path_manager.status_dir(),
                patterns=["*.xml"])
            directory_watcher.add_watch(
                path_manager.response_dir(),
                patterns=["*.json"],
                ignore_delete=True)

            return directory_watcher
        except Exception as ex:
            raise config_exception.ConfigException(
                config_exception.composeMessage("Managing Communication Paths", str(ex))
            )

    def check_registry_and_update_apps(self):
        added_apps, removed_apps = self.__plugin_registry.added_and_removed_app_ids()

        if not added_apps and not removed_apps:
            return
        apps_proxy = 'APPSPROXY'
        if apps_proxy in self.__m_computer.get_app_ids():
            self.__m_computer.remove_adapter_by_app_id(apps_proxy)

        # Check for any new app_ids 'for newly installed plugins'
        if added_apps:
            LOGGER.info(
                "New AppIds found to register for: " +
                ', '.join(added_apps))
        if removed_apps:
            LOGGER.info(
                "AppIds not supported anymore: " +
                ' ,'.join(removed_apps))
            # Not removing adapters if plugin uninstalled -
            # this will cause Central to delete commands
        for app in added_apps:
            if app == "LiveQuery":
                LOGGER.debug( "Add LiveQuery adapter for app {}".format(app))
                self.__m_computer.add_adapter(
                    livequery_adapter.LiveQueryAdapter(app, path_manager.install_dir())
                )
            else:
                LOGGER.debug( "Add Generic adapter for app {}".format(app))
                self.__m_computer.add_adapter(
                    generic_adapter.GenericAdapter(
                        app, path_manager.install_dir()))

        # AppsProxy needs to report all AppIds apart from AGENT and APPSPROXY
        app_ids = [app for app in self.__m_computer.get_app_ids() if app not in [
            apps_proxy, 'AGENT']]
        LOGGER.info(
            "Reconfiguring the APPSPROXY to handle: " +
            ' '.join(app_ids))
        self.__m_computer.add_adapter(
            app_proxy_adapter.AppProxyAdapter(app_ids))


    def run(self):
        """
        run
        """
        # pylint: disable=too-many-locals, too-many-branches, too-many-statements

        config = self.__m_config

        if config.get_default("MCSID") is None:
            LOGGER.critical("Not registered: MCSID is not present")
            return 1
        if config.get_default("MCSPassword") is None:
            LOGGER.critical("Not registered: MCSPassword is not present")
            return 2

        comms = self.__m_comms

        events = events_module.Events()
        events_timer = events_timer_module.EventsTimer(
            config.get_int(
                "EVENTS_REGULATION_DELAY",
                5),
            # Mac appears to use 3
            config.get_int("EVENTS_MAX_REGULATION_DELAY", 60),
            config.get_int("EVENTS_MAX_EVENTS", 20)
        )

        def add_event(*event_args):
            """
            add_event
            """
            events.add_event(*event_args)
            events_timer.event_added()
            LOGGER.debug(
                "Next event update in %.2f s",
                events_timer.relative_time())

        responses = responses_module.Responses()

        def add_response(*response_args):
            """
            add_response
            """
            responses.add_response(*response_args)

        def status_updated(reason=None):
            """
            status_updated
            """
            LOGGER.debug(
                "Checking for status update due to %s",
                reason or "unknown reason")

            self.__m_status_timer.status_updated()
            LOGGER.debug(
                "Next status update in %.2f s",
                self.__m_status_timer.relative_time())

        # setup signal handler before connecting
        signal_handler.setup_signal_handler()

        # setup a directory watcher for events and statuses
        directory_watcher = self._get_directory_watcher()
        notify_pipe_file_descriptor = directory_watcher.notify_pipe_file_descriptor

        last_commands = 0

        running = True
        reregister = False
        error_count = 0

        # see if register_central.py --reregister has been called
        if config.get_default("MCSID") == "reregister":
            reregister = True

        # pylint: disable=too-many-nested-blocks
        try:
            while running:
                timeout_compensation = 0

                before_time = time.time()

                try:
                    if comms is None:
                        self.startup()
                        comms = self.__m_comms

                    if reregister:
                        LOGGER.info("Re-registering with MCS")
                        self.register()
                        LOGGER.info("Endpoint re-registered")
                        reregister = False
                        error_count = 0
                        # If re-registering due to a de-dupe from Central,
                        # clear cache and re-send status.
                        self.__m_computer.clear_cache()
                        status_updated(reason="reregistration")

                    self.check_registry_and_update_apps()

                    if time.time() > last_commands + self.__m_command_check_interval.get():
                        appids = self.__m_computer.get_app_ids()
                        LOGGER.debug("Checking for commands for %s", str(appids))
                        commands = comms.query_commands(appids)
                        last_commands = time.time()

                        mcs_token_before_commands = self.__get_mcs_token()

                        if self.__m_computer.run_commands(
                                commands):  # To run any pending commands as well
                            status_updated(reason="applying commands")
                            mcs_token_after_commands = self.__get_mcs_token()
                            if mcs_token_before_commands != mcs_token_after_commands:
                                self.__update_user_agent()

                        if commands:
                            LOGGER.debug("Got commands; resetting interval")
                            self.__m_command_check_interval.set()
                        else:
                            LOGGER.debug("No commands; increasing interval")
                            self.__m_command_check_interval.increment()
                        error_count = 0

                        LOGGER.debug(
                            "Next command check in %.2f s",
                            self.__m_command_check_interval.get())

                    timeout_compensation = (time.time() - last_commands)

                    # Check to see if any adapters have new status
                    if self.__m_computer.has_status_changed() \
                            or self.__m_mcs_adapter.has_new_status():
                        status_updated(
                            reason="adapter reporting status change")

                    # get all pending events
                    for app_id, event_time, event in event_receiver.receive():
                        LOGGER.info("queuing event for %s", app_id)
                        add_event(app_id, event, timestamp.timestamp(
                            event_time), 10000, id_manager.generate_id())

                    # get all pending responses
                    for file_path, app_id, correlation_id, response_time, response_body in response_receiver.receive():
                        LOGGER.info("queuing response for %s", app_id)
                        add_response(file_path, app_id, correlation_id, timestamp.timestamp(
                            response_time), response_body)

                    # send status
                    if error_count > 0:
                        pass  # Not sending status while in error state
                    elif self.__m_status_timer.send_status():
                        status_event = status_event_module.StatusEvent()
                        changed = self.__m_computer.fill_status_event(status_event)
                        if changed:
                            LOGGER.debug("Sending status")
                            try:
                                comms.send_status_event(status_event)
                                self.__m_status_timer.status_sent()
                            except Exception:
                                self.__m_status_timer.error_sending_status()
                                raise
                        else:
                            LOGGER.debug(
                                "Not sending status as nothing changed")
                            # Don't actually need to send status
                            self.__m_status_timer.status_sent()

                        LOGGER.debug(
                            "Next status update in %.2f s",
                            self.__m_status_timer.relative_time())

                    # send events
                    if error_count > 0:
                        pass  # Not sending events while in error state
                    elif events_timer.send_events():
                        LOGGER.debug("Sending events")
                        try:
                            comms.send_events(events)
                            events_timer.events_sent()
                            events.reset()
                        except Exception:
                            events_timer.error_sending_events()
                            raise

                    # send responses
                    if error_count > 0:
                        pass  # Not sending responses while in error state
                    elif responses.has_responses():
                        LOGGER.debug("Sending responses")
                        try:
                            comms.send_responses(responses.get_responses())
                            responses.reset()
                        except Exception as exception:
                            LOGGER.error("Failed to send responses: {}".format(str(exception)))

                except socket.error:
                    LOGGER.warning("Got socket error")
                    error_count += 1
                    self.__m_command_check_interval.set_on_error(error_count)
                except mcs_connection.MCSHttpUnauthorizedException as exception:
                    LOGGER.warning("Lost authentication with server")
                    header = exception.headers().get(
                        "www-authenticate", None)  # HTTP headers are case-insensitive
                    if header == 'Basic realm="register"':
                        reregister = True

                    else:
                        LOGGER.error(
                            "Received Unauthenticated without register header=%s",
                            str(header))

                    error_count += 1
                    self.__m_command_check_interval.set_on_error(
                        error_count, transient=False)
                except mcs_connection.MCSHttpException as exception:
                    error_count += 1
                    transient = True

                    if exception.error_code() == 503:
                        LOGGER.warning("Endpoint has been temporarily suspended, possibly due to the machine "
                                       "being deleted from the Sophos Central Console. "
                                       "Communication should resume within the hour.")
                        transient = False
                    elif exception.error_code() == 400:
                        # From MCS spec section 12 - HTTP Bad Request is
                        # semi-permanent
                        transient = False
                    else:
                        LOGGER.exception("Got http error from MCS")

                    self.__m_command_check_interval.set_on_error(
                        error_count, transient)
                except (mcs_exception.MCSNetworkException, http.client.NotConnected):
                    # Already logged from mcsclient
                    #~ LOGGER.exception("Got connection failed exception")
                    error_count += 1
                    self.__m_command_check_interval.set_on_error(error_count)
                except http.client.BadStatusLine as exception:
                    after_time = time.time()
                    bad_status_line_delay = after_time - before_time
                    if bad_status_line_delay < 1.0:
                        LOGGER.debug(
                            "HTTPS connection closed (httplib.BadStatusLine %s) (took %f seconds)",
                            str(exception),
                            bad_status_line_delay,
                            exc_info=False)
                    else:
                        LOGGER.error(
                            "HTTPS connection closed (httplib.BadStatusLine %s) (took %f seconds)",
                            str(exception),
                            bad_status_line_delay,
                            exc_info=False)

                    timeout = 10
                    error_count += 1
                    self.__m_command_check_interval.set_on_error(error_count)

                timeout = self.__m_command_check_interval.get()
                if error_count == 0 and not reregister:
                    # Only count the timers for events and status if we don't
                    # have any errors
                    timeout = min(
                        self.__m_status_timer.relative_time(), timeout)
                    timeout = min(events_timer.relative_time(), timeout)
                else:
                    timeout -= timeout_compensation

                # Avoid busy looping and negative timeouts
                timeout = max(0.5, timeout)

                try:
                    before = time.time()
                    # pylint: disable=unused-variable
                    ready_to_read, ready_to_write, in_error = \
                        select.select(
                            [signal_handler.sig_term_pipe[0], notify_pipe_file_descriptor],
                            [],
                            [],
                            timeout)
                    # pylint: enable=unused-variable
                    after = time.time()
                except select.error as exception:
                    if exception.args[0] == errno.EINTR:
                        LOGGER.debug("Got EINTR")
                        continue
                    else:
                        raise

                if signal_handler.sig_term_pipe[0] in ready_to_read:
                    LOGGER.info("Exiting MCS")
                    running = False
                    break
                elif notify_pipe_file_descriptor in ready_to_read:
                    LOGGER.debug("Got directory watch notification")
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
                    LOGGER.debug(
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
