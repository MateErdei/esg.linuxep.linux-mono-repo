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
import shutil
import time
import json
import copy

from . import computer
from . import mcs_push_client
from .adapters import agent_adapter
from .adapters import app_proxy_adapter
from .adapters import event_receiver
from .adapters import response_receiver
from .adapters import datafeed_receiver
from .adapters import generic_adapter
from .adapters import livequery_adapter
from .adapters import liveresponse_adapter
from .adapters import mcs_adapter
from .mcsclient import config_exception
from .mcsclient import events as events_module
from .mcsclient import events_timer as events_timer_module
from .mcsclient import responses as responses_module
from .mcsclient import datafeeds as datafeeds_module
from .mcsclient import mcs_commands
from .mcsclient import mcs_connection
from .mcsclient import mcs_exception
from .mcsclient import status_event as status_event_module
from .mcsclient import status_timer
from .utils import config as config_module
from .utils import directory_watcher as directory_watcher_module
from .utils import id_manager
from .utils import path_manager
from .utils import default_values
from .utils import plugin_registry
from .utils import signal_handler
from .utils import timestamp
from .utils import handle_json
from .utils import flags
from .utils import migration

from .utils.get_ids import get_gid, get_uid

LOGGER = logging.getLogger(__name__)

class DeploymentApiException(RuntimeError):
    def __init__(self, message):
        # Call the base class constructor with the parameters it needs
        super().__init__(message)

class CommandCheckInterval:
    """
    CommandCheckInterval
    """

    DEFAULT_MAX_POLLING_INTERVAL = default_values.get_default_flags_poll()
    DEFAULT_MIN_POLLING_INTERVAL = default_values.get_default_command_poll()

    def __init__(self, config):
        """
        __init__
        """
        self.__m_config = config
        self.__m_command_check_interval = 0
        self.__m_command_check_interval_minimum = 0
        self.__m_command_check_interval_maximum = 0
        self.__m_push_ping_timeout = 0
        self.__m_push_command_check_interval = 0
        self.__use_fallback_polling_interval = False
        self.__m_command_check_maximum_retry_number = 0
        self.__m_command_check_base_retry_delay = 0
        self.__m_command_check_semi_permanent_error_retry_delay = 0

        self.set()
        self.__get_push_ping_timeout()
        self.__get_push_poll_interval()

    def get(self):
        """
        get
        """
        return self.__m_command_check_interval

    def __get_command_polling_delay(self):
        """
        __get_command_polling_delay
        """
        command_polling_delay = self.__m_config.get_int(
            "commandPollingDelay",
            self.DEFAULT_MIN_POLLING_INTERVAL)
        if self.__m_command_check_interval_minimum != command_polling_delay:
            self.__m_command_check_interval_minimum = command_polling_delay
            LOGGER.debug("commandPollingDelay={}".format(str(command_polling_delay)))
        return command_polling_delay

    def __get_flags_polling_interval(self):
        """
        __get_flags_polling_interval
        """
        flags_polling_interval = self.__m_config.get_int(
            "flagsPollingInterval",
            self.DEFAULT_MAX_POLLING_INTERVAL)
        if self.__m_command_check_interval_maximum != flags_polling_interval:
            self.__m_command_check_interval_maximum = flags_polling_interval
            LOGGER.debug("flagsPollingInterval={}".format(str(flags_polling_interval)))
        return flags_polling_interval

    def __get_push_ping_timeout(self):
        push_ping_timeout = self.__m_config.get_int(
            "pushPingTimeout",
            self.__get_command_polling_delay())
        if self.__m_push_ping_timeout != push_ping_timeout:
            self.__m_push_ping_timeout = push_ping_timeout
            LOGGER.debug("pushPingTimeout={}".format(str(push_ping_timeout)))
        return push_ping_timeout

    def __get_push_poll_interval(self):
        push_poll_interval = self.__m_config.get_int(
            "pushFallbackPollInterval",
            self.__get_command_polling_delay())
        if self.__m_push_command_check_interval != push_poll_interval:
            self.__m_push_command_check_interval = push_poll_interval
            LOGGER.debug("pushFallbackPollInterval={}".format(str(push_poll_interval)))
        return push_poll_interval

    def update_retry_info(self):
        self.__m_command_check_maximum_retry_number = self.__m_config.get_int("COMMAND_CHECK_MAXIMUM_RETRY_NUMBER", 10)
        self.__m_command_check_base_retry_delay = self.__m_config.get_int("commandPollingDelay", 20)
        self.__m_command_check_semi_permanent_error_retry_delay = self.__m_command_check_base_retry_delay * 2

    def set(self, val=None):
        """
        set
        """
        if self.__use_fallback_polling_interval:
            self.__m_command_check_interval = self.__get_push_poll_interval()
            LOGGER.info(f"Using pushFallbackPollInterval. Set command poll interval to {self.__m_command_check_interval}")
        else:
            if val is None:
                # this is the minimum we should ever set the command poll to
                val = 5
            val = max(val, self.__get_command_polling_delay())
            val = min(val, self.DEFAULT_MAX_POLLING_INTERVAL)
            self.__m_command_check_interval = val
            LOGGER.info(f"Using commandPollingDelay. Set command poll interval to {val}")

        self.update_retry_info()

        return self.__m_command_check_interval

    def increment(self, val=None):
        """
        increment
        """
        if val is None:
            val = self.__m_command_check_base_retry_delay
        if not self.__use_fallback_polling_interval:
            LOGGER.debug("increasing interval")
            self.__m_command_check_interval += val

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

    def set_use_fallback_polling_interval(self, use_fallback_polling_interval):
        self.__use_fallback_polling_interval = use_fallback_polling_interval

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

        self.__m_root_config = config_module.Config(
            filename=path_manager.root_config(),
            parent_config=config,
            mode=0o640,
            user_id=get_uid("sophos-spl-local"),
            group_id=get_gid("sophos-spl-group")
        )

        self.__m_root_config.set_default(
            "MCSURL",
            "https://mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep")

        self.__m_policy_config = config_module.Config(
            filename=path_manager.mcs_policy_config(),
            parent_config=self.__m_root_config,
            mode=0o600,
            user_id=get_uid("sophos-spl-local"),
            group_id=get_gid("sophos-spl-group")
        )
        self.__m_config = config_module.Config(
            filename=path_manager.sophosspl_config(),
            parent_config=self.__m_policy_config,
            mode=0o640,
            user_id=get_uid("sophos-spl-local"),
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
        self.__m_app_proxy_adapter = app_proxy_adapter.AppProxyAdapter(self.__m_computer.get_app_ids())
        self.__m_computer.add_adapter(self.__m_app_proxy_adapter)
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

        product_version = "unknown"
        try:
            product_version_tmp = agent_adapter.get_version()
            if product_version_tmp != 0:
                product_version = product_version_tmp
        except Exception as ex:
            LOGGER.error(f"Could not get base version while composing user agent string: {str(ex)}")
        token = self.__get_mcs_token()

        comms.set_user_agent(
            mcs_connection.create_user_agent(
                product_version, token))

    def process_deployment_response_body(self, body):
        try:
            response_json = json.loads(body)

            deployment_registration_token = response_json.get("registrationToken", None)
            LOGGER.debug(f"Deployment api response: {body}")
        except json.decoder.JSONDecodeError as exception:
            raise DeploymentApiException(f"Failed to process response body: {body}, reason: {exception}")

        if not deployment_registration_token:
            raise DeploymentApiException(f"Could not find registration token in response body: {body}")

        # We don't want to abandon processing the response if this check fails
        try:
            products = response_json.get("products", None)
            if products:
                for product in products:
                    if not product["supported"]:
                        LOGGER.warning(f"requested product: {product['product']}, is not supported for this licence and platform. Reasons: {product['reasons']}. "
                                        "The product will not be assigned in central")
        except Exception as exception:
            LOGGER.debug(f"Check for unsupported products failed: {exception}")

        return deployment_registration_token

    def register(self, options=None):
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
        status = agent.get_status_xml(options)
        token = config.get("MCSToken")

        # pre-registration request to the deployment api
        if options and options.customer_token and options.selected_products:
            try:
                response_from_deployment_api = comms.deployment_check(options.customer_token, status)
                token_from_deployment_api = self.process_deployment_response_body(response_from_deployment_api)
                if token_from_deployment_api:
                    token = token_from_deployment_api
            except (DeploymentApiException, mcs_connection.MCSHttpException) as exception:
                LOGGER.error(f"Deployment API call failed: {exception}, Continuing registration with default registration token: {token}")

        (endpoint_id, password) = comms.register(token, status)
        self.__m_computer.clear_cache()
        config.set("MCSID", endpoint_id)
        config.set("MCSPassword", password)
        config.set("MCS_saved_token", token)
        config.save()
        ## we want to resend the ALC update event on a re registration
        self.resend_old_event()

    def resend_old_event(self):
        for filename in os.listdir(path_manager.event_cache_dir()):
            file_path = os.path.join(path_manager.event_cache_dir(), filename)
            try:
                if filename.startswith('ALC'):
                    if os.path.isfile(file_path):
                        shutil.move(file_path, os.path.join(path_manager.event_dir(), filename))
            except Exception as ex:
                LOGGER.error('Failed to move old event file back {} due to error {}'.format(file_path, str(ex)))

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
            elif app == "LiveTerminal":
                LOGGER.debug( "Add LiveResponse adapter for app {}".format(app))
                self.__m_computer.add_adapter(
                    liveresponse_adapter.LiveResponseAdapter(app, path_manager.install_dir())
                )
            elif app == "MCS":
                LOGGER.debug("Do nothing here as we don't want to override MCS adapter with Generic adapter")
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
        self.__m_app_proxy_adapter = app_proxy_adapter.AppProxyAdapter(app_ids)
        self.__m_computer.add_adapter(self.__m_app_proxy_adapter)

    def stop_push_client(self, push_client):
        if push_client:
            LOGGER.info("Requesting MCS Push client to stop")
            push_client.stop_service()
        self.__m_command_check_interval.set_use_fallback_polling_interval(False)

    def on_error(self, push_client, error_count=0, transient=True):
        # need to call set_use_fallback_polling_interval via stop_push_client before set_on_error
        self.stop_push_client(push_client)
        self.__m_command_check_interval.set_on_error(error_count, transient)

    def get_flags(self, last_time_checked):
        flags_polling = self.__m_config.get_int("flagsPollingInterval", default_values.get_default_flags_poll())
        if (time.time() > last_time_checked + flags_polling) \
                or not os.path.isfile(path_manager.mcs_flags_file()):
            LOGGER.info("Checking for updates to mcs flags")
            mcs_flags_content = self.__m_comms.get_flags()
            if mcs_flags_content:
                handle_json.write_mcs_flags(mcs_flags_content)
            last_time_checked = time.time()
        return last_time_checked

    def should_generate_new_jwt_token(self):
        if self.__m_comms is None:
            LOGGER.info("New JWT should be requested because MCS comms is not set")
            return True

        if self.__m_comms.m_jwt_token is None:
            LOGGER.info("New JWT should be requested because MCS does not have a token")
            return True

        if self.__m_comms.m_device_id is None:
            LOGGER.info("New JWT should be requested because MCS does not have a device ID")
            return True

        if self.__m_comms.m_tenant_id is None:
            LOGGER.info("New JWT should be requested because MCS does not have a tenant ID")
            return True

        # Get a new JWT if device ID from policy does not match the one in the current config.
        if self.__m_config.get_default("policy_device_id", "") != self.__m_comms.m_device_id:
            LOGGER.info(f"New JWT should be requested because device ID in policy is different to the current device ID, policy: {self.__m_config.get_default('policy_device_id', '')}, config: {self.__m_comms.m_device_id} ")
            return True

        # We want to request a new one when there is less than 10 mins left until the current one expires
        if time.time() > (self.__m_comms.m_jwt_expiration_timestamp - 600):
            LOGGER.info("New JWT should be requested because token will expire within the next 10 mins")
            return True

        return False

    def token_and_url_are_set(self):
        if os.path.isfile(path_manager.root_config()):
            token = self.__m_policy_config.get("MCSToken")
            url = self.__m_policy_config.get("MCSURL")
            if token and url:
                return True
        return False

    def status_updated(self, reason=None):
        """
        status_updated, indicate to MCS that the EP status has changed.
        """
        LOGGER.debug("Checking for status update due to %s", reason or "unknown reason")
        self.__m_status_timer.status_updated()
        LOGGER.debug("Next status update in %.2f s", self.__m_status_timer.relative_time())

    def migration_file_purge(self):
        def wrapped_remove(file_path_to_delete: str):
            try:
                if os.path.isfile(file_path_to_delete):
                    os.remove(file_path_to_delete)
            except (FileNotFoundError, PermissionError) as exception:
                LOGGER.error('Failed to delete file {} due to error {}'.format(file_path_to_delete, str(exception)))

        def purge_files_in_dir(dir_path: str):
            for file in os.listdir(dir_path):
                file_path_to_delete = os.path.join(dir_path, file)
                wrapped_remove(file_path_to_delete)

        # Purge all events
        purge_files_in_dir(os.path.join(path_manager.event_dir()))

        # Purge all actions
        purge_files_in_dir(os.path.join(path_manager.action_dir()))

        # Purge all policies
        purge_files_in_dir(os.path.join(path_manager.policy_dir()))

        # We want to make sure all statuses are re-sent after
        self.status_updated(reason="migration")

        # Purge all datafeed files
        purge_files_in_dir(os.path.join(path_manager.datafeed_dir()))

        # Remove config based on MCS policy
        wrapped_remove(path_manager.mcs_policy_config())
        wrapped_remove(path_manager.sophosspl_config())

        # Remove current proxy file
        wrapped_remove(path_manager.mcs_current_proxy())

        # Remove flags
        wrapped_remove(path_manager.mcs_flags_file())
        wrapped_remove(path_manager.combined_flags_file())
        # Don't remove warehouse flags file, it contains SDDS3 flag and we do not want updating to revert to SDDS2

    def attempt_migration(self, old_comms, live_config, root_config, push_client) -> bool:
        LOGGER.info("Attempting Central migration")
        migration_data = migration.Migrate()
        migrate_comms = None
        try:
            migration_data.read_migrate_action(self.__m_app_proxy_adapter.get_migrate_action())
            migrate_config = copy.deepcopy(live_config)
            migrate_comms = mcs_connection.MCSConnection(
                migrate_config,
                install_dir=path_manager.install_dir(),
                migrate_mode=True
            )

            # Set new comms object to use the MCS URL sent down in migrate action
            migrate_config.set("MCSURL", migration_data.get_migrate_url())

            # Configure (safely) to talk to new Central
            response = migrate_comms.send_migration_request(
                migration_data.get_command_path(),
                migration_data.get_token(),
                self.__m_agent.get_status_xml(),
                old_comms.m_device_id,
                old_comms.m_tenant_id
            )

            # Check response before updating comms mode
            endpoint_id, device_id, tenant_id, password = migration_data.extract_values(response)

            # Ensure that the old comms does not read or write the existing config files
            old_comms.set_migrate_mode(True)

            self.migration_file_purge()

            self.__m_computer.clear_cache()
            migrate_comms.set_migrate_mode(False)
            migrate_config.set("MCSID", endpoint_id)
            migrate_config.set("MCSPassword", password)
            migrate_config.set("tenant_id", tenant_id)
            migrate_config.set("device_id", device_id)
            migrate_config.save()

            # Remove old MCS token from root config
            # Rely on the one that is sent via policy and stored in mcs policy config
            # Update root config URL to latest known good one
            root_config.remove("MCSToken")
            root_config.set("MCSURL", migration_data.get_migrate_url())
            root_config.save_non_atomic()

            self.stop_push_client(push_client)

            try:
                old_comms.send_migration_event(str(migration_data.create_success_response_body()))
                LOGGER.info("Sent 'Migration succeeded' event to previous Central account")
            except Exception as exception:
                LOGGER.warning("'Migration succeeded' event failed to send: {}".format(exception))

            old_comms.close()

            # we want to resend the ALC update event on migration
            self.resend_old_event()

            self.__m_config = migrate_config
            migration_success = True

        except Exception as exception:
            migration_success = False
            LOGGER.error("Migration request failed: {}".format(exception))
            self.__m_comms.send_migration_event(str(migration_data.create_failed_response_body(exception)))
            if migrate_comms:
                migrate_comms.close()
        finally:
            self.__m_app_proxy_adapter.clear_migrate_action()

        if migration_success:
            LOGGER.info("Successfully migrated Sophos Central account")
        return migration_success

    def run(self):
        """
        run
        """
        # pylint: disable=too-many-locals, too-many-branches, too-many-statements

        config = self.__m_config
        config_error = False
        if not config.get_default("MCSID"):
            LOGGER.critical("Not registered: MCSID is not present")
            if self.token_and_url_are_set():
                LOGGER.debug("MCS root config file exists so we will re-register with the stored token and url")
                config_error = True
            else:
                return 1
        if not config.get_default("MCSPassword"):
            LOGGER.critical("Not registered: MCSPassword is not present")
            if self.token_and_url_are_set():
                LOGGER.debug("MCS root config file exists so we will re-register with the stored token and url")
                config_error = True
            else:
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

        # Container to hold Datafeed objects, new datafeeds can be added to the product by including them here.
        all_datafeeds = [
            datafeeds_module.Datafeeds("scheduled_query")
        ]

        def gather_datafeed_files():
            if datafeed_receiver.pending_files():
                LOGGER.debug("Found datafeed result files")
                for result_file_path, datafeed_id, datafeed_timestamp, datafeed_body in datafeed_receiver.receive():
                    for datafeed in all_datafeeds:
                        if datafeed.get_feed_id() == datafeed_id:
                            if datafeed.add_datafeed_result(result_file_path, datafeed_id, datafeed_timestamp, datafeed_body):
                                LOGGER.debug(f"Queuing datafeed result for: {datafeed_id}, with timestamp: {datafeed_timestamp}")
                            else:
                                LOGGER.debug(f"Already queued datafeed result for: {datafeed_id}, with timestamp: {datafeed_timestamp}")
                        else:
                            LOGGER.warning(f"There is no datafeed handler setup for the datafeed ID: {datafeed_id}")
            else:
                LOGGER.debug("No datafeed result files")

        def send_datafeed_files():
            datafeeds_module.Datafeeds.send_datafeed_files(all_datafeeds, comms)

        def should_we_migrate() -> bool:
            return self.__m_app_proxy_adapter.get_migrate_action() is not None

        # setup a directory watcher for events and statuses
        directory_watcher = self._get_directory_watcher()
        notify_pipe_file_descriptor = directory_watcher.notify_pipe_file_descriptor

        push_client = mcs_push_client.MCSPushClient()
        push_notification_pipe_file_descriptor = push_client.notify_activity_pipe()

        last_command_time_check = 0
        last_flag_time_check = 0

        running = True
        reregister = False
        error_count = 0

        # see if register_central.py --reregister has been called or if we have lost either the MCSID or MCSpassword
        if config.get_default("MCSID") == "reregister" or config_error:
            reregister = True

        # pylint: disable=too-many-nested-blocks
        try:
            while running:
                LOGGER.debug("Re-entered main loop")
                timeout_compensation = 0

                before_time = time.time()

                try:
                    if comms is None:
                        self.startup()
                        comms = self.__m_comms

                    if should_we_migrate():
                        if self.attempt_migration(comms, config, self.__m_root_config, push_client):
                            running = False
                            break

                    if reregister:
                        LOGGER.info("Re-registering with MCS")
                        self.register()
                        LOGGER.info("Endpoint re-registered")
                        reregister = False
                        error_count = 0
                        # If re-registering due to a de-dupe from Central,
                        # clear cache and re-send status.
                        self.status_updated(reason="reregistration")

                    self.check_registry_and_update_apps()
                    push_commands = push_client.pending_commands()
                    commands_to_run = []
                    if push_commands:
                        LOGGER.info("Received command from Push Server")

                    force_mcs_server_command_processing = False

                    for push_command in push_commands:
                        if push_command.msg_type == mcs_push_client.MsgType.Error:
                            LOGGER.warning("Push Server service reported: {}".format(push_command.msg))
                            self.stop_push_client(push_client)
                            force_mcs_server_command_processing = True
                        elif push_command.msg_type == mcs_push_client.MsgType.Wakeup:
                            LOGGER.debug("Received Wakeup from Push Server")
                            force_mcs_server_command_processing = True
                        else:
                            LOGGER.debug("Got pending push_command: {}".format(push_command.msg))
                            try:
                                commands = comms.extract_commands_from_xml(push_command.msg)
                                commands_to_run.extend(commands)
                            except Exception as exception:
                                self.stop_push_client(push_client)
                                force_mcs_server_command_processing = True
                                LOGGER.error("Failed to process MCS Push commands: {}".format(exception))

                    if commands_to_run:
                        self.__m_computer.run_commands(commands_to_run)

                    should_check_commands = force_mcs_server_command_processing or (
                                time.time() > last_command_time_check + self.__m_command_check_interval.get())

                    if should_check_commands:
                        appids = self.__m_computer.get_app_ids()
                        LOGGER.debug("Checking for commands for %s", str(appids))
                        commands = comms.query_commands(appids)
                        last_command_time_check = time.time()

                        mcs_token_before_commands = self.__get_mcs_token()

                        if commands:
                            LOGGER.debug("Got commands")

                        if self.__m_computer.run_commands(commands):  # To run any pending commands as well
                            self.status_updated(reason="applying commands")
                            mcs_token_after_commands = self.__get_mcs_token()
                            if mcs_token_before_commands != mcs_token_after_commands:
                                self.__update_user_agent()

                        if self.should_generate_new_jwt_token():
                            comms.set_jwt_token_settings()
                            # Status needs to contain device ID and tenant ID
                            self.__m_agent.set_device_id_for_status(comms.m_device_id)
                            self.__m_agent.set_tenant_id_for_status(comms.m_tenant_id)

                        error_count = 0

                    # check for new flags
                    last_flag_time_check = self.get_flags(last_flag_time_check)
                    flags.combine_flags_files()

                    # Uncomment this line if we add endpoint flags that are MCS-relevant
                    # flags.get_mcs_relevant_flags()

                    # If we checked commands, we need to reset the check interval
                    # This is separated from the segment above to get flags before attempting push server connection
                    # Push server connection can take long due to a bad proxy
                    if should_check_commands:
                        # If the push server is not connected, but has received mcs policy settings previously or
                        # If the settings have just changed, we need to attempt to connect to server.
                        if push_client.ensure_push_server_is_connected(self.__m_config,
                                                                       comms.ca_cert(),
                                                                       comms.create_list_of_proxies_for_push_client(),
                                                                       comms.get_v2_headers()):
                            self.__m_command_check_interval.set_use_fallback_polling_interval(True)
                        else:
                            self.__m_command_check_interval.set_use_fallback_polling_interval(False)

                        self.__m_command_check_interval.set()

                        LOGGER.info(
                            "Next command check in %.2f s",
                            self.__m_command_check_interval.get())

                    timeout_compensation = (time.time() - last_command_time_check)

                    # Check to see if any adapters have new status
                    if self.__m_computer.has_status_changed() or self.__m_mcs_adapter.has_new_status():
                        self.status_updated(reason="adapter reporting status change")

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


                    # get all pending datafeeds
                    gather_datafeed_files()

                    # send statuses, events and responses only if not in error state
                    if error_count == 0:

                        if self.__m_status_timer.send_status():
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


                        if events_timer.send_events():
                            LOGGER.debug("Sending events")
                            try:
                                comms.send_events(events)
                                events_timer.events_sent()
                                events.reset()
                            except Exception:
                                events_timer.error_sending_events()
                                raise

                        # Send live query responses
                        if responses.has_responses():
                            LOGGER.debug("Sending responses")
                            try:
                                comms.send_responses(responses.get_responses())
                                responses.reset()
                            except Exception as exception:
                                LOGGER.error("Failed to send responses: {}".format(str(exception)))

                        # Send datafeed results
                        send_datafeed_files()

                    # reset command poll
                except PermissionError as ex:
                    LOGGER.warning("Got PermissionError error: {}".format(str(ex)))
                except socket.error as ex:
                    LOGGER.warning("Got socket error: {}".format(str(ex)))
                    error_count += 1

                    self.on_error(push_client, error_count)
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
                    self.on_error(push_client, error_count, transient=False)
                except mcs_connection.MCSHttpGatewayTimeoutException as exception:
                    # already logged in mcs_connection when error raised
                    error_count += 1
                    self.on_error(push_client, error_count)
                except mcs_connection.MCSHttpException as exception:
                    error_count += 1
                    transient = True

                    # Don't re-use old cookies after an error, as this may trigger de-duplication
                    LOGGER.info("Forgetting cookies due to http exception")
                    comms.clear_cookies()
                    comms.close()

                    if exception.error_code() == 503:
                        LOGGER.warning("Endpoint has been temporarily suspended, possibly due to the machine "
                                       "being deleted from the Sophos Central Console. "
                                       "Communication should resume within the hour.")
                        transient = False
                    elif exception.error_code() == 400:
                        # From MCS spec section 12 - HTTP Bad Request is
                        # semi-permanent
                        transient = False

                    self.on_error(push_client, error_count, transient)
                except (mcs_exception.MCSNetworkException, http.client.NotConnected):
                    # Already logged from mcsclient
                    error_count += 1
                    self.on_error(push_client, error_count)
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
                    self.on_error(push_client, error_count)

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

                if error_count > 0:
                    # if during backoff flush pipe so that the pipe is cleared every four hours at miniumum
                    while True:
                        try:
                            if os.read(
                                    notify_pipe_file_descriptor,
                                    1024) is None:
                                break
                        except OSError as err:
                            if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                                break

                try:
                    before = time.time()
                    # pylint: disable=unused-variable
                    if error_count > 0:
                        # Do not break back off for events or statuses being generated
                        ready_to_read, ready_to_write, in_error = \
                            select.select(
                                [signal_handler.sig_term_pipe[0], push_notification_pipe_file_descriptor],
                                [],
                                [],
                                timeout)
                    else:
                        ready_to_read, ready_to_write, in_error = \
                            select.select(
                                [signal_handler.sig_term_pipe[0], notify_pipe_file_descriptor, push_notification_pipe_file_descriptor],
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
                elif push_notification_pipe_file_descriptor in ready_to_read:
                    LOGGER.debug("Pipe notified that there is new command from Push Server")
                elif (after - before) < timeout:
                    LOGGER.debug(
                        "select exited with no event after=%f before=%f delta=%f timeout=%f",
                        after,
                        before,
                        after - before,
                        timeout)

        finally:
            if push_client:
                push_client.stop_service()
            if self.__m_comms is not None:
                self.__m_comms.close()
                self.__m_comms = None
            for filename in os.listdir(path_manager.action_dir()):
                file_path = os.path.join(path_manager.action_dir(), filename)
                try:
                    if os.path.isfile(file_path) or os.path.islink(file_path):
                        os.unlink(file_path)
                        LOGGER.debug("Removing file {} as part of mcs shutdown".format(file_path))

                except Exception as ex:
                    LOGGER.error('Failed to delete file {} due to error {}'.format(file_path, str(ex)))

        return 0
