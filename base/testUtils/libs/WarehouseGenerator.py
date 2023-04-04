import errno
import shutil
import subprocess
import os
import robot.api.logger

import PathManager

class ComponentConfig(object):
    def __init__(self, rigidname: str, source_directory: str, target_directory: str, product_type):
        self.rigidname = rigidname
        self.source_directory = source_directory
        self.target_directory = target_directory
        self.product_type = product_type

    def get_rigidname(self):
        return self.rigidname

    def get_source_directory(self):
        return self.source_directory

    def get_target_directory(self):
        return self.target_directory

    def get_product_type(self):
        return self.product_type

    def __str__(self):
        return "{} , {}, {}, {}".format(self.rigidname, self.source_directory, self.target_directory, self.product_type)

    def __repr__(self):
        return self.__str__()


# noinspection PyInterpreter
class WarehouseGenerator(object):
    def __init__(self):
        tmp_path = os.path.join(".", "tmp")
        if not os.path.exists(tmp_path):
            os.makedirs(tmp_path)
        self.log_path = os.path.join(tmp_path, "warehouseGenerator.log")
        self.log = open(self.log_path, 'w+')

        self.warehouseConfigMap = dict()

    def get_product_type(self, component_suite=False):
        if component_suite:
            return "true"
        return "false"

    def _add_warehouse_config(self, rigidname, source_directory, target_directory, component_parent, warehouse_name, component_suite):
        '''
        Function provides implementation to store the warehouse configuration.  This function should not be called
        directly.

        :param rigidname: rigidname of the component suite or component
        :param source_directory: source file set
        :param target_directory: target file set
        :param component_parent: If a component belongs to a component suite, the parent should be
               the component suite, else the parent should be itself.
        :param warehouse_name: Name of the warehouse to generate, if None the rigidname is used.
        :param component_suite: if True, is a component suite, false is a component.
        '''

        robot.api.logger.info("component_suite = {}".format(component_suite))

        if warehouse_name is None:
            warehouse_name = rigidname
        source_directory_abs_path = os.path.abspath(source_directory)
        target_directory_abs_path = os.path.abspath(target_directory)
        robot.api.logger.info("Adding config: Rigidname = {}, Source = {}, Target = {}, WarehouseName = {}"
                              .format(rigidname, source_directory_abs_path, target_directory_abs_path, warehouse_name))

        component_config_map = self.warehouseConfigMap.get(warehouse_name, {})

        component_configs = component_config_map.get(component_parent, [])

        product_type = self.get_product_type(component_suite)
        config = ComponentConfig(rigidname, source_directory_abs_path, target_directory_abs_path, product_type)

        component_configs.append(config)

        component_config_map[component_parent] = component_configs

        self.warehouseConfigMap[warehouse_name] = component_config_map

    def generate_install_file_in_directory(self, dir_to_save_install_file):
        try:
            os.makedirs(dir_to_save_install_file)
        except OSError as ex:
            if ex.errno != errno.EEXIST:
                raise
        filename = "install.sh"
        filepath = os.path.join(dir_to_save_install_file, filename)
        try:
            os.remove(filepath)
        except OSError as ex:
            if ex.errno not in [errno.EEXIST, errno.ENOENT]:
                raise
        with open(filepath, 'w+') as installfile:
            installfile.write('#!/bin/bash\n'
                              'echo "INSTALLER EXECUTED"\n'
                              'echo args: "$@" > /tmp/args_thininstaller_called_base_installer_with\n'
                              'echo "$PRODUCT_ARGUMENTS"> /tmp/PRODUCT_ARGUMENTS\n'
                              'echo "$CUSTOMER_TOKEN_ARGUMENT"> /tmp/CUSTOMER_TOKEN_ARGUMENT\n'
                              'echo "$MCS_TOKEN"> /tmp/MCS_TOKEN\n'
                              'echo "$MCS_URL"> /tmp/MCS_URL\n'
                              'exit 0\n')