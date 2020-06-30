#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import shutil
import tarfile

import PathManager
SUPPORTFILESPATH = PathManager.get_support_file_path()
PathManager.addPathToSysPath(SUPPORTFILESPATH)

from PluginCommunicationTools.common.SetupLogger import setup_logging

LOGGER = setup_logging("system_product_test_import", "import from Base SystemProductTestOutput log")

TEST_UTILS_PATH = PathManager.get_testUtils_dir()
DESTINATION = os.path.join(TEST_UTILS_PATH, "SystemProductTestOutput")

def getSystemProductTestOutput(install_base_path=None):
    if install_base_path is None:
        install_base_path = TEST_UTILS_PATH

    dest = DESTINATION
    try:
        deststat = os.stat(dest)
    except EnvironmentError:
        deststat = None

    
    everest_base_path = PathManager.find_local_base_dir_path()
    system_product_test_output = []
    if everest_base_path:
        system_product_test_output.append(os.path.join(everest_base_path, "output"))
        system_product_test_output.append(os.path.join(everest_base_path, "cmake-build-debug", "SystemProductTestOutput"))
        system_product_test_output.append(os.path.join(everest_base_path, "build64", "SystemProductTestOutput"))
    env_path = os.environ.get("SYSTEM_PRODUCT_TEST_OUTPUT")
    if env_path:
        system_product_test_output.insert(0, env_path)
    LOGGER.info("Attempt to detect path of systemproducttest in the following order {}".format(system_product_test_output))
    system_product_test_output_found = False
    system_product_test_path = ""
    for path in system_product_test_output:
        LOGGER.info("checking {}".format(path))
        if os.path.isdir(path):
            tarPath = os.path.join(path, "SystemProductTestOutput.tar.gz")
            if os.path.isfile(tarPath):
                LOGGER.info("stat file {}".format(tarPath))
                srcstat = os.stat(tarPath)
                LOGGER.info("file found {}".format(tarPath))
                system_product_test_output_found = True
                system_product_test_path = tarPath

                if deststat is not None and deststat.st_mtime > srcstat.st_mtime:
                    LOGGER.info("Already unpacked System Product Test Output from {} to {}".format(tarPath, install_base_path))
                    break

                LOGGER.info("Untaring System Product Test Output from {} to {}".format(tarPath, install_base_path))
                with tarfile.open(tarPath, "r|gz") as tar_file:
                    tar_file.extractall(install_base_path)
                break
            else:
                shutil.copytree(path, dest)
                system_product_test_output_found = True
                break

    if not system_product_test_output_found:
        LOGGER.info(TEST_UTILS_PATH)
        LOGGER.error("Can't find System Product Test Output!")
        raise AssertionError("Can't find System Product Test Output!, looked at {}".format(TEST_UTILS_PATH))

    return system_product_test_path


class SystemProductTestOutputInstall(object):

    def __init__(self):
        self.install_base_path = TEST_UTILS_PATH

    def install_system_product_test_output(self, capnp_location):
        LOGGER.info(TEST_UTILS_PATH)
        system_product_test_path = getSystemProductTestOutput(self.install_base_path)
        capnp_tar_path = os.path.join(capnp_location, "capnproto.tar")
        LOGGER.info(f"capn tar path: {capnp_tar_path}")
        capnp_prod_sys_test_output_path = os.path.join(self.install_base_path, "SystemProductTestOutput")
        LOGGER.info(capnp_prod_sys_test_output_path)
        if capnp_location == 'IGNORE':
            return system_product_test_path, capnp_prod_sys_test_output_path

        ret_code = os.system("tar -xf {} -C {} capnproto/include/capnp/ --strip-components 2 > /dev/null".format(capnp_tar_path, capnp_prod_sys_test_output_path))
        if ret_code != 0 or not os.path.exists(os.path.join(capnp_prod_sys_test_output_path, "capnp")):
            raise AssertionError("CAPNPROTO not extracted correctly! return is {} ".format(ret_code))
        
        
        return system_product_test_path, capnp_prod_sys_test_output_path

    def clean_up_system_product_test_output(self):
        shutil.rmtree(os.path.join(self.install_base_path, "SystemProductTestOutput"))

