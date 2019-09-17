#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.
"""
computer Module
"""

from __future__ import print_function, division, unicode_literals

import logging
import os
import glob

from .mcsclient import status_cache
from .utils import timestamp, path_manager

LOGGER = logging.getLogger(__name__)


class Computer(object):
    """
    Class that represents a computer with adapters and a connection to a management service
    """

    def __init__(self, status_cache_var=None):
        """
        __init__
        """
        self.__m_adapters = {}
        self.__m_commands = []
        self.__m_status_cache = status_cache_var or status_cache.StatusCache()

    def add_adapter(self, adapter):
        """
        add_adapter
        """
        app_id = adapter.get_app_id()
        LOGGER.info("Adding %s adapter", app_id)
        self.__m_adapters[app_id] = adapter

    def remove_adapter_by_app_id(self, app_id):
        """
        remove_adapter_by_app_id
        """
        try:
            del self.__m_adapters[app_id]
        except KeyError as exception:
            LOGGER.warning('Failed to remove adapter: ' + str(exception))

    def get_timestamp(self):
        """
        get_timestamp
        """
        # pylint: disable=no-self-use
        return timestamp.timestamp()

    def fill_status_event(self, status_event):
        """
        Return True if any adapter's status has changed
        """
        # Enumerate adapters statuses - including self
        changed = False
        for adapter in self.__m_adapters.values():
            status_xml = adapter.get_status_xml()
            if status_xml is None:
                # No status for this adapter
                continue

            if not self.__m_status_cache.has_status_changed_and_record(
                    adapter.get_app_id(), status_xml):
                # Status for this adapter hasn't actually changed
                continue

            LOGGER.info("Sending status for %s adapter", adapter.get_app_id())

            # Status has changed so add it to the message
            status_event.add_adapter(
                adapter.get_app_id(),
                adapter.get_status_ttl(),
                self.get_timestamp(),
                status_xml)

            changed = True

        return changed

    def has_status_changed(self):
        """
        has_status_changed
        """
        for adapter in self.__m_adapters.values():
            if adapter.has_new_status():
                return True
        return False

    def direct_command(self, command):
        """
        direct_command
        """
        app_id = command.get_app_id()
        adapter = self.__m_adapters.get(app_id, None)
        if adapter is not None:
            return adapter.process_command(command)
        LOGGER.error("No adapter for %s", app_id)
        command.complete()
        return []

    def process_log_event(self, event):
        """
        process_log_event
        """
        LOGGER.debug("received log event: %s", ";".join(
            [m.msgid for m in event.getMessages()]))

        for adapter in self.__m_adapters.values():
            if adapter == self:
                continue
            xml = adapter.process_log_event(event)
            if xml is not None:
                return (adapter.get_app_id(), xml)
        return (None, None)

    def get_app_ids(self):
        """
        get_app_ids
        """
        return self.__m_adapters.keys()

    def _run_commands(self, commands):
        """
        run_commands
        """
        self.__m_commands += commands

        if not self.__m_commands:
            return False

        LOGGER.debug("Running commands:")

        while self.__m_commands:
            command = self.__m_commands.pop(0)
            LOGGER.debug("  %s", str(command))
            new_command = None
            try:
                new_command = self.direct_command(command)
            except Exception:
                LOGGER.warning("Failed to execute command: %s", str(command))
                self.__m_commands.append(command)
                raise

            if new_command is not None:
                # Put new commands first, so that we process, e.g. UpdatePolicy
                # before UpdateNow
                self.__m_commands = new_command + self.__m_commands

        return True

    def _setup_temp_dir(self, temp_path):
        if not os.path.isdir(temp_path):
            os.mkdir(temp_path)
            os.chmod(temp_path, 0o700)

    def run_commands(self, commands):
        # We are writing policies received in a single command poll to a temporary directory
        # before moving them into the mcs policy folder. This will mean that if multiple policies are
        # received for a specific appId that only the latest one is actioned.
        self._setup_temp_dir(path_manager.policy_temp_dir())
        self._setup_temp_dir(path_manager.actions_temp_dir())

        try:
            return self._run_commands(commands)
        finally:
            for filepath in glob.glob(os.path.join(path_manager.policy_temp_dir(), "*.xml")):
                try:
                    filename = os.path.basename(filepath)
                    os.rename(filepath, os.path.join(path_manager.policy_dir(), filename))
                    LOGGER.info("Distribute new policy: {}".format(filename))
                except OSError as ex:
                    LOGGER.warning("Failed to write a policy to: {}. Reason: {}".format(filepath, ex))

            # We want to introduce actions to the system in the order in which they were received.
            actions = glob.glob(os.path.join(path_manager.actions_temp_dir(), "*.xml"))
            actions.sort(key=lambda a: os.path.basename(a).split("_", 1)[0])
            for filepath in actions:
                try:
                    # This removes the timestamp (tag to sort by) from the front of the filename.
                    filename = os.path.basename(filepath).split("_", 1)[-1]
                    os.rename(filepath, os.path.join(path_manager.action_dir(), filename))
                    LOGGER.info("Distribute new action: {}".format(filename))
                except OSError as ex:
                    LOGGER.warning("Failed to write an action to: {}. Reason: {}".format(filepath, ex))

    def clear_cache(self):
        """
        clear_cache
        """
        self.__m_status_cache.clear_cache()
