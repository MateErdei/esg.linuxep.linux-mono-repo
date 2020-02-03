import errno
import shutil
import subprocess
import os
import robot.api.logger

import PathManager

class ComponentConfig(object):
    def __init__(self, rigidname, source_directory, target_directory, product_type):
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
        self.script = os.path.join(PathManager.get_support_file_path(), "warehouseGeneration/generateWarehouse.sh")
        self.customer_file_script = os.path.join(PathManager.get_support_file_path(), "warehouseGeneration/generateCustomerFile.sh")
        if not os.path.isfile(self.script):
            raise AssertionError("Cannot find: {}".format(self.script))

        tmp_path = os.path.join(".", "tmp")
        if not os.path.exists(tmp_path):
            os.makedirs(tmp_path)
        self.log_path = os.path.join(tmp_path, "warehouseGenerator.log")
        self.log = open(self.log_path, 'w+')

        self.warehouseConfigMap = dict()

    def create_warehouse_generator(self):
        pass

    def write_config_to_file(self):
        file_path = os.path.join("/", "tmp", "config_file.csv")
        if os.path.exists(file_path):
            os.remove(file_path)
        with open(file_path, 'w') as config_file:
            for warehouse_name, component_config_map in list(self.warehouseConfigMap.items()):
                for component_parent, component_configs in list(component_config_map.items()):
                    for config in component_configs:
                        config_file.write("{},{},{},{},{},{}\n".format(
                            warehouse_name, component_parent, config.get_rigidname(), config.get_source_directory(),
                            config.get_target_directory(), config.get_product_type()))

    def get_product_type(self, component_suite=False):
        if component_suite == True:
            return "true"
        return "false"

    def add_component_suite_warehouse_config(self, rigidname, source_directory, target_directory, warehouse_name=None):
        self._add_warehouse_config(rigidname, source_directory, target_directory, rigidname, warehouse_name, component_suite=True)

    def add_component_warehouse_config(self, rigidname, source_directory, target_directory, component_parent=None, warehouse_name=None):
        if component_parent == None:
            component_parent = rigidname
        self._add_warehouse_config(rigidname, source_directory, target_directory, component_parent, warehouse_name, component_suite=False)

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

    def clear_warehouse_config(self):
        self.warehouseConfigMap = dict()

    def dump_warehouse_generator_log(self):
        filename = self.log_path
        if os.path.exists(filename):
            try:
                with open(filename, "r") as f:
                    robot.api.logger.info(''.join(f.readlines()))
            except Exception as e:
                robot.api.logger.info("Failed to read file: {}".format(e))
        else:
            robot.api.logger.info("File does not exist")

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
            installfile.write("#!/bin/bash\n"
                              "echo 'INSTALLER EXECUTED'\n"
                              "exit 0\n")

    def generate_warehouse(self, update_from_sophos_location=True, **kwargs):
        if kwargs:
            robot.api.logger.info("Inside generate warehouse with extra args: {}".format(repr(kwargs)))
        # for each warehouse config generate an independent warehouse and customer file.

        self.write_config_to_file()

        for warehouse_name, component_config_map in list(self.warehouseConfigMap.items()):
            for component_parent, component_configs in list(component_config_map.items()):
                for config in component_configs:
                    if os.path.exists(config.get_target_directory()):
                        shutil.rmtree(config.get_target_directory())

        update_from_sophos = "1"
        if not update_from_sophos_location:
            update_from_sophos = "0"

        warehouse_name_list = ' '.join(list(self.warehouseConfigMap.keys()))
        root_target = None
        for warehouse_name, warehouse_configs in list(self.warehouseConfigMap.items()):
            for component_parent, warehouse_component_configs in list(warehouse_configs.items()):

                # expecting root source and target root directories are the same for each product in group
                root_source = warehouse_component_configs[0].get_source_directory()
                root_target = warehouse_component_configs[0].get_target_directory()

                robot.api.logger.info("source: " + str(root_source))
                robot.api.logger.info("target: " + str(root_target))

                for config in warehouse_component_configs:
                    target = os.path.join(root_target, config.get_rigidname())
                    if not os.path.exists(target):
                        os.makedirs(target)
                        print("making target directories")

            command = ["bash", "-x", self.script, warehouse_name, update_from_sophos]
            robot.api.logger.info("Run bash command: {}".format(repr(command)))
            process_env = os.environ.copy()

            # pass the extra arguments to the warehouse generator via environment variables
            if kwargs:
                for k, v in list(kwargs.items()):
                    process_env[k] = v
            process = subprocess.Popen(command, stdout=self.log, stderr=subprocess.STDOUT, env=process_env)
            process.communicate()
            if process.wait() != 0:
                self.dump_warehouse_generator_log()
                raise AssertionError("Failed to create warehouse: {}".format(process.wait()))

        if not root_target:
            raise AssertionError("Invalid configuration. Unable to run script to generate warehouse.")
        # Now create a customer file which contains all generated warehouses.
        command = ["bash", "-x", self.customer_file_script, root_target, update_from_sophos, warehouse_name_list]
        robot.api.logger.info("Run bash command: {}".format(repr(command)))
        process = subprocess.Popen(command, stdout=self.log, stderr=subprocess.STDOUT)
        process.communicate()
        if process.wait() != 0:
            self.dump_warehouse_generator_log()
            raise AssertionError("Failed to create customer file: {}".format(process.wait()))

        self.dump_warehouse_generator_log()
