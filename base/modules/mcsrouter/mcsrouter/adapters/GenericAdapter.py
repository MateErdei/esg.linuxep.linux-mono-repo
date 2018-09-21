
import xml.dom.minidom
import os

import logging
logger = logging.getLogger(__name__)

import AdapterBase
import mcsrouter.utils.AtomicWrite
import mcsrouter.utils.Timestamp
import mcsrouter.utils.PathManager as PathManager


class GenericAdapter(AdapterBase.AdapterBase):
    def __init__(self,appid,installdir=None):

        self.__m_appid = appid
        self.__m_last_status_time = None
        if installdir is not None:
            PathManager.INST = installdir

    def getAppId(self):
        return self.__m_appid

    def __processPolicy(self, policy):
        policy = policy.encode('utf-8')  # handle non ascii characters ( LINUXEP-6757 )
        logger.debug("%s Adapter processing policy %s", self.__m_appid, str(policy))
        logger.debug("Received %s policy", self.__m_appid)

        try:
            doc = xml.dom.minidom.parseString(policy)
        except xml.parsers.expat.ExpatError as e:
            logger.error("Failed to parse %s policy (%s): %s", self.__m_appid, str(e), policy)
            return []
        except Exception:
            logger.exception("Failed to parse %s policy: %s", self.__m_appid, policy)
            return []

        nodes = doc.getElementsByTagName("csc:Comp")
        if len(nodes) == 1:
            node = nodes[0]
            policyType = node.getAttribute("policyType")
            policyName = "%s-%s_policy.xml"%(self.__m_appid, policyType)
        else:
            logger.info("%s Policy didn't contain one compliance node"%self.__m_appid)
            policyName = "%s_policy.xml"%(self.__m_appid)

        policyPath = os.path.join(PathManager.policyDir(), policyName)
        policyPathTmp = os.path.join(PathManager.tempDir(), policyName)
        mcsrouter.utils.AtomicWrite.atomic_write(policyPath, policyPathTmp, policy)

        return []

    def __processAction(self, command):
        logger.debug("Received %s action", self.__m_appid)

        body = command.get(u"body")
        try:
            timestamp = command.get(u"creationTime")
        except KeyError:
            timestamp = mcsrouter.utils.Timestamp.timestamp()
        
        actionName = "%s_action_%s.xml"%(self.__m_appid, timestamp)
        actionPath = os.path.join(PathManager.actionDir(), actionName)
        actionPathTmp = os.path.join(PathManager.tempDir(), actionName)
        mcsrouter.utils.AtomicWrite.atomic_write(actionPath, actionPathTmp, body)

        return []

    def _getStatusXml(self):
        statuspath = os.path.join(PathManager.statusDir(), "%s_status.xml"%self.__m_appid)
        try:
            self.__m_last_status_time = os.path.getmtime(statuspath)
        except OSError:
            pass
        try:
            return open(statuspath).read()
        except IOError:
            return None

    def _hasNewStatus(self):
        statuspath = os.path.join(PathManager.statusDir(), "%s_status.xml"%self.__m_appid)
        try:
            status_time = os.path.getmtime(statuspath)
        except OSError:
            return False

        if self.__m_last_status_time is None:
            return True

        return status_time > self.__m_last_status_time

    def processCommand(self, command):
        try:
            logger.debug("%s Adapter processing %s", self.__m_appid, str(command))
            try:
                policy = command.getPolicy()
                return self.__processPolicy(policy)
            except NotImplementedError:
                return self.__processAction(command)
        finally:
            command.complete()
