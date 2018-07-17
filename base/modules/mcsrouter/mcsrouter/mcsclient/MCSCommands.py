#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import xml.dom

import mcsrouter.utils.XmlHelper

class Command(object):
    def getConnection(self):
        return self._m_connection

    def getAppId(self):
        return self._m_appid

    def complete(self):
        """
        Close off and delete the command, as it has been completed.
        """
        return self._m_connection.deleteCommand(self._m_commandid)

    def get(self, key):
        raise NotImplementedError()

    def getPolicy(self):
        raise NotImplementedError()

    def getXmlText(self):
        return ""

class BasicCommand(Command):
    def __getTextFromElement(self, element):
        return mcsrouter.utils.XmlHelper.getTextFromElement(element)

    def __decodeCommand(self, commandNode):
        values = {}
        for n in commandNode.childNodes:
            if n.nodeType == xml.dom.Node.ELEMENT_NODE:
                values[n.tagName] = self.__getTextFromElement(n)
        return values

    def __init__(self, connection, commandNode, xmlText):
        self._m_connection = connection
        self.__m_values = self.__decodeCommand(commandNode)
        self._m_commandid = self.__m_values['id']
        self.__m_body = self.__m_values['body']
        self._m_appid = self.__m_values['appId']
        self.__m_xmlText = xmlText

    def get(self, key):
        return self.__m_values[key]

    def getXmlText(self):
        return self.__m_xmlText

    def __repr__(self):
        return "Command %s for %s: %s"%(self._m_commandid, self.getAppId(), str(self.__m_values))

class PolicyCommand(Command):
    def __init__(self, commandid, appid, policyid, mcsConnection):
        """
        Represent a Policy apply command
        """
        self._m_commandid = commandid
        self._m_appid = appid
        self.__m_policyid = policyid
        self._m_connection = mcsConnection

    def __str__(self):
        return "Policy command %s for %s"%(self._m_commandid, self.getAppId())

    def getPolicy(self):
        """
        Query MCS for the policy
        """
        return self._m_connection.getPolicy(self.getAppId(), self.__m_policyid)

    def complete(self):
        """
        Do nothing for a Policy Command
        """
        pass

class FragmentedPolicyCommand(PolicyCommand):
    FRAGMENT_CACHE_DIR=None
    FRAGMENT_CACHE={}

    def __init__(self, commandid, appid, fragmentNodes, mcsConnection):
        """
        Represents a fragmented policy
        """
        self._m_commandid = commandid
        self._m_appid = appid

        fragments = {}
        for node in fragmentNodes:
            fragments[int(node.getAttribute("seq"))] = node.getAttribute("id")

        self._m_fragments = fragments
        self._m_connection = mcsConnection

    def __str__(self):
        return "Policy command %s for %s"%(self._m_commandid, self.getAppId())

    def __getFragment(self, fragmentid):
        ## In memory cache
        data = FragmentedPolicyCommand.FRAGMENT_CACHE.get(fragmentid,None)
        if data is not None:
            return data

        ## On disk-cache
        if FragmentedPolicyCommand.FRAGMENT_CACHE_DIR is not None:
            try:
                data = open(os.path.join(FragmentedPolicyCommand.FRAGMENT_CACHE_DIR,fragmentid)).read()
                FragmentedPolicyCommand.FRAGMENT_CACHE[fragmentid] = data
                return data
            except EnvironmentError:
                pass

        data = self._m_connection.getPolicyFragment(self.getAppId(), fragmentid)
        FragmentedPolicyCommand.FRAGMENT_CACHE[fragmentid] = data

        if FragmentedPolicyCommand.FRAGMENT_CACHE_DIR is not None:
            open(os.path.join(FragmentedPolicyCommand.FRAGMENT_CACHE_DIR,fragmentid),"w").write(data)

        return data

    def getPolicy(self):
        """
        Query MCS for the policy
        """
        ## TODO caching
        policy = []
        keys = self._m_fragments.keys()
        keys.sort()
        for seq in keys:
            fragmentid = self._m_fragments[seq]
            policy.append(self.__getFragment(fragmentid))
        return "".join(policy)
