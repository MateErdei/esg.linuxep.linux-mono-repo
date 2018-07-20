#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import print_function,division,unicode_literals

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

class CommandCheckInterval(object):
    DEFAULT_COMMAND_POLLING_INTERVAL = 20

    def __init__(self, config):
        self.__m_config = config
        self.__m_commandCheckIntervalMinimum = 0
        self.__m_commandCheckIntervalMaximum = 0

        self.__m_commandCheckMaximumRetryNumber = self.__m_config.getInt(
            "COMMAND_CHECK_MAXIMUM_RETRY_NUMBER",10)
        self.__m_commandCheckBaseRetryDelay = self.__m_config.getInt(
            "COMMAND_CHECK_BASE_RETRY_DELAY",20)
        self.__m_commandCheckSemiPermanentErrorRetryDelay = self.__m_config.getInt(
            "COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY",self.__m_commandCheckBaseRetryDelay*2)

        self.set()

    def get(self):
        return self.__m_commandCheckInterval

    def __getMinimum(self):
        intervalMin = self.__m_config.getInt("COMMAND_CHECK_INTERVAL_MINIMUM",
            self.DEFAULT_COMMAND_POLLING_INTERVAL)
        if self.__m_commandCheckIntervalMinimum != intervalMin:
            self.__m_commandCheckIntervalMinimum = intervalMin
            logger.debug("COMMAND_CHECK_INTERVAL_MINIMUM=%d",intervalMin)
        return intervalMin

    def __getMaximum(self):
        intervalMax = self.__m_config.getInt("COMMAND_CHECK_INTERVAL_MAXIMUM",
            self.DEFAULT_COMMAND_POLLING_INTERVAL)
        if self.__m_commandCheckIntervalMaximum != intervalMax:
            self.__m_commandCheckIntervalMaximum = intervalMax
            logger.debug("COMMAND_CHECK_INTERVAL_MAXIMUM=%d",intervalMax)
        return intervalMax

    def set(self, val=None):
        if val is None:
            val = self.__m_commandCheckBaseRetryDelay
        val = max(val, self.__getMinimum())
        val = min(val, self.__getMaximum())
        self.__m_commandCheckInterval = val
        return self.__m_commandCheckInterval

    def increment(self, val=None):
        if val is None:
            val = self.__m_commandCheckBaseRetryDelay
        self.set(self.__m_commandCheckInterval + val)

    def setOnError(self, errorCount, transient=True):
        maxRetryNumber = self.__m_commandCheckMaximumRetryNumber
        retryNumber = min(errorCount+1,maxRetryNumber)
        if transient:
            baseRetryDelay = self.__m_commandCheckBaseRetryDelay
        else:
            ## Semi-Permanent - From MCS spec section 12 - "Consequently, the same back-off algorithm as for transient failures shall be used but with a larger base delay."
            baseRetryDelay = self.__m_commandCheckSemiPermanentErrorRetryDelay

        ## From MCS spec 12.1.1:
        ###     retryDelay = random(baseRetryDelay * 2 ^ (retryNumber - 1))
        ###     Where random is a function that generates a random number in the range 0 to the value of its parameter
        retryDelay = random.uniform(0, baseRetryDelay * (2 ** (retryNumber - 1)))
        self.set(retryDelay)
        logger.info("[backoff] waiting %fs after %d failures", self.__m_commandCheckInterval, errorCount)



class MCS(object):
    def __init__(self, config, installDir):
        self.__m_comms = None
        config.setDefault("MCSURL","https://mcs-amzn-eu-west-1-f9b7.upe.d.hmr.sophos.com/sophos/management/ep")
        fixedConfig = utils.Config.Config(filename=os.path.join(installDir,"etc","mcs.config"),parentConfig=config)
        self.__m_policy_config = utils.Config.Config(filename=os.path.join(installDir,"etc","sophosspl","mcs_policy.config"),parentConfig=fixedConfig)
        self.__m_config = utils.Config.Config(filename=os.path.join(installDir,"etc","sophosspl","mcs.config"),parentConfig=self.__m_policy_config)
        config = self.__m_config

        self.__m_installDir = installDir
        statusLatency = self.__m_config.getInt("STATUS_LATENCY",30)
        logger.debug("STATUS_LATENCY=%d", statusLatency)
        self.__m_statusTimer = mcsclient.StatusTimer.StatusTimer(
            statusLatency,
            self.__m_config.getInt("STATUS_INTERVAL",60*60*24)
            )

        ## Configure fragmented policy cache directory
        mcsclient.MCSCommands.FragmentedPolicyCommand.FRAGMENT_CACHE_DIR = \
            os.path.join(self.__m_installDir,"var","cache","mcs_fragmented_policies")

        ## Create computer
        self.__m_computer = Computer.Computer(self.__m_installDir)

        self.__m_computer.addAdapter(adapters.MCSAdapter.MCSAdapter(installDir,self.__m_policy_config,self.__m_config))

        #~ apps = [ "ALC", "SAV", "HBT", "NTP", "SHS", "SDU", "UC", "MR" ]
        self.__plugin_registry = utils.PluginRegistry.PluginRegistry(installDir)
        apps, ignored = self.__plugin_registry.added_and_removed_appids()

        for app in apps:
            self.__m_computer.addAdapter(adapters.GenericAdapter.GenericAdapter(app, self.__m_installDir))

        ## AppsProxy needs to report all AppIds apart from AGENT and APPSPROXY
        appids = self.__m_computer.getAppIds()
        self.__m_computer.addAdapter(adapters.AppProxyAdapter.AppProxyAdapter(appids))

        self.__m_agent = adapters.AgentAdapter.AgentAdapter()
        self.__m_computer.addAdapter(self.__m_agent)

        ## TODO: reload on MCS policy change (and any other configurables? Event & status regulation?)
        self.__m_commandCheckInterval = CommandCheckInterval(config)

    def startup(self):
        """
        Connect and register if required
        """
        config = self.__m_config
        installDir = self.__m_installDir

        comms = mcsclient.MCSConnection.MCSConnection(
            config,
            installDir=installDir
            )

        self.__updateUserAgent(comms)

        ## Check capabilities
        capabilities = comms.capabilities()
        logger.info("Capabilities=%s",capabilities)
        ## TODO parse and verify if we need something beyond baseline

        self.__m_comms = comms

    def __updateUserAgent(self, comms=None):
        if comms is None:
            comms = self.__m_comms
            if comms is None:
                return

        installDir = self.__m_installDir
        productVersionFile = os.path.join(installDir,"engine","savVersion")
        try:
            productVersion = open(productVersionFile).read().strip()
        except EnvironmentError:
            productVersion = "unknown"

        config = self.__m_config
        token = config.getDefault("MCSToken","unknown")

        comms.setUserAgent(mcsclient.MCSConnection.createUserAgent(productVersion,token))

    def register(self):
        config = self.__m_config
        assert config is not None
        agent = self.__m_agent
        assert agent is not None
        comms = self.__m_comms
        assert comms is not None

        logger.info("Registering")
        status = agent.getStatusXml()
        token = config.get("MCSToken")
        (endpointid, password) = comms.register(token,status)
        config.set("MCSID",endpointid)
        config.set("MCSPassword",password)
        config.set("MCS_saved_token",token)
        config.save()

    def run(self):
        config = self.__m_config

        if config.getDefault("MCSID") is None:
            logger.critical("Not registered: MCSID is not present")
            return 1
        if config.getDefault("MCSPassword") is None:
            logger.critical("Not registered: MCSPassword is not present")
            return 2

        installDir = self.__m_installDir
        comms = self.__m_comms
        computer = self.__m_computer

        events = mcsclient.Events.Events()
        eventsTimer = mcsclient.EventsTimer.EventsTimer(
            config.getInt("EVENTS_REGULATION_DELAY",5), ## Mac appears to use 3
            config.getInt("EVENTS_MAX_REGULATION_DELAY",60),
            config.getInt("EVENTS_MAX_EVENTS",20)
            )
        eventReceiver = adapters.EventReceiver.EventReceiver(installDir)

        def addEvent(*eventArgs):
            events.addEvent(*eventArgs)
            eventsTimer.eventAdded()
            logger.debug("Next event update in %.2f s",eventsTimer.relativeTime())

        def statusUpdated(reason=None):
            logger.debug("Checking for status update due to %s", reason or "unknown reason")

            self.__m_statusTimer.statusUpdated()
            logger.debug("Next status update in %.2f s",self.__m_statusTimer.relativeTime())

        ## setup signal handler before connecting
        utils.SignalHandler.setupSignalHandler()

        ## setup a directory watcher for events and statuses
        directoryWatcher = utils.DirectoryWatcher.DirectoryWatcher()
        directoryWatcher.add_watch(os.path.join(installDir, "event"), patterns=["*.xml"], ignore_delete=True)
        directoryWatcher.add_watch(os.path.join(installDir, "status"), patterns=["*.xml"])
        notify_pipe_fd = directoryWatcher.notify_pipefd

        lastCommands = 0

        running = True
        reregister = False
        errorCount = 0

        # see if registerCentral.py --reregister has been called
        if config.getDefault("MCSID") == "reregister":
            reregister = True

        try:
            while running:
                timeout = self.__m_commandCheckInterval.get()
                beforeTime = time.time()

                try:
                    if comms is None:
                        self.startup()
                        comms = self.__m_comms

                    if reregister:
                        logger.info("Re-registering with MCS")
                        self.register()
                        reregister = False
                        errorCount = 0
                        # If re-registering due to a de-dupe from Central, clear cache and re-send status.
                        computer.clearCache()
                        statusUpdated(reason="reregistration")

                    # Check for any new appids 'for newly installed plugins'
                    added_apps, removed_apps = self.__plugin_registry.added_and_removed_appids()
                    if added_apps or removed_apps:
                        if  added_apps:
                            logger.info("New AppIds found to register for: " + ' ,'.join(added_apps))
                        if removed_apps:
                            logger.info("AppIds not supported anymore: " ' ,'.join(removed_apps))
                            # Not removing adapters if plugin uninstalled - this will cause Central to delete commands
                        for app in added_apps:
                            self.__m_computer.addAdapter(adapters.GenericAdapter.GenericAdapter(app, self.__m_installDir))
                        appids = [app for app in self.__m_computer.getAppIds() if app not in ['APPSPROXY','AGENT']]
                        logger.info("Reconfiguring the APPSPROXY to handle: " + ' '.join(appids))
                        self.__m_computer.removeAdapterByAppId('APPSPROXY')
                        self.__m_computer.addAdapter(adapters.AppProxyAdapter.AppProxyAdapter(appids))

                    if time.time() > lastCommands + self.__m_commandCheckInterval.get():
                        logger.debug("Checking for commands")
                        commands = comms.queryCommands(computer.getAppIds())
                        lastCommands = time.time()
                        if computer.runCommands(commands): ## To run any pending commands as well
                            statusUpdated(reason="applying commands")

                        if len(commands) > 0:
                            logger.debug("Got commands; resetting interval")
                            self.__m_commandCheckInterval.set()
                        else:
                            logger.debug("No commands; increasing interval")
                            self.__m_commandCheckInterval.increment()
                        errorCount = 0

                        logger.debug("Next command check in %.2f s", self.__m_commandCheckInterval.get())

                    timeout = self.__m_commandCheckInterval.get() - (time.time() - lastCommands)

                    # Check to see if any adapters have new status
                    if computer.hasStatusChanged():
                        statusUpdated(reason="adapter reporting status change")

                    ## get all pending events
                    for appid, event_time, event in eventReceiver.receive():
                        logger.info("queuing event for %s", appid)
                        addEvent(appid,event,utils.Timestamp.timestamp(event_time),10000,utils.IdManager.id())

                    ## send status
                    if errorCount > 0:
                        pass # Not sending status while in error state
                    elif self.__m_statusTimer.sendStatus():
                        statusEvent = mcsclient.StatusEvent.StatusEvent()
                        changed = computer.fillStatusEvent(statusEvent)
                        if changed:
                            logger.debug("Sending status")
                            try:
                                comms.sendStatusEvent(statusEvent)
                                self.__m_statusTimer.statusSent()
                            except Exception:
                                self.__m_statusTimer.errorSendingStatus()
                                raise
                        else:
                            logger.debug("Not sending status as nothing changed")
                            ## Don't actually need to send status
                            self.__m_statusTimer.statusSent()

                        logger.debug("Next status update in %.2f s",self.__m_statusTimer.relativeTime())

                    ## send events
                    if errorCount > 0:
                        pass # Not sending events while in error state
                    elif eventsTimer.sendEvents():
                        logger.debug("Sending events")
                        try:
                            comms.sendEvents(events)
                            eventsTimer.eventsSent()
                            events.reset()
                        except Exception:
                            eventsTimer.errorSendingEvents()
                            raise

                except socket.error as e:
                    logger.exception("Got socket error")
                    errorCount += 1
                    self.__m_commandCheckInterval.setOnError(errorCount)
                except mcsclient.MCSConnection.MCSHttpUnauthorizedException as e:
                    logger.warning("Lost authentication with server")
                    header = e.headers().get("www-authenticate",None) ## HTTP headers are case-insensitive
                    if header == 'Basic realm="register"':
                        reregister = True

                    else:
                        logger.error("Received Unauthenticated without register header=%s",str(header))

                    errorCount += 1
                    self.__m_commandCheckInterval.setOnError(errorCount,transient=False)
                except mcsclient.MCSConnection.MCSHttpException as e:
                    logger.exception("Got http error from MCS")
                    errorCount += 1
                    transient = True
                    if e.errorCode() == 400:
                        ## From MCS spec section 12 - HTTP Bad Request is semi-permanent
                        transient = False

                    self.__m_commandCheckInterval.setOnError(errorCount,transient)
                except mcsclient.MCSException.MCSConnectionFailedException as e:
                    ## Already logged from mcsclient
                    #~ logger.exception("Got connection failed exception")
                    errorCount += 1
                    self.__m_commandCheckInterval.setOnError(errorCount)
                except httplib.BadStatusLine as e:
                    afterTime = time.time()
                    badStatusLineDelay = afterTime-beforeTime
                    if badStatusLineDelay < 1.0:
                        logger.debug("HTTPS connection closed (httplib.BadStatusLine %s) (took %f seconds)",str(e), badStatusLineDelay, exc_info=False)
                    else:
                        logger.error("HTTPS connection closed (httplib.BadStatusLine %s) (took %f seconds)",str(e), badStatusLineDelay, exc_info=False)

                    timeout = 10
                    errorCount += 1
                    self.__m_commandCheckInterval.setOnError(errorCount)

                if errorCount == 0 and not reregister:
                    ## Only count the timers for events and status if we don't have any errors
                    timeout = min(self.__m_statusTimer.relativeTime(), timeout)
                    timeout = min(eventsTimer.relativeTime(), timeout)

                ## Avoid busy looping and negative timeouts
                timeout = max(0.5, timeout)

                try:
                    before = time.time()
                    ready_to_read, ready_to_write, in_error = \
                                   select.select(
                                      [utils.SignalHandler.sigtermPipe[0], notify_pipe_fd],
                                      [],
                                      [],
                                      timeout)
                    after = time.time()
                except select.error as e:
                    if e.args[0] == errno.EINTR:
                        logger.debug("Got EINTR")
                        continue
                    else:
                        raise

                if utils.SignalHandler.sigtermPipe[0] in ready_to_read:
                    logger.info("Exiting MCS")
                    running = False
                    break
                elif notify_pipe_fd in ready_to_read:
                    logger.debug("Got directory watch notification")
                    ## flush the pipe
                    while True:
                        try:
                            if os.read(notify_pipe_fd, 1024) is None:
                                break
                        except OSError as err:
                            if err.errno == errno.EAGAIN or err.errno == errno.EWOULDBLOCK:
                                break
                            else:
                                raise
                elif (after - before) < timeout:
                    logger.debug("select exited with no event after=%f before=%f delta=%f timeout=%f", after, before, after-before, timeout)

        finally:
            if self.__m_comms is not None:
                self.__m_comms.close()
                self.__m_comms = None

        return 0
