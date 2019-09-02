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
            app_ids = set(parsed_file.get('policyAppIds', []))
            app_ids = app_ids.union(parsed_file.get('statusAppIds', []))
            return app_ids

    except IOError as exception:
        LOGGER.error("Failed to load plugin file: %s", str(file_path))
        LOGGER.error(str(exception))
        return None

    except ValueError as exception:
        LOGGER.error("File not valid json: %s", str(file_path))
        LOGGER.error(str(exception))
        return None


def get_app_ids_from_directory(directory_path):
    """
    get_app_ids_from_directory
    """
    app_ids = set()
    file_names = {}
    for file_name in os.listdir(directory_path):
        if file_name.endswith('.json'):
            file_names[file_name] = set()
            app_ids_for_file = get_app_ids_from_plugin_json(os.path.join(directory_path, file_name))
            if app_ids_for_file:
                app_ids = app_ids.union(app_ids_for_file)
                file_names[file_name] = app_ids_for_file

    return app_ids, file_names


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
        self._plugin_file_names = set()
        self._prev_file_names_app_ids = {}

    def added_and_removed_app_ids(self):
        """
        added_and_removed_app_ids
        """
        # fixme improve efficiency. It is parsing the files every time.
        # It should do only when new files are added.
        app_ids, file_names_app_ids = get_app_ids_from_directory(self._plugin_registry_path)

        file_names = set(file_names_app_ids.keys())
        added_plugins = file_names.difference(self._plugin_file_names)
        removed_plugins = self._plugin_file_names.difference(file_names)
        self._plugin_file_names = file_names

        added_app_ids = app_ids.difference(self._current_app_ids)
        removed_app_ids = self._current_app_ids.difference(app_ids)
        self._current_app_ids = app_ids
        added_list = [node.encode('ascii', 'ignore') for node in added_app_ids]
        removed_list = [node.encode('ascii', 'ignore')
                        for node in removed_app_ids]
        added_list.sort()
        removed_list.sort()

        if added_plugins:
            for plugin in added_plugins:
                message = "Plugin found: {}, with APPIDs: {}"\
                    .format(plugin, ', '.join(file_names_app_ids[plugin]))
                LOGGER.info(message)

        if removed_plugins:
            for plugin in removed_plugins:
                message = "Plugin removed: {}, with APPIDs: {}"\
                    .format(plugin, ', '.join(self._prev_file_names_app_ids[plugin]))
                LOGGER.info(message)
        self._prev_file_names_app_ids = file_names_app_ids
        return added_list, removed_list
