
import logging
logger = logging.getLogger(__name__)

#~ import adapters.ConfigManager

#~ import EventFactory

TRUTHY = (True, "true","1")
FALSEY = (False,"false","0")

class PolicyHandlerBase(object):
    def __init__(self, installDir):
        #~ self.m_configManager = mcsrouter.adapters.ConfigManager.getGlobalConfigManager(installDir)
        #~ self.__m_eventFactory = EventFactory.EventFactory()
        pass

    def configDiffers(self, path, flatConfig, debugAsWarning=False):
        raise NotImplementedError()

        logdebug = logger.debug
        if debugAsWarning:
            logdebug = logger.warning

        layer = self.m_layer
        (consoleLeaf,consoleSettings) = self.m_configManager.retrieve(layer+"/"+path)
        (flatLeaf,flatSettings) = flatConfig.retrieve(path)

        ## Some special cases to handle differences that don't matter between the interfaces
        if len(consoleSettings) == 0 and len(flatSettings) == 0:
            ## Empty lists are the same regardless
            return False

        if consoleSettings == [""] and flatSettings == []:
            return False
        if consoleSettings == [] and flatSettings == [""]:
            return False

        if flatLeaf != consoleLeaf:
            logdebug("CONFIG differs - leaf vs. non-leaf - %s - %s vs %s",path, str(consoleSettings),str(flatSettings))
            return True

        if len(consoleSettings) == 1 and len(flatSettings) == 1:
            if consoleSettings[0] in FALSEY and flatSettings[0] in FALSEY:
                return False

            if consoleSettings[0] in TRUTHY and flatSettings[0] in TRUTHY:
                return False

        if consoleSettings != flatSettings:
            logdebug("CONFIG differs for %s - %s vs %s",path, str(consoleSettings),str(flatSettings))
            flatConfig.debug()
            return True

        return False

    def isCompliant(self):
        """
        @return True if the endpoint is compliant with the policy
        """
        raise NotImplementedError()

        try:
            self.m_configManager.openConnection()
            try:
                return self._checkCompliantWithOpenConnection()
            finally:
                self.m_configManager.closeConnection()
        except mcsrouter.adapters.ConfigManager.ConfigManagerConnectionException:
            ## Not compliant since savd isn't available
            return False

    def sendSimpleEvent(self, action):
        raise NotImplementedError()

        eventFactory = self.__m_eventFactory
        event = eventFactory.newSimpleEvent(action)
        self.m_configManager.sendEvent(event)
