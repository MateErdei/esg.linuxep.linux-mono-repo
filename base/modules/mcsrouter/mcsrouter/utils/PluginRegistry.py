import json
import logging
import os

import PathManager

logger = logging.getLogger(__name__)

def get_appids_from_plugin_registry_json(file_path):

    try:
        with open(file_path, 'r') as filep:
            parsed_file = json.load(filep)
            appsids = set(parsed_file[u'policyAppIds'])
            appsids = appsids.union(parsed_file[u'statusAppIds'])
            return appsids

    except Exception as ex:
        logger.error("Failed to load plugin file: " + str(file_path))
        logger.error(str(ex))
        return None


def get_appids_from_directory(directory_path):
    appids = set()
    for file_name in os.listdir(directory_path):
        if( file_name.endswith('.json')):
            appids_forfile = get_appids_from_plugin_registry_json(os.path.join(directory_path,file_name))
            if appids_forfile:
                logger.info("Apps for plugin " + file_name + " are: " + ', '.join(appids_forfile))
                appids = appids.union(appids_forfile)

    return appids


class PluginRegistry:
    def __init__(self, installdir):
        PathManager.INST = installdir
        self._plugin_registry_path = PathManager.pluginRegistryPath()
        logger.info("PluginRegistry path: " + self._plugin_registry_path)
        self._currentAppIds = set()

    def added_and_removed_appids(self):
        #fixme improve efficiency. It is parsing the files every time. It should do only when new files are added.
        appids = get_appids_from_directory(self._plugin_registry_path)
        added_app_ids = appids.difference(self._currentAppIds)
        removed_app_ids = self._currentAppIds.difference(appids)
        self._currentAppIds = appids
        added_list = [node.encode('ascii', 'ignore') for node in added_app_ids]
        removed_list = [node.encode('ascii', 'ignore') for node in removed_app_ids]
        added_list.sort()
        removed_list.sort()

        return added_list, removed_list
