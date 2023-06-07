#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.
import glob
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


def unpack(srctarfile, destdirectory):
    LOGGER.info(f"Untaring System Product Test Output from {srctarfile} to {destdirectory}")
    with tarfile.open(srctarfile, "r|gz") as tar_file:
        tar_file.extractall(destdirectory)


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
        system_product_test_output.append(
            os.path.join(everest_base_path, "cmake-build-release", "SystemProductTestOutput"))
        system_product_test_output.append(
            os.path.join(everest_base_path, "cmake-build-debug", "SystemProductTestOutput"))
        system_product_test_output.append(os.path.join(everest_base_path, "output"))
    env_path = os.environ.get("SYSTEM_PRODUCT_TEST_OUTPUT")
    if env_path:
        LOGGER.info(f"Considering 'SYSTEM_PRODUCT_TEST_OUTPUT' env var which is: {env_path}")
        system_product_test_output.insert(0, env_path)
    LOGGER.info(
        "Attempt to detect path of systemproducttest in the following order {}".format(system_product_test_output))
    system_product_test_output_found = False
    system_product_test_tar_path = ""
    for path in system_product_test_output:
        LOGGER.info("checking {}".format(path))
        if os.path.isdir(path):
            tarPath = os.path.join(path, "SystemProductTestOutput.tar.gz")
            if os.path.isfile(tarPath):
                LOGGER.info("stat file {}".format(tarPath))
                srcstat = os.stat(tarPath)
                LOGGER.info("file found {}".format(tarPath))
                system_product_test_output_found = True
                system_product_test_tar_path = tarPath

                if deststat is not None and deststat.st_mtime > srcstat.st_mtime:
                    LOGGER.info(
                        "Already unpacked System Product Test Output from {} to {}".format(tarPath, install_base_path))
                    break

                LOGGER.info(f"Untaring System Product Test Output from {tarPath} to {install_base_path}")
                with tarfile.open(tarPath, "r|gz") as tar_file:
                    tar_file.extractall(install_base_path)

                break
            else:
                LOGGER.info(f"Don't need to untar anything because the output is already available here: {path}")
                system_product_test_output_found = True
                break

    if not system_product_test_output_found:
        LOGGER.info(system_product_test_output)
        LOGGER.error("Can't find System Product Test Output!")
        raise AssertionError("Can't find System Product Test Output!, looked at {}".format(system_product_test_output))

    LOGGER.info(f"Using: system_product_test_output={system_product_test_tar_path}")
    return system_product_test_tar_path


def newer_folder(folder_one, folder_two):
    if os.path.isdir(folder_one) and os.path.isdir(folder_two):
        newest_timestamp_folder_one = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_one, "*"))])
        newest_timestamp_folder_two = max([os.path.getctime(file) for file in glob.glob(os.path.join(folder_two, "*"))])

        if newest_timestamp_folder_one >= newest_timestamp_folder_two:
            return folder_one
        else:
            return folder_two
    elif os.path.isdir(folder_one):
        return folder_one
    elif os.path.isdir(folder_two):
        return folder_two
    return None


class SystemProductTestOutputInstall(object):
    ROBOT_LIBRARY_SCOPE = "GLOBAL"

    def __init__(self):
        self.install_base_path = TEST_UTILS_PATH
        self.__m_temp_system_product_test_output = None

    def install_system_product_test_output(self):
        # CI / jenkins example:
        # system_product_test_tar_path = /tmp/system-product-test-inputs/SystemProductTestOutput/SystemProductTestOutput.tar.gz
        # system_product_test_output_path = /opt/sspl/SystemProductTestOutput

        # Local example:
        # system_product_test_tar_path = /vagrant/esg.linuxep.everest-base/output/SystemProductTestOutput.tar.gz
        # system_product_test_output_path = /vagrant/esg.linuxep.everest-base/cmake-build-release/SystemProductTestOutput

        base_path = PathManager.find_local_base_dir_path()
        system_product_test_tar_path = None
        newest_local_dir = None
        if base_path:
            expected_path_of_local_test_output_tar = os.path.join(base_path, "output/SystemProductTestOutput.tar.gz")
            expected_path_of_local_test_output_dir_release = os.path.join(base_path,
                                                                          "cmake-build-release/SystemProductTestOutput")
            expected_path_of_local_test_output_dir_debug = os.path.join(base_path,
                                                                        "cmake-build-debug/SystemProductTestOutput")
            newest_local_dir = newer_folder(expected_path_of_local_test_output_dir_release,
                                            expected_path_of_local_test_output_dir_debug)

            if os.path.isfile(expected_path_of_local_test_output_tar):
                system_product_test_tar_path = expected_path_of_local_test_output_tar

        if not system_product_test_tar_path:
            system_product_test_tar_path = getSystemProductTestOutput(self.install_base_path)
        LOGGER.info(f"system_product_test_tar_path={system_product_test_tar_path}")

        if newest_local_dir and os.path.isdir(newest_local_dir):
            system_product_test_path = newest_local_dir
        else:
            system_product_test_path = os.path.join(self.install_base_path, "SystemProductTestOutput")

        if (system_product_test_tar_path and
                os.path.isfile(system_product_test_tar_path) and
                not os.path.isdir(system_product_test_path)):
            system_product_test_path = "/tmp/SystemProductTestOutput"
            self.__m_temp_system_product_test_output = system_product_test_path
            unpack(system_product_test_tar_path, "/tmp")

        assert os.path.isdir(system_product_test_path)
        LOGGER.info(f"system_product_test_path={system_product_test_path}")

        return system_product_test_tar_path, system_product_test_path

    def clean_up_system_product_test_output(self):
        shutil.rmtree(os.path.join(self.install_base_path, "SystemProductTestOutput"), ignore_errors=True)
        if self.__m_temp_system_product_test_output:
            shutil.rmtree(os.path.join(self.__m_temp_system_product_test_output), ignore_errors=True)

