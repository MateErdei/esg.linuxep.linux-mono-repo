#!/usr/bin/env python

from __future__ import print_function, division, unicode_literals

import os
import xml.dom

import mcsrouter.utils.XmlHelper


class Command(object):
    def get_connection(self):
        return self._m_connection

    def get_app_id(self):
        return self._m_app_id

    def complete(self):
        """
        Close off and delete the command, as it has been completed.
        """
        return self._m_connection.delete_command(self._m_command_id)

    def get(self, key):
        raise NotImplementedError()

    def get_policy(self):
        raise NotImplementedError()

    def get_xml_text(self):
        return ""


class BasicCommand(Command):
    def __get_text_from_element(self, element):
        return mcsrouter.utils.XmlHelper.get_text_from_element(element)

    def __decode_command(self, command_node):
        values = {}
        for node in command_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                values[node.tagName] = self.__get_text_from_element(node)
        return values

    def __init__(self, connection, command_node, xml_text):
        self._m_connection = connection
        self.__m_values = self.__decode_command(command_node)
        self._m_command_id = self.__m_values['id']
        self.__m_body = self.__m_values['body']
        self._m_app_id = self.__m_values['appId']
        self.__m_xml_text = xml_text

    def get(self, key):
        return self.__m_values[key]

    def get_xml_text(self):
        return self.__m_xml_text

    def __repr__(self):
        return "Command %s for %s: %s" % (
            self._m_command_id, self.get_app_id(), str(self.__m_values))


class PolicyCommand(Command):
    def __init__(self, command_id, app_id, policy_id, mcs_connection):
        """
        Represent a Policy apply command
        """
        self._m_command_id = command_id
        self._m_app_id = app_id
        self.__m_policy_id = policy_id
        self._m_connection = mcs_connection

    def __str__(self):
        return "Policy command %s for %s" % (
            self._m_command_id, self.get_app_id())

    def get_policy(self):
        """
        Query MCS for the policy
        """
        return self._m_connection.get_policy(
            self.get_app_id(), self.__m_policy_id)

    def complete(self):
        """
        Do nothing for a Policy Command
        """
        pass


class FragmentedPolicyCommand(PolicyCommand):
    FRAGMENT_CACHE_DIR = None
    FRAGMENT_CACHE = {}

    def __init__(self, command_id, app_id, fragment_nodes, mcs_connection):
        """
        Represents a fragmented policy
        """
        self._m_command_id = command_id
        self._m_app_id = app_id

        fragments = {}
        for node in fragment_nodes:
            fragments[int(node.getAttribute("seq"))] = node.getAttribute("id")

        self._m_fragments = fragments
        self._m_connection = mcs_connection

    def __str__(self):
        return "Policy command %s for %s" % (
            self._m_command_id, self.get_app_id())

    def __get_fragment(self, fragment_id):
        # In memory cache
        data = FragmentedPolicyCommand.FRAGMENT_CACHE.get(fragment_id, None)
        if data is not None:
            return data

        # On disk-cache
        if FragmentedPolicyCommand.FRAGMENT_CACHE_DIR is not None:
            try:
                data = open(
                    os.path.join(
                        FragmentedPolicyCommand.FRAGMENT_CACHE_DIR,
                        fragment_id)).read()
                FragmentedPolicyCommand.FRAGMENT_CACHE[fragment_id] = data
                return data
            except EnvironmentError:
                pass

        data = self._m_connection.get_policyFragment(
            self.get_app_id(), fragment_id)
        FragmentedPolicyCommand.FRAGMENT_CACHE[fragment_id] = data

        if FragmentedPolicyCommand.FRAGMENT_CACHE_DIR is not None:
            open(
                os.path.join(
                    FragmentedPolicyCommand.FRAGMENT_CACHE_DIR,
                    fragment_id),
                "w").write(data)

        return data

    def get_policy(self):
        """
        Query MCS for the policy
        """
        # TODO caching
        policy = []
        keys = sorted(self._m_fragments.keys())
        for seq in keys:
            fragment_id = self._m_fragments[seq]
            policy.append(self.__get_fragment(fragment_id))
        return "".join(policy)
