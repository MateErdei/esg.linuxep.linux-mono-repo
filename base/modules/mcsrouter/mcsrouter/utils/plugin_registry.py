"""
plugin_registry Module
"""
#pylint: disable=too-few-public-methods

import json
import logging
import os

import mcsrouter.utils.path_manager

LOGGER = logging.getLogger(__name__)


def get_app_ids_from_plugin_json(file_path):
    """
    get_app_ids_from_plugin_json
    """
    try:
        with open(file_path, 'r') as file_to_read:
            parsed_file = json.load(file_to_read)
            app_ids = set(parsed_file.get(u'policyAppIds', []))
            app_ids = app_ids.union(parsed_file.get(u'statusAppIds', []))
            return app_ids

    except IOError as exception:
        LOGGER.error("Failed to load plugin file: %s", str(file_path))
        LOGGER.error(str(exception))
        return None


def get_app_ids_from_directory(directory_path):
    """
    get_app_ids_from_directory
    """
    app_ids = set()
    for file_name in os.listdir(directory_path):
        if file_name.endswith('.json'):
            app_ids_for_file = get_app_ids_from_plugin_json(
                os.path.join(directory_path, file_name))
            if app_ids_for_file:
                LOGGER.info(
                    "Apps for plugin " +
                    file_name +
                    " are: " +
                    ', '.join(app_ids_for_file))
                app_ids = app_ids.union(app_ids_for_file)

    return app_ids


class PluginRegistry(object):
    """
    PluginRegistry
    """

    def __init__(self, install_dir):
        """
        __init__
        """
        mcsrouter.utils.path_manager.INST = install_dir
        self._plugin_registry_path = mcsrouter.utils.path_manager.plugin_registry_path()
        LOGGER.info("PluginRegistry path: %s", str(self._plugin_registry_path))
        self._current_app_ids = set()

    def added_and_removed_app_ids(self):
        """
        added_and_removed_app_ids
        """
        # fixme improve efficiency. It is parsing the files every time. It
        # should do only when new files are added.
        app_ids = get_app_ids_from_directory(self._plugin_registry_path)
        added_app_ids = app_ids.difference(self._current_app_ids)
        removed_app_ids = self._current_app_ids.difference(app_ids)
        self._current_app_ids = app_ids
        added_list = [node.encode('ascii', 'ignore') for node in added_app_ids]
        removed_list = [node.encode('ascii', 'ignore')
                        for node in removed_app_ids]
        added_list.sort()
        removed_list.sort()

        return added_list, removed_list
