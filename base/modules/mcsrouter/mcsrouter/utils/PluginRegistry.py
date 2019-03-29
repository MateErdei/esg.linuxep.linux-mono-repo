import json
import logging
import os

import PathManager

logger = logging.getLogger(__name__)

def get_appids_from_plugin_registry_json(file_path):

    try:
        with open(file_path, 'r') as filep:
            parsed_file = json.load(filep)
            appsids = set(parsed_file.get(u'policyAppIds', []))
            appsids = appsids.union(parsed_file.get(u'statusAppIds', []))
            return appsids

    except Exception as ex:
        logger.error("Failed to load plugin file: " + str(file_path))
        logger.error(str(ex))
        return None


def get_appids_from_directory(directory_path):
    appids = set()
    file_names = {}
    for file_name in os.listdir(directory_path):
        if file_name.endswith('.json'):
            file_names[file_name] = set()
            appids_forfile = get_appids_from_plugin_registry_json(os.path.join(directory_path, file_name))
            if appids_forfile:
                appids = appids.union(appids_forfile)
                file_names[file_name] = appids_forfile

    return appids, file_names


class PluginRegistry:
    def __init__(self, installdir):
        PathManager.INST = installdir
        self._plugin_registry_path = PathManager.pluginRegistryPath()
        logger.info("PluginRegistry path: " + self._plugin_registry_path)
        self._currentAppIds = set()
        self._plugin_file_names = set()
        self._prev_file_names_appids = {}

    def added_and_removed_appids(self):
        #fixme improve efficiency. It is parsing the files every time. It should do only when new files are added.
        appids, file_names_appids = get_appids_from_directory(self._plugin_registry_path)

        file_names = set(file_names_appids.keys())
        added_plugins = file_names.difference(self._plugin_file_names)
        removed_plugins = self._plugin_file_names.difference(file_names)
        self._plugin_file_names = self._plugin_file_names.union(file_names)

        added_app_ids = appids.difference(self._currentAppIds)
        removed_app_ids = self._currentAppIds.difference(appids)
        self._currentAppIds = appids

        added_list = [node.encode('ascii', 'ignore') for node in added_app_ids]
        removed_list = [node.encode('ascii', 'ignore') for node in removed_app_ids]
        added_list.sort()
        removed_list.sort()

        if len(added_plugins) > 0:
            for plugin in added_plugins:
                logger.info("Plugin found: {}, with APPIDs: {}".format(plugin, ', '.join(file_names_appids[plugin])))

        if len(removed_plugins) > 0:
            for plugin in removed_plugins:
                logger.info("Plugin removed: {}, with APPIDs: {}".format(plugin, ', '.join(self._prev_file_names_appids[plugin])))
        self._prev_file_names_appids = file_names_appids
        return added_list, removed_list
