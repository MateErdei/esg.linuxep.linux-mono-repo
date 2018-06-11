#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import print_function,division,unicode_literals

import logging
logger = logging.getLogger(__name__)

from mcsclient import StatusCache
import utils.Timestamp

class Computer(object):
    """
    Class that represents a computer with adapters and a connection to a management service
    """
    def __init__(self, installDir=".", statusCache=None):
        self.__m_installDir = installDir
        self.__m_adapters = {}
        self.__m_commands = []
        self.__m_statusCache = statusCache or StatusCache.StatusCache()

    def addAdapter(self, adapter):
        appId = adapter.getAppId()
        logger.info("Adding %s adapter", appId)
        self.__m_adapters[appId] = adapter

    def getTimestamp(self):
        return utils.Timestamp.timestamp()

    def fillStatusEvent(self, statusEvent):
        """
        Return True if any adapter's status has changed
        """
        ## Enumerate adapters statuses - including self
        changed = False
        for adapter in self.__m_adapters.values():
            statusXml = adapter.getStatusXml()
            if statusXml is None:
                ## No status for this adapter
                continue

            if not self.__m_statusCache.hasStatusChangedAndRecord(adapter.getAppId(),statusXml):
                ## Status for this adapter hasn't actually changed
                continue

            logger.info("Sending status for %s adapter", adapter.getAppId())

            ## Status has changed so add it to the message
            statusEvent.addAdapter(
                    adapter.getAppId(),
                    adapter.getStatusTTL(),
                    self.getTimestamp(),
                    statusXml)

            changed = True

        return changed

    def hasStatusChanged(self):
        for adapter in self.__m_adapters.values():
            if adapter.hasNewStatus():
                return True
        return False

    def directCommand(self, command):
        appid = command.getAppId()
        adapter = self.__m_adapters.get(appid, None)
        if adapter is not None:
            return adapter.processCommand(command)
        logger.error("No adapter for %s",appid)
        command.complete()
        return []

    def processLogEvent(self, event):
        logger.debug("received log event: %s",";".join([m.msgid for m in event.getMessages()]))

        for adapter in self.__m_adapters.values():
            if adapter == self:
                continue
            xml = adapter.processLogEvent(event)
            if xml is not None:
                return (adapter.getAppId(),xml)
        return (None,None)

    def getAppIds(self):
        return self.__m_adapters.keys()

    def runCommands(self, commands):
        self.__m_commands += commands

        if len(self.__m_commands) == 0:
            return False

        logger.debug("Running commands:")

        while len(self.__m_commands) > 0:
            command = self.__m_commands.pop(0)
            logger.debug("  %s",str(command))
            n = None
            try:
                n = self.directCommand(command)
            except Exception:
                logger.warning("Failed to execute command: %s",str(command))
                self.__m_commands.append(command)
                raise

            if n is not None:
                self.__m_commands += n

        return True

    def clearCache(self):
        self.__m_statusCache.clearCache()
