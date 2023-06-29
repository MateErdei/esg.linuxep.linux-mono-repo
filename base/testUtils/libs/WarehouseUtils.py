#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime
import hashlib
import os
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET

import requests
import robot.api.logger as logger
from packaging import version
from robot.libraries.BuiltIn import BuiltIn

import PathManager
import UpdateServer

THIS_FILE = os.path.realpath(__file__)
LIBS_DIR = PathManager.get_libs_path()
PRODUCT_TEST_ROOT_DIRECTORY = PathManager.get_testUtils_dir()

SUPPORT_FILE_PATH = PathManager.get_support_file_path()
REAL_WAREHOUSE_DIRECTORY = os.path.join(SUPPORT_FILE_PATH, "CentralXml", "RealWarehousePolicies")
TEMPLATE_FILE_DIRECTORY = os.path.join(REAL_WAREHOUSE_DIRECTORY, "templates")
GENERATED_FILE_DIRECTORY = os.path.join(REAL_WAREHOUSE_DIRECTORY, "GeneratedAlcPolicies")

SYSTEMPRODUCT_TEST_INPUT = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")
LOCAL_WAREHOUSES_ROOT = os.path.join(SYSTEMPRODUCT_TEST_INPUT, "local_warehouses")
LOCAL_WAREHOUSES = os.path.join(LOCAL_WAREHOUSES_ROOT, "dev", "sspl-warehouse")
WAREHOUSE_LOCAL_SERVER_PORT = 443

USERNAME = "username"
PASSWORD = "password"
ENV_KEY = "env_key"

BUILD_TYPE = "build_type"
PROD_BUILD_CERTS = "prod"
DEV_BUILD_CERTS = "dev"

# The version under test, usually master dev builds
OSTIA_VUT_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/vut"

# a version with a different query pack than vut
OSTIA_QUERY_PACK_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/query-pack"
# a version with edr 9.99.9 for downgrade tests
OSTIA_EDR_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/edr-999"
OSTIA_BASE_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/base-999"
OSTIA_MTR_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/mdr-999"
OSTIA_EDR_AND_MTR_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/edr-mdr-999"

# dictionary of ostia addresses against the ports that should be used to serve their customer files locally
OSTIA_ADDRESSES = {
    OSTIA_VUT_ADDRESS: "2233",
    OSTIA_EDR_999_ADDRESS: "7233",
    OSTIA_BASE_999_ADDRESS: "7235",
    OSTIA_QUERY_PACK_ADDRESS: "7239",
    OSTIA_MTR_999_ADDRESS: "7237",
    OSTIA_EDR_AND_MTR_999_ADDRESS: "7240"
}

BALLISTA_ADDRESS = "https://dci.sophosupd.com/cloudupdate"
INTERNAL_MIRROR_BALLISTA_ADDRESS = "https://dci.sophosupd.net/sprints"
ADDRESS_DICT = {
    "external": BALLISTA_ADDRESS,
    "internal": INTERNAL_MIRROR_BALLISTA_ADDRESS
}

DEV_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "rootca.crt")
DEV_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "ps_rootca.crt")
PROD_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "rootca.crt")
PROD_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "ps_rootca.crt")

ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "rootcerts", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "rootcerts", "ps_rootca.crt")
ROOT_CA_INSTALLATION_EXTENSION_OLD = os.path.join("base", "update", "certs", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION_OLD = os.path.join("base", "update", "certs", "ps_rootca.crt")

SOPHOS_ALIAS_EXTENSION = os.path.join("base", "update", "var", "sophos_alias.txt")


def _get_sophos_install_path():
    sophos_install_path = BuiltIn().get_variable_value("${SOPHOS_INSTALL}", "/opt/sophos-spl")
    if not os.path.isdir(sophos_install_path):
        raise OSError(f"sophos install path: \"{sophos_install_path}\", does not exist")
    return sophos_install_path


def _install_upgrade_certs(root_ca, ps_root_ca):
    sophos_install_path = _get_sophos_install_path()
    logger.info(f"root_ca:  {root_ca}")
    logger.info(f"ps_root_ca:  {ps_root_ca}")

    try:
        shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION))
        shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION))
    except:
        shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION_OLD))
        shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION_OLD))


def calculate_hashed_creds(username, password):
    credentials_as_string = f"{username}:{password}"
    hash = hashlib.md5(credentials_as_string.encode("utf-8"))
    logger.info(credentials_as_string)
    return hash.hexdigest()


def get_yesterday():
    now = datetime.date.today()
    yesterday = now - datetime.timedelta(days=1)
    return yesterday.strftime("%A")  # Returns day as week day name


def get_importrefrence_for_component_with_tag(rigid_name, tag, pubspec):
    line = list(filter(lambda n: rigid_name == n.attrib["id"], pubspec.findall("./warehouses//line")))[0]
    for component in line.findall("./component"):
        for release_tag in component.findall("./releasetag"):
            if release_tag.attrib["tag"] == tag:
                return component.attrib["importreference"]
    raise AssertionError(f"Did not find {rigid_name} in {pubspec}")


def get_importrefrence_for_component_with_tag_from_componentsuite(rigid_name, componentsuite_rigid_name, tag, pubspec):
    componentsuite_importref = get_importrefrence_for_component_with_tag(componentsuite_rigid_name, tag, pubspec)

    line = \
        list(filter(lambda n: componentsuite_rigid_name == n.attrib["id"], pubspec.findall("./componentsuites/line")))[0]
    componentsuite = \
        list(filter(lambda n: componentsuite_importref == n.attrib["importreference"], line.findall("./componentsuite")))[0]
    component = list(filter(lambda n: rigid_name == n.attrib["line"], componentsuite.findall("./component")))[0]
    return component.attrib["importreference"]


def get_version_for_component_with_importref(rigid_name, importref, importspec):
    line = list(filter(lambda n: rigid_name == n.attrib["id"], importspec.findall("./imports/line")))[0]
    for component in line.findall("./"):
        if component.attrib["id"] == importref:
            return component.attrib["version"]
    raise AssertionError(f"Did not find {importref} in {importspec}")


def get_version_for_rigidname(rigid_name, tag="RECOMMENDED"):
    importref = get_importrefrence_for_component_with_tag(rigid_name, tag)
    version = get_version_for_component_with_importref(rigid_name, importref)
    return version


def get_dci_xml_from_update_credentials_inner(dci_address):
    r = requests.get(dci_address)
    signature_start = r.content.find(b"-----BEGIN SIGNATURE-----")
    xml_string = r.content[:signature_start].decode()
    return xml_string


def get_dci_xml_from_update_credentials(dci_address):
    error_message = None
    for i in range(10):
        try:
            return get_dci_xml_from_update_credentials_inner(dci_address)
        except Exception as reason:
            error_message = reason
    else:
        logger.error(f"Failed to get dci xml from update credentials for {dci_address}: {error_message}")


def get_sdds_names_from_dci_xml_string(dci_xml_string):
    root = ET.fromstring(dci_xml_string)
    warehouse_entries = root.findall("./Warehouses/WarehouseEntry/Name")
    sdds_names = []
    for warehouse_entry in warehouse_entries:
        sdds_names.append(warehouse_entry.text)
    return sdds_names


def get_sdds_names_from_update_credentials(dci_address):
    xml = get_dci_xml_from_update_credentials(dci_address)
    if not xml:
        return []
    return get_sdds_names_from_dci_xml_string(xml)


importspec = "importspec"
pubspec = "publicationspec"


def get_spec_type_from_spec(spec):
    root_tag = spec.find(".").tag
    if root_tag == importspec:
        return importspec
    elif root_tag == pubspec:
        return pubspec
    else:
        raise AssertionError(f"expected {root_tag} to be either {importspec} or {pubspec}")


def get_version_from_sdds_import_file(path):
    with open(path) as file:
        contents = file.read()
        xml = ET.fromstring(contents)
        return xml.find(".//Version").text


def get_version_of_component_with_tag_from_spec_xml(rigidname, tag, spec_xml_dict, relevant_sdds_names):
    for sdds_name in relevant_sdds_names:
        # print(sdds_name)
        try:
            if spec_xml_dict.get(sdds_name, None):
                importref = get_importrefrence_for_component_with_tag(rigidname, tag, spec_xml_dict[sdds_name][pubspec])
                version = get_version_for_component_with_importref(rigidname, importref,
                                                                   spec_xml_dict[sdds_name][importspec])
                return version
        except:
            pass
    else:
        # print(relevant_sdds_names)
        # print(spec_xml_dict.keys())
        raise AssertionError(f"Did not find {rigidname} in {spec_xml_dict}")


class TemplateConfig:
    use_local_warehouses = os.path.isdir(LOCAL_WAREHOUSES)

    def __init__(self, env_key, username, build_type, ostia_adress):
        """
        :param env_key: environment variable key used to get overrides
        :param username: username for the warehouse this policy is made for
        :param build_type: build type (prod or dev) of the products in the warehouse (needed for cert disambiguation)
        """
        self.local_connection_address = None
        self.env_key = env_key
        environment_config = os.environ.get(env_key, None)
        if environment_config:
            # assume ballista warehouse if override given
            user_pass = environment_config.split(":")
            self.username = user_pass[0]
            self.password = user_pass[1]
            self.hashed_credentials = user_pass[2]
            if len(user_pass) == 4:
                self.remote_connection_address = ADDRESS_DICT[user_pass[3]]
            else:
                self.remote_connection_address = BALLISTA_ADDRESS
            dci_address = f"{self.remote_connection_address}/{self.hashed_credentials[0]}/{self.hashed_credentials[1:3]}/{self.hashed_credentials}.dat"
            self.relevent_sdds_files = get_sdds_names_from_update_credentials(dci_address)
            self.build_type = PROD_BUILD_CERTS
            self.algorithm = "AES256"
        else:
            self.username = username
            self.password = PASSWORD
            self.remote_connection_address = ostia_adress
            self.build_type = build_type
            self.algorithm = "Clear"
            self._define_hashed_creds()
            self._set_local_connection_address_to_use_local_customer_address_if_using_local_ostia_warehouses()
        self._define_valid_update_certs()
        self._set_customer_file_domain()
        self._set_warehouse_domain()
        self._validate_values()

    def _set_customer_file_domain(self):
        if "localhost" in self.get_connection_address():
            self.customer_file_domain = f"localhost:{self.local_customer_file_port}"
        elif "ostia" in self.get_connection_address():
            self.customer_file_domain = "ostia.eng.sophos:443"
        else:
            self.customer_file_domain = "dci.sophosupd.com:443"

    def _set_warehouse_domain(self):
        if "localhost" in self.get_connection_address() or "ostia" in self.get_connection_address():
            self.warehouse_domain = "ostia.eng.sophos:443"
        else:
            self.warehouse_domain = "d1.sophosupd.com:443"

    def _set_local_connection_address_to_use_local_customer_address_if_using_local_ostia_warehouses(self):
        # If we have local copies of the ostia warehouses, we want to set the connection address to use
        # the localhost:2233 address which customer file update servers will use
        # don't do this if using a ballista override
        if self.use_local_warehouses and self.remote_connection_address not in ADDRESS_DICT.values():
            self.local_customer_file_port = OSTIA_ADDRESSES[self.remote_connection_address]
            self.local_connection_address = f"https://localhost:{self.local_customer_file_port}"
            logger.info(f"setting {self.remote_connection_address} local connection address to {self.local_connection_address} as local warehouse directory was found")

    def get_connection_address(self):
        if self.local_connection_address:
            logger.info(f"returning '{self.local_connection_address}' as connection address")
            return self.local_connection_address
        else:
            logger.info(f"returning '{self.remote_connection_address}' as connection address")
            return self.remote_connection_address

    def _validate_values(self):
        if self.build_type not in [PROD_BUILD_CERTS, DEV_BUILD_CERTS]:
            raise ValueError(f"Build type override for {self.env_key} is invalid, should be {PROD_BUILD_CERTS} or {DEV_BUILD_CERTS}, was {self.build_type}")

    def _define_hashed_creds(self):
        self.hashed_credentials = calculate_hashed_creds(self.username, self.password)
        logger.info(self.hashed_credentials)

    def _define_valid_update_certs(self):

        if self.build_type == DEV_BUILD_CERTS:
            self.root_ca = DEV_ROOT_CA
        else:
            self.root_ca = PROD_ROOT_CA

        if self.remote_connection_address in OSTIA_ADDRESSES:
            self.ps_root_ca = PROD_PS_ROOT_CA
            self.thininstaller_cert = PROD_PS_ROOT_CA
        else:
            # we take the dirname of this, which is why there is a slash
            self.thininstaller_cert = "system/"
            self.ps_root_ca = PROD_PS_ROOT_CA

    def install_upgrade_certs(self, root_ca=None, ps_root_ca=None):
        if not root_ca:
            root_ca = self.root_ca
        if not ps_root_ca:
            ps_root_ca = self.ps_root_ca

        _install_upgrade_certs(root_ca, ps_root_ca)

    def generate_warehouse_policy_from_template(self, template_file_name, proposed_output_path=None):
        """
        :param template_file_name: file name for the ALC policy which will be used to generate a new policy file
        based on data stored in the  template_configuration_values dictionary
        :return path of output policy

        """
        self.policy_file_name = template_file_name
        template_policy = os.path.join(TEMPLATE_FILE_DIRECTORY, template_file_name)
        if proposed_output_path is None:
            output_policy = os.path.join(GENERATED_FILE_DIRECTORY, template_file_name)
        else:
            logger.info(f"ALC policy will be placed at: {proposed_output_path}")
            output_policy = proposed_output_path

        if not os.path.isfile(template_policy):
            raise OSError(f"{template_policy} does not exist")

        password_marker = "@PASSWORD@"
        username_marker = "@USERNAME@"
        connection_address_marker = "@CONNECTIONADDRESS@"
        algorithm_marker = "@ALGORITHM@"

        with open(template_policy) as template_file:
            template_string = template_file.read()
            template_string_with_replaced_values = template_string.replace(password_marker, self.password) \
                .replace(username_marker, self.username) \
                .replace(connection_address_marker, self.get_connection_address()) \
                .replace(algorithm_marker, self.algorithm)
            with open(output_policy, "w+") as output_file:  # replaces existing file if exists
                output_file.write(template_string_with_replaced_values)
                logger.info(
                    f"""
                    Wrote real warehouse policy \"{self.policy_file_name}\" with:
                    username: {self.username}
                    password: {self.password}
                    connection address: {self.get_connection_address()}
                    full contents:
                    {template_string_with_replaced_values}
                    
                    """
                )
        return output_policy


class WarehouseUtils(object):
    """
    Class to setup ALC Policy files used in tests.
    """

    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    ROBOT_LISTENER_API_VERSION = 2
    os.environ[
        'VUT_PREV_RELEASE'] = "CSP7I0S0GZZE:CCADasoUC50JcnRFdhJR+mhonNzZ872yyT0W8e2/3dGohT2bPmkQy/baXddi+RzbTxg=:db55fcf8898da2b3f3c06f26e9246cbb"
    os.environ[
        'VUT_PREV_DOGFOOD'] = "QA767596:CCCirMa73nCbF+rU9aHeyqCasmwZR9GpWyPav3N0ONhr56KqcJR8L7OdlrmdHJLXc08=:105f4d87e14c91142561bf3d022e55b9"

    template_configuration_values = {
        "mtr_edr_vut_and_base_999.xml": TemplateConfig("BASE_999", "mtr_user_vut", PROD_BUILD_CERTS,
                                                       OSTIA_BASE_999_ADDRESS),
        "old_query_pack.xml": TemplateConfig("QUERY_PACK", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_QUERY_PACK_ADDRESS),
        "base_mtr_vut_and_edr_999.xml": TemplateConfig("BASE_MTR_AND_EDR_999", "user_mtr_vut_edr_999", PROD_BUILD_CERTS,
                                                       OSTIA_EDR_999_ADDRESS),
        "base_vut_and_mtr_edr_999.xml": TemplateConfig("BASE_AND_MTR_EDR_999", "mtr_and_edr_user_999", PROD_BUILD_CERTS,
                                                       OSTIA_EDR_AND_MTR_999_ADDRESS),
        "base_vut_and_mtr_edr_av_999.xml": TemplateConfig("BASE_AND_MTR_EDR_AV_999", "av_user_999", PROD_BUILD_CERTS,
                                                          OSTIA_EDR_AND_MTR_999_ADDRESS),
        "base_and_mtr_VUT.xml": TemplateConfig("BALLISTA_VUT", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_edr_and_mtr_and_av_VUT.xml": TemplateConfig("BALLISTA_VUT", "av_user_vut", PROD_BUILD_CERTS,
                                                          OSTIA_VUT_ADDRESS),
        # we are overriding the two warehouses below to use ballista warehouse,
        # if we want to switch them back to ostia the address has to change to the new warehouses location
        "base_edr_and_mtr_and_av_VUT-1.xml": TemplateConfig("VUT_PREV_DOGFOOD", "av_user_vut", PROD_BUILD_CERTS,
                                                            OSTIA_VUT_ADDRESS),
        "base_edr_and_mtr_and_av_Release.xml": TemplateConfig("VUT_PREV_RELEASE", "av_user_vut", PROD_BUILD_CERTS,
                                                              OSTIA_VUT_ADDRESS),
        "base_and_edr_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_edr_and_mtr.xml": TemplateConfig("BALLISTA_VUT", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_fixed_version_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS,
                                                          OSTIA_VUT_ADDRESS),
        "base_only_fixed_version_999.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS,
                                                          OSTIA_VUT_ADDRESS)
    }

    RIGIDNAMES_AGAINST_PRODUCT_NAMES_IN_VERSION_INI_FILES = {
        "ServerProtectionLinux-Plugin-AV": "SPL-Anti-Virus-Plugin",
        "ServerProtectionLinux-Plugin-liveresponse": "SPL-Live-Response-Plugin",
        "ServerProtectionLinux-Plugin-MDR": "SPL-Managed-Threat-Response-Plugin",
        "ServerProtectionLinux-Plugin-EventJournaler": "SPL-Event-Journaler-Plugin",
        "ServerProtectionLinux-Base-component": "SPL-Base-Component",
        "ServerProtectionLinux-Plugin-EDR": "SPL-Endpoint-Detection-and-Response-Plugin",
        "ServerProtectionLinux-Plugin-RuntimeDetections": "SPL-Runtime-Detection-Plugin",
        "ServerProtectionLinux-Plugin-Heartbeat": "Sophos Server Protection Linux - Heartbeat",
    }

    update_server = UpdateServer.UpdateServer("ostia_update_server.log")

    def _get_template_config_from_dictionary_using_filename(self, policy_filename):
        template_config = self.template_configuration_values.get(policy_filename, None)
        if template_config:
            return template_config
        else:
            raise KeyError(f"{policy_filename} not found in template_configuration_values dictionary")

    def _get_template_config_from_dictionary_using_path(self, policy_file_path):
        if not os.path.isfile(policy_file_path):
            raise OSError(f"{policy_file_path} is not a file")
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


    def install_upgrade_certs_for_policy(self, policy_path, root_ca=None, ps_root_ca=None):
        template_config = self._get_template_config_from_dictionary_using_path(policy_path)
        if not root_ca:
            root_ca = template_config.root_ca
        if not ps_root_ca:
            ps_root_ca = template_config.ps_root_ca
        template_config.install_upgrade_certs(root_ca, ps_root_ca)

    def install_upgrade_certs(self, root_ca, ps_root_ca):
        _install_upgrade_certs(root_ca, ps_root_ca)

    def generate_local_ssl_certs_if_they_dont_exist(self):
        server_https_cert = os.path.join(SUPPORT_FILE_PATH, "https", "ca", "root-ca.crt.pem")
        if not os.path.isfile(server_https_cert):
            self.update_server.generate_update_certs()

    def create_alc_policy_for_warehouse_credentials(self, template_name, username, target_output_file):
        templ = TemplateConfig('HopefullyNoEnvironmentVariableWillHaveThisName',
                               username, PROD_BUILD_CERTS, BALLISTA_ADDRESS)
        templ.generate_warehouse_policy_from_template(template_name, target_output_file)

    def get_version_for_rigidname_in_vut_warehouse(self, rigidname):
        if rigidname == "ServerProtectionLinux-Base":
            if not os.environ.get("BALLISTA_VUT"):
                # dev warehouse hard code version
                return "1.0.0"
            return get_version_from_sdds_import_file(
                os.path.join(PathManager.SYSTEM_PRODUCT_TEST_INPUTS, "sspl-componentsuite", "SDDS-Import.xml"))
        warehouse_root = os.path.join(LOCAL_WAREHOUSES_ROOT, "dev", "sspl-warehouse", "develop", "warehouse",
                                      "warehouse")
        product_name = self.RIGIDNAMES_AGAINST_PRODUCT_NAMES_IN_VERSION_INI_FILES[rigidname]
        version = subprocess.check_output(
            f'grep -r "PRODUCT_NAME = {product_name}" {SYSTEMPRODUCT_TEST_INPUT}/local_warehouses/dev/sspl-warehouse/develop/warehouse/warehouse/ | awk -F: \'{{print $1}}\' | xargs grep "PRODUCT_VERSION" | sed "s/PRODUCT_VERSION\ =\ //"',
            shell=True)
        return version.strip().decode()

    def get_version_for_rigidname_in_sdds3_warehouse(self, release_type, rigidname):
        if release_type == "vut":
            warehouse_root = os.path.join(SYSTEMPRODUCT_TEST_INPUT, "sdds3", "repo", "package")
        else:
            warehouse_root = os.path.join(SYSTEMPRODUCT_TEST_INPUT, f"sdds3-{release_type}", "repo", "package")
        product_name = self.RIGIDNAMES_AGAINST_PRODUCT_NAMES_IN_VERSION_INI_FILES[rigidname]

        try:
            packages = os.listdir(warehouse_root)
        except EnvironmentError:
            logger.error("Can't list warehouse_root: %s" % warehouse_root)
            log = os.path.join(SYSTEMPRODUCT_TEST_INPUT, "GatherReleaseWarehouses.log")
            if os.path.isfile(log):
                contents = str(open(log).read())
                logger.error("GatherReleaseWarehouses.log: %s" % contents)
            raise
        for package in packages:
            if package.startswith(product_name):
                version = package[len(product_name)+1:-15]
                if not re.match(r"^((?:(9+)\.)?){3}(\*|\d+)$", version):
                    return version
        raise AssertionError(f"Did not find {rigidname} in {warehouse_root}")

    def second_version_is_lower(self, version1, version2):
        return version.parse(version1) > version.parse(version2)


# If ran directly, file sets up local warehouse directory from filer6
if __name__ == "__main__":
    pass
