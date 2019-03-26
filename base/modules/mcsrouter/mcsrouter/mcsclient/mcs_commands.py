#!/usr/bin/env python
"""
MCSCommand Module
"""

from __future__ import print_function, division, unicode_literals

import os
import xml.dom

import mcsrouter.utils.xml_helper


# pylint: disable=no-self-use
class Command(object):
    """
    Command
    """
    def __init__(self):
        """
        __init__
        """
        self._m_connection = None
        self._m_app_id = None
        self._m_command_id = None

    def get_connection(self):
        """
        get_connection
        """
        return self._m_connection

    def get_app_id(self):
        """
        get_app_id
        """
        return self._m_app_id

    def complete(self):
        """
        Close off and delete the command, as it has been completed.
        """
        return self._m_connection.delete_command(self._m_command_id)

    def get(self, key):
        """
        get
        """
        raise NotImplementedError()

    def get_policy(self):
        """
        get_policy
        """
        raise NotImplementedError()

    def get_xml_text(self):
        """
        get_xml_text
        """
        return ""


class BasicCommand(Command):
    """
    BasicCommand
    """

    def __get_text_from_element(self, element):
        """
        __get_text_from_element
        """
        return mcsrouter.utils.xml_helper.get_text_from_element(element)

    def __decode_command(self, command_node):
        """
        __decode_command
        """
        values = {}
        for node in command_node.childNodes:
            if node.nodeType == xml.dom.Node.ELEMENT_NODE:
                values[node.tagName] = self.__get_text_from_element(node)
        return values

    def __init__(self, connection, command_node, xml_text):
        """
        __init__
        """
        super(BasicCommand, self).__init__()
        self._m_connection = connection
        self.__m_values = self.__decode_command(command_node)
        self._m_command_id = self.__m_values['id']
        self.__m_body = self.__m_values['body']
        self._m_app_id = self.__m_values['appId']
        self.__m_xml_text = xml_text

    def get(self, key):
        """
        get
        """
        return self.__m_values[key]

    def get_xml_text(self):
        """
        get_xml_text
        """
        return self.__m_xml_text

    def __repr__(self):
        """
        __repr__
        """
        return "Command %s for %s: %s" % (
            self._m_command_id, self.get_app_id(), str(self.__m_values))

    def get_policy(self):
        """
        get_policy
        """
        raise NotImplementedError()


class PolicyCommand(Command):
    """
    PolicyCommand
    """

    def __init__(self, command_id, app_id, policy_id, mcs_connection):
        """
        Represent a Policy apply command
        """
        super(PolicyCommand, self).__init__()
        self._m_command_id = command_id
        self._m_app_id = app_id
        self.__m_policy_id = policy_id
        self._m_connection = mcs_connection

    def __str__(self):
        """
        __str__
        """
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

    def get(self, key):
        """
        get
        """
        raise NotImplementedError()


class FragmentedPolicyCommand(PolicyCommand):
    """
    FragmentedPolicyCommand
    """
    FRAGMENT_CACHE_DIR = None
    FRAGMENT_CACHE = {}

    def __init__(self, command_id, app_id, fragment_nodes, mcs_connection):
        """
        Represents a fragmented policy
        """
        super(FragmentedPolicyCommand, self).__init__(command_id, app_id, None, mcs_connection)
        self._m_command_id = command_id
        self._m_app_id = app_id

        fragments = {}
        for node in fragment_nodes:
            fragments[int(node.getAttribute("seq"))] = node.getAttribute("id")

        self._m_fragments = fragments
        self._m_connection = mcs_connection

    def __str__(self):
        """
        __str__
        """
        return "Policy command %s for %s" % (
            self._m_command_id, self.get_app_id())

    def __get_fragment(self, fragment_id):
        """
        __get_fragment
        """
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

    def get(self, key):
        """
        get
        """
        raise NotImplementedError()
