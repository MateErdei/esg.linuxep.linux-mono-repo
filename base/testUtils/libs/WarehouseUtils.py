#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import shutil
import hashlib
import glob
import UpdateServer
import robot.api.logger as logger
from robot.libraries.BuiltIn import BuiltIn

import PathManager

THIS_FILE = os.path.realpath(__file__)
LIBS_DIR = PathManager.get_libs_path()
PRODUCT_TEST_ROOT_DIRECTORY =  PathManager.get_testUtils_dir()

SUPPORT_FILE_PATH = PathManager.get_support_file_path()
REAL_WAREHOUSE_DIRECTORY = os.path.join(SUPPORT_FILE_PATH, "CentralXml", "RealWarehousePolicies")
TEMPLATE_FILE_DIRECTORY = os.path.join(REAL_WAREHOUSE_DIRECTORY, "templates")
GENERATED_FILE_DIRECTORY = os.path.join(REAL_WAREHOUSE_DIRECTORY, "GeneratedAlcPolicies")

FILER_6_DIRECTORY = os.path.join("/", "mnt", "filer6", "bfr", "sspl-warehouse")
LOCAL_WAREHOUSES_ROOT = os.path.join(PRODUCT_TEST_ROOT_DIRECTORY, "local_warehouses")
LOCAL_WAREHOUSES = os.path.join(LOCAL_WAREHOUSES_ROOT, "dev", "sspl-warehouse")
WAREHOUSE_LOCAL_SERVER_PORT = 443
# newline is important in the redirect below
OSTIA_HOST_REDIRECT = """127.0.0.1  ostia.eng.sophos
"""
OSTIA_HOSTS_BACKUP_FILENAME="ostia_hosts.bk"

USERNAME = "username"
PASSWORD = "password"
ENV_KEY = "env_key"

BUILD_TYPE = "build_type"
PROD_BUILD_CERTS = "prod"
DEV_BUILD_CERTS = "dev"

# WARNING: These overrides will not work with locally on pairing station. If you want to use them locally on vagrant
# You must run the robot command from inside the vagrant box
# The version under test, usually master dev builds
OSTIA_VUT_ADDRESS_BRANCH_OVERRIDE = "OSTIA_VUT_OVERRIDE"
OSTIA_VUT_ADDRESS_BRANCH = os.environ.get(OSTIA_VUT_ADDRESS_BRANCH_OVERRIDE, "master")
OSTIA_VUT_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/{}".format(OSTIA_VUT_ADDRESS_BRANCH)
# Usually the previous release
OSTIA_PREV_ADDRESS_BRANCH_OVERRIDE = "OSTIA_PREV_OVERRIDE"
OSTIA_PREV_ADDRESS_BRANCH = os.environ.get(OSTIA_PREV_ADDRESS_BRANCH_OVERRIDE, "feature-prod-warehouse")
OSTIA_PREV_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/{}".format(OSTIA_PREV_ADDRESS_BRANCH)
# The GA Release
OSTIA_GA_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/feature-GA-milestone"
# A version with mocked libraries (to test file removal on upgrade)
OSTIA_0_6_0_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/feature-version-0-6-0-warehouse"
# A warehouse containing 3 base versions for paused updating tests
OSTIA_PAUSED_ADDRESS_BRANCH_OVERRIDE = "OSTIA_PAUSED_OVERRIDE"
OSTIA_PAUSED_ADDRESS_BRANCH = os.environ.get(OSTIA_VUT_ADDRESS_BRANCH_OVERRIDE, "feature-paused-updating")
OSTIA_PAUSED_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/{}".format(OSTIA_PAUSED_ADDRESS_BRANCH)
# dictionary of ostia addresses against the ports that should be used to serve their customer files locally
OSTIA_ADDRESSES = {
                    OSTIA_VUT_ADDRESS: "2233",
                    OSTIA_PREV_ADDRESS: "3233",
                    OSTIA_0_6_0_ADDRESS: "4233",
                    OSTIA_GA_ADDRESS: "5233",
                    OSTIA_PAUSED_ADDRESS: "6233"
                   }

BALLISTA_ADDRESS = "https://dci.sophosupd.com/cloudupdate"

DEV_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "rootca.crt")
DEV_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "ps_rootca.crt")
PROD_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "rootca.crt")
PROD_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "ps_rootca.crt")

ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "certs", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "certs", "ps_rootca.crt")
SOPHOS_ALIAS_EXTENSION = os.path.join("base", "update", "var", "sophos_alias.txt")

def _make_local_copy_of_warehouse():
    branch_directories = []
    for address in OSTIA_ADDRESSES:
        branch_name = os.path.basename(address)
        branch_directories.append(branch_name)

        filer_branch_path = os.path.join(FILER_6_DIRECTORY, branch_name)
        local_branch_path = os.path.join(LOCAL_WAREHOUSES, branch_name)
        os.makedirs(local_branch_path)
        logger.info("made branch path")
        with open(os.path.join(filer_branch_path, "lastgoodbuild.txt"), "r") as last_good_build:
            latest_good_build = last_good_build.read()
            latest_path = os.path.join(filer_branch_path, latest_good_build, "warehouse", "1.0.0")
            local_destination_path = os.path.join(local_branch_path, latest_good_build)
            shutil.copytree(os.path.join(latest_path, "customer"), os.path.join(local_destination_path, "customer"))
            shutil.copytree(os.path.join(latest_path, "warehouse"), os.path.join(local_destination_path, "warehouse"))

def _cleanup_local_warehouses():
    if os.path.isdir(LOCAL_WAREHOUSES):
        shutil.rmtree(LOCAL_WAREHOUSES_ROOT)

def _get_sophos_install_path():
    sophos_install_path = BuiltIn().get_variable_value("${SOPHOS_INSTALL}", "/opt/sophos-spl")
    if not os.path.isdir(sophos_install_path):
        raise OSError("sophos install path: \"{}\", does not exist".format(sophos_install_path))
    return sophos_install_path


def _install_upgrade_certs(root_ca, ps_root_ca):
    sophos_install_path = _get_sophos_install_path()
    logger.info("root_ca:  {}".format(root_ca))
    logger.info("ps_root_ca:  {}".format(ps_root_ca))
    shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION))
    shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION))

def _install_sophos_alias_file(url):
    sophos_install_path = _get_sophos_install_path()
    sophos_alias_file_path = os.path.join(sophos_install_path, SOPHOS_ALIAS_EXTENSION)
    base_update_var_directory = os.path.dirname(sophos_alias_file_path)
    logger.info("Sophos alias URL: {}".format(url))
    if not os.path.isdir(base_update_var_directory):
        raise OSError("cannot create sophos alias file as \"{}\" does not exist".format(base_update_var_directory))
    with open(sophos_alias_file_path, "w") as sophos_alias_file:
        sophos_alias_file.write(url)
        logger.info("wrote alias '{}' to '{}'".format(url, sophos_alias_file_path))

def _remove_sophos_alias_file():
    sophos_install_path = _get_sophos_install_path()
    sophos_alias_file_path = os.path.join(sophos_install_path, SOPHOS_ALIAS_EXTENSION)
    os.remove(sophos_alias_file_path)


def calculate_hashed_creds(username, password):
    credentials_as_string = "{}:{}".format(username, password)
    hash = hashlib.md5(credentials_as_string.encode("utf-8"))
    logger.info(credentials_as_string)
    return hash.hexdigest()


class TemplateConfig:

    use_local_warehouses = os.path.isdir(LOCAL_WAREHOUSES)

    def __init__(self, env_key, username, build_type, ostia_adress):
        """
        :param env_key: environment variable key used to get overrides
        :param username: username for the warehouse this policy is made for
        :param build_type: build type (prod or dev) of the products in the warehouse (needed for cert disambiguation)
        """
        self.env_key = env_key
        environment_config = os.environ.get(env_key, None)
        if environment_config:
            # assume ballista warehouse if override given
            env_values = environment_config.split(":")
            self.username = env_values[0]
            self.password = env_values[1]
            self.remote_connection_address = BALLISTA_ADDRESS
            self.build_type = PROD_BUILD_CERTS
        else:
            self.username = username
            self.password = PASSWORD
            self.remote_connection_address = ostia_adress
            self.build_type = build_type
        self._define_valid_update_certs()
        self._define_hashed_creds()
        self.local_connection_address = None
        self._set_local_connection_address_to_use_local_customer_address_if_using_local_ostia_warehouses()
        self._set_customer_file_domain()
        self._set_warehouse_domain()
        self._validate_values()

    def _set_customer_file_domain(self):
        if "localhost" in self.get_connection_address():
            self.customer_file_domain = "localhost:{}".format(self.local_customer_file_port)
        elif "ostia" in self.get_connection_address():
            self.customer_file_domain = "ostia.eng.sophos:443"
        else:
            self.customer_file_domain = "dci.sophosupd.com:443"

    def _set_warehouse_domain(self):
        if "localhost" in self.get_connection_address() or "ostia" in self.get_connection_address():
            self.warehouse_domain = "ostia.eng.sophos:443"
        else:
            self.warehouse_domain = "*.sophosupd.*:443"

    def _set_local_connection_address_to_use_local_customer_address_if_using_local_ostia_warehouses(self):
        # If we have local copies of the ostia warehouses, we want to set the connection adress to use
        # the localhost:2233 address which customer file update servers will use
        if self.use_local_warehouses:
            self.local_customer_file_port = OSTIA_ADDRESSES[self.remote_connection_address]
            self.local_connection_address = "https://localhost:{}".format(self.local_customer_file_port)
            logger.info("setting {} local connection address to {} as local warehouse directory was found".format(
                self.remote_connection_address, self.local_connection_address
            ))

    def get_connection_address(self):
        if self.local_connection_address:
            logger.info("returning '{}' as connection address".format(self.local_connection_address))
            return self.local_connection_address
        else:
            logger.info("returning '{}' as connection address".format(self.remote_connection_address))
            return self.remote_connection_address

    def _validate_values(self):
        if self.build_type not in [PROD_BUILD_CERTS, DEV_BUILD_CERTS]:
            raise ValueError("Build type override for {} is invalid, should be {} or {}, was {}".format(
                             self.env_key, PROD_BUILD_CERTS, DEV_BUILD_CERTS, self.build_type))

    def _define_hashed_creds(self):
        self.hashed_credentials = calculate_hashed_creds(self.username, self.password)
        logger.info(self.hashed_credentials)

    def _define_valid_update_certs(self):

        if self.build_type == DEV_BUILD_CERTS:
            self.root_ca = DEV_ROOT_CA
        else:
            self.root_ca = PROD_ROOT_CA

        if self.remote_connection_address in OSTIA_ADDRESSES:
            self.ps_root_ca = DEV_PS_ROOT_CA
        else:
            self.ps_root_ca = PROD_PS_ROOT_CA

    def install_upgrade_certs(self, root_ca=None, ps_root_ca=None):
        if not root_ca:
            root_ca = self.root_ca
        if not ps_root_ca:
            ps_root_ca = self.ps_root_ca

        _install_upgrade_certs(root_ca, ps_root_ca)

    def generate_warehouse_policy_from_template(self, template_file_name, proposed_output_path = None):
        """
        :param template_file_name: file name for the ALC policy which will be used to generate a new policy file
        based on data stored in the  template_configuration_values dictionary

        """
        self.policy_file_name = template_file_name
        template_policy = os.path.join(TEMPLATE_FILE_DIRECTORY, template_file_name)
        if proposed_output_path is None:
            output_policy = os.path.join(GENERATED_FILE_DIRECTORY, template_file_name)
        else:
            logger.info("ALC policy will be placed at: {}".format(proposed_output_path))
            output_policy = proposed_output_path

        if not os.path.isfile(template_policy):
            raise OSError("{} does not exist".format(template_policy))

        password_marker = "@PASSWORD@"
        username_marker = "@USERNAME@"
        connection_address_marker = "@CONNECTIONADDRESS@"

        with open(template_policy) as template_file:
            template_string = template_file.read()
            template_string_with_replaced_values = template_string.replace(password_marker, self.password) \
                .replace(username_marker, self.username) \
                .replace(connection_address_marker, self.get_connection_address())
            with open(output_policy, "w+") as output_file:  # replaces existing file if exists
                output_file.write(template_string_with_replaced_values)
                logger.info(
                    """
                    Wrote real warehouse policy \"{}\" with:
                    username: {}
                    password: {}
                    connection address: {}
                    full contents:
                    {}
                    
                    """.format(self.policy_file_name, self.username, self.password, self.get_connection_address(), template_string_with_replaced_values)
                )
    def install_sophos_alias_file(self):
        _install_sophos_alias_file(self.get_connection_address())

    def install_sophos_alias_file_if_using_ostia(self):
        if self.remote_connection_address in OSTIA_ADDRESSES:
            self.install_sophos_alias_file()


class WarehouseUtils(object):
    """
    Class to setup ALC Policy files used in tests.
    """

    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    ROBOT_LISTENER_API_VERSION = 2

    template_configuration_values = {
        "base_and_mtr_0_6_0.xml": TemplateConfig("BASE_AND_MTR_0_6_0", "mtr_user_0_6_0", DEV_BUILD_CERTS, OSTIA_0_6_0_ADDRESS),
        "base_and_mtr_VUT.xml": TemplateConfig("BASE_AND_MTR_VUT", "mtr_user_vut", DEV_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_and_mtr_VUT-1.xml": TemplateConfig("BASE_AND_MTR_VUT_PREV", "mtr_user_vut-1", DEV_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_and_mtr_GA.xml": TemplateConfig("BASE_AND_MTR_GA", "ga_mtr_user", DEV_BUILD_CERTS, OSTIA_GA_ADDRESS),
        "base_and_edr_VUT.xml": TemplateConfig("BASE_ONLY_VUT", "base_user_vut", DEV_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_0_6_0.xml": TemplateConfig("BASE_ONLY_0_6_0", "base_user_0_6_0", DEV_BUILD_CERTS, OSTIA_0_6_0_ADDRESS),
        "base_only_VUT.xml": TemplateConfig("BASE_ONLY_VUT", "base_user_vut", DEV_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_VUT-1.xml": TemplateConfig("BASE_ONLY_VUT_PREV", "base_user_vut-1", DEV_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_only_GA.xml": TemplateConfig("BASE_ONLY_GA", "ga_base_user", DEV_BUILD_CERTS, OSTIA_GA_ADDRESS),
        "base_VUT_and_fake_plugins.xml": TemplateConfig("BASE_VUT_AND_FAKE_PLUGINS", "fake_plugin_user", DEV_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_paused_update_VUT-1.xml": TemplateConfig("BASE_PAUSED_VUT_PREV", "base_user_paused", DEV_BUILD_CERTS, OSTIA_PAUSED_ADDRESS),
        "base_paused_update_VUT.xml": TemplateConfig("BASE_PAUSED_VUT", "base_user_paused", DEV_BUILD_CERTS, OSTIA_PAUSED_ADDRESS),
        "base_paused_update_999.xml": TemplateConfig("BASE_PAUSED_999", "base_user_paused", DEV_BUILD_CERTS, OSTIA_PAUSED_ADDRESS),
    }

    update_server = UpdateServer.UpdateServer("ostia_update_server.log")

    def _get_template_config_from_dictionary_using_filename(self, policy_filename):
        template_config = self.template_configuration_values.get(policy_filename, None)
        if template_config:
            return template_config
        else:
            raise KeyError("{} not found in template_configuration_values dictionary".format(policy_filename))

    def _get_template_config_from_dictionary_using_path(self, policy_file_path):
        if not os.path.isfile(policy_file_path):
            raise OSError("{} is not a file".format(policy_file_path))
        file_name = os.path.basename(policy_file_path)
        template_config = self._get_template_config_from_dictionary_using_filename(file_name)
        return template_config

    def get_template_config(self, template_file_name):
        """
        :param template_file_name: file name for template.
        :return: template_configuration object
        """
        template_config = self._get_template_config_from_dictionary_using_filename(template_file_name)

        return template_config

    def generate_real_warehouse_alc_files(self):
        """
        Entry function used to generate the set of ALC policy files used for end-to-end testing with
        Real production or Ostia warehouses.
        """

        for file_name, template_config in list(self.template_configuration_values.items()):
            template_config.generate_warehouse_policy_from_template(file_name)

    def generate_local_warehouses(self):
        _make_local_copy_of_warehouse()

    def cleanup_local_warehouses(self):
        _cleanup_local_warehouses()

    def install_upgrade_certs_for_policy(self, policy_path, root_ca=None, ps_root_ca=None):
        template_config = self._get_template_config_from_dictionary_using_path(policy_path)
        if not root_ca:
            root_ca = template_config.root_ca
        if not ps_root_ca:
            ps_root_ca = template_config.ps_root_ca
        template_config.install_upgrade_certs(root_ca, ps_root_ca)

    def install_upgrade_certs(self, root_ca, ps_root_ca):
        _install_upgrade_certs(root_ca, ps_root_ca)

    def install_sophos_alias_file_for_policy(self, policy_path):
        """
        installs a sophos alias file to <spl-installation>/base/etc/update/var/sophos_alias.txt which will redirect
        suldownloader to a url.

        :param policy_path: Policy to infer the redirect url from
        :return:
        """
        template_config = self._get_template_config_from_dictionary_using_path(policy_path)
        template_config.install_sophos_alias_file()

    def install_sophos_alias_file_for_policy_if_using_ostia(self, policy_path):
        """
        installs a sophos alias file to <spl-installation>/base/etc/update/var/sophos_alias.txt which will redirect
        suldownloader to a url.

        :param policy_path: Policy to infer the redirect url from
        :return:
        """
        template_config = self._get_template_config_from_dictionary_using_path(policy_path)
        template_config.install_sophos_alias_file_if_using_ostia()

    def install_sophos_alias_file(self, url):
        """
        installs a sophos alias file to <spl-installation>/base/etc/update/var/sophos_alias.txt which will redirect
        suldownloader.

        :param url: url to write to the alias file
        :return: None
        """
        _install_sophos_alias_file(url)

    def remove_sophos_alias_file(self):
        """
        removes the sophos alias file from the installation
        :return: None
        """
        _remove_sophos_alias_file()


    def get_customer_file_domain_for_policy(self, policy_path):
        template_config = self._get_template_config_from_dictionary_using_path(policy_path)
        return template_config.customer_file_domain

    def get_warehouse_domain_for_policy(self, policy_path):
        template_config = self._get_template_config_from_dictionary_using_path(policy_path)
        return template_config.warehouse_domain

    def stop_local_ostia_servers(self):
        self.update_server.stop_update_server()

    def restore_host_file_after_using_local_ostia_warehouses(self):
        if os.path.isfile(os.path.join("/etc", OSTIA_HOSTS_BACKUP_FILENAME)):
            self.update_server.restore_host_file(backup_filename=OSTIA_HOSTS_BACKUP_FILENAME)

    def modify_host_file_for_local_ostia_warehouses(self):
        self.update_server.modify_host_file_for_local_updating(new_hosts_file_content=OSTIA_HOST_REDIRECT, backup_filename=OSTIA_HOSTS_BACKUP_FILENAME)

    def start_all_local_update_servers(self):
        self.update_server.stop_update_server()

        for address, port in list(OSTIA_ADDRESSES.items()):
            branch_name = os.path.basename(address)
            local_warehouse_branch = os.path.join(LOCAL_WAREHOUSES, branch_name)
            list_of_files = os.listdir(local_warehouse_branch)
            logger.info(list_of_files)
            assert len(list_of_files) == 1, "Error, couldn't get local warehouse timestamp for {}".format(address)
            timestamp = list_of_files[0]
            customer_directory = os.path.join(local_warehouse_branch, timestamp, "customer")

            self.update_server.start_update_server(port, customer_directory)
            logger.info("started local warehouse customer file server on localhost:{} for {}".format(port, branch_name))
            self.update_server.can_curl_url("https://localhost:{}".format(port))

        self.update_server.start_update_server(WAREHOUSE_LOCAL_SERVER_PORT, LOCAL_WAREHOUSES_ROOT)
        logger.info("started local warehouse catalogue server on localhost:{}".format(WAREHOUSE_LOCAL_SERVER_PORT))
        self.update_server.can_curl_url("https://localhost:{}".format(WAREHOUSE_LOCAL_SERVER_PORT))

    def setup_local_warehouses_if_needed(self):
        if os.path.isdir(LOCAL_WAREHOUSES):
            self.start_all_local_update_servers()
            self.modify_host_file_for_local_ostia_warehouses()

    def generate_local_ssl_certs_if_they_dont_exist(self):
        server_https_cert = os.path.join(SUPPORT_FILE_PATH, "https", "ca", "root-ca.crt.pem")
        if not os.path.isfile(server_https_cert):
            self.update_server.generate_update_certs()

    def create_alc_policy_for_warehouse_credentials(self, template_name, username, target_output_file):
        templ = TemplateConfig('HopefullyNoEnvironmentVariableWillHaveThisName',
                               username, PROD_BUILD_CERTS, BALLISTA_ADDRESS)
        templ.generate_warehouse_policy_from_template(template_name, target_output_file)


# If ran directly, file sets up local warehouse directory from filer6
if __name__ == "__main__":
    _make_local_copy_of_warehouse()