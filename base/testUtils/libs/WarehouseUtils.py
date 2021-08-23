#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime
import time
import os
import shutil
import hashlib
import glob
import UpdateServer
import robot.api.logger as logger
from robot.libraries.BuiltIn import BuiltIn
import xml.etree.ElementTree as ET
import requests
import subprocess


import PathManager

THIS_FILE = os.path.realpath(__file__)
LIBS_DIR = PathManager.get_libs_path()
PRODUCT_TEST_ROOT_DIRECTORY =  PathManager.get_testUtils_dir()

SUPPORT_FILE_PATH = PathManager.get_support_file_path()
REAL_WAREHOUSE_DIRECTORY = os.path.join(SUPPORT_FILE_PATH, "CentralXml", "RealWarehousePolicies")
TEMPLATE_FILE_DIRECTORY = os.path.join(REAL_WAREHOUSE_DIRECTORY, "templates")
GENERATED_FILE_DIRECTORY = os.path.join(REAL_WAREHOUSE_DIRECTORY, "GeneratedAlcPolicies")

FILER_6_DIRECTORY = os.path.join("/", "mnt", "filer6", "bfr", "sspl-warehouse")
LOCAL_WAREHOUSES_ROOT = "/tmp/system-product-test-inputs/local_warehouses"
LOCAL_WAREHOUSES = os.path.join(LOCAL_WAREHOUSES_ROOT, "dev", "sspl-warehouse")
WAREHOUSE_LOCAL_SERVER_PORT = 443
# newline is important in the redirect below
OSTIA_HOST_REDIRECT = """127.0.0.1  ostia.eng.sophos
"""
INTERNAL_OSTIA_HOST_REDIRECT = """127.0.0.1  ostia.eng.sophos
10.1.200.228    dci.sophosupd.com
10.1.200.228    dci.sophosupd.net
10.1.200.228    d1.sophosupd.com
10.1.200.228    d1.sophosupd.net
10.1.200.228    d2.sophosupd.com
10.1.200.228    d2.sophosupd.net
10.1.200.228    d3.sophosupd.com
10.1.200.228    d3.sophosupd.net
"""
OSTIA_HOSTS_BACKUP_FILENAME="ostia_hosts.bk"

USERNAME = "username"
PASSWORD = "password"
ENV_KEY = "env_key"

BUILD_TYPE = "build_type"
PROD_BUILD_CERTS = "prod"
DEV_BUILD_CERTS = "dev"

# The version under test, usually master dev builds
OSTIA_VUT_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/vut"
# Warehouse without RECOMMENDED tag
OSTIA_BETA_ONLY_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/only-beta"
# Usually the previous release
OSTIA_PREV_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/latest-recommended"
# The GA Release
# A version with mocked libraries (to test file removal on upgrade)
OSTIA_0_6_0_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/0-6-0"
#a version with a different query pack than vu
OSTIA_QUERY_PACK_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/query-pack"
# a version with edr 9.99.9 for downgrade tests
OSTIA_EDR_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/edr-999"
OSTIA_BASE_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/base-999"
OSTIA_MTR_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/mdr-999"
OSTIA_EDR_AND_MTR_999_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/edr-mdr-999"
# A warehouse containing 3 base versions for paused updating tests
OSTIA_PAUSED_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/paused-updating"
# A warehouse containing Base and EDR pre live response
OSTIA_BASE_EDR_OLD_WH_ADDRESS = "https://ostia.eng.sophos/latest/sspl-warehouse/old-style"
# dictionary of ostia addresses against the ports that should be used to serve their customer files locally
OSTIA_ADDRESSES = {
                    OSTIA_VUT_ADDRESS: "2233",
                    OSTIA_PREV_ADDRESS: "3233",
                    OSTIA_0_6_0_ADDRESS: "4233",
                    OSTIA_PAUSED_ADDRESS: "6233",
                    OSTIA_EDR_999_ADDRESS: "7233",
                    OSTIA_BASE_999_ADDRESS: "7235",
                    OSTIA_QUERY_PACK_ADDRESS: "7239",
                    OSTIA_MTR_999_ADDRESS: "7237",
                    OSTIA_EDR_AND_MTR_999_ADDRESS: "7240",
                    OSTIA_BETA_ONLY_ADDRESS: "7244",
                    OSTIA_BASE_EDR_OLD_WH_ADDRESS: "7245"
                   }

BALLISTA_ADDRESS = "https://dci.sophosupd.com/cloudupdate"

DEV_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "rootca.crt")
DEV_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "ps_rootca.crt")
PROD_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "rootca.crt")
PROD_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "ps_rootca.crt")

ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "rootcerts", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "rootcerts", "ps_rootca.crt")
ROOT_CA_INSTALLATION_EXTENSION_OLD = os.path.join("base", "update", "certs", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION_OLD = os.path.join("base", "update", "certs", "ps_rootca.crt")

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

    # todo consider making this better
    try:
        shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION))
        shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION))
    except:
        shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION_OLD))
        shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION_OLD))

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


def getYesterday():
    now = datetime.date.today()
    yesterday = now - datetime.timedelta(days=1)
    return yesterday.strftime("%A")  # Returns day as week day name

filer6_directory = "/mnt/filer6/linux/SSPL/testautomation/sdds-specs/"

def get_importrefrence_for_component_with_tag(rigid_name, tag, pubspec):
    line = list(filter(lambda n: rigid_name == n.attrib["id"], pubspec.findall("./warehouses//line")))[0]
    for component in line.findall("./component"):
        for release_tag in component.findall("./releasetag"):
            if release_tag.attrib["tag"] == tag:
                return component.attrib["importreference"]
    raise AssertionError(f"Did not find {rigid_name} in {pubspec}")

def get_importrefrence_for_component_with_tag_from_componentsuite(rigid_name, componentsuite_rigid_name, tag, pubspec):
    componentsuite_importref = get_importrefrence_for_component_with_tag(componentsuite_rigid_name, tag, pubspec)

    line = list(filter(lambda n: componentsuite_rigid_name == n.attrib["id"], pubspec.findall("./componentsuites/line")))[0]
    componentsuite = list(filter(lambda n: componentsuite_importref == n.attrib["importreference"], line.findall("./componentsuite")))[0]
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

def get_dci_xml_from_update_credentials(update_credentials):
    dci_url = f"https://dci.sophosupd.com/update/{update_credentials[0]}/{update_credentials[1:3]}/{update_credentials}.dat"
    r = requests.get(dci_url)
    signature_start = r.content.find(b"-----BEGIN SIGNATURE-----")
    xml_string = r.content[:signature_start].decode()
    return xml_string

def get_sdds_names_from_dci_xml_string(dci_xml_string):
    root = ET.fromstring(dci_xml_string)
    warehouse_entries = root.findall("./Warehouses/WarehouseEntry/Name")
    sdds_names = []
    for warehouse_entry in warehouse_entries:
        sdds_names.append(warehouse_entry.text)
    return sdds_names

def get_sdds_names_from_update_credentials(update_credentials):
    xml = get_dci_xml_from_update_credentials(update_credentials)
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

def get_spec_xml_dict_from_filer6():
    files_on_filer6 = os.listdir(filer6_directory)
    files_on_filer6_dict = {}
    for file_name in files_on_filer6:
        spec = ET.parse(os.path.join(filer6_directory, file_name))
        expected_sdds_name = ".".join(file_name.split(".")[:3])
        spec_type = get_spec_type_from_spec(spec)
        if not files_on_filer6_dict.get(expected_sdds_name, None):
            files_on_filer6_dict[expected_sdds_name] = {}

        if files_on_filer6_dict[expected_sdds_name].get(spec_type, None):
            raise AssertionError(f"Found multiple {spec_type} for {expected_sdds_name}: {files_on_filer6_dict[expected_sdds_name][spec_type]} & {file_name}")
        else:
            files_on_filer6_dict[expected_sdds_name][spec_type] = spec

    # import pprint
    # pprint.pprint(files_on_filer6_dict)

    return files_on_filer6_dict

def get_version_of_component_with_tag_from_spec_xml(rigidname, tag, spec_xml_dict, relevant_sdds_names):
    for sdds_name in relevant_sdds_names:
        # print(sdds_name)
        try:
            if spec_xml_dict.get(sdds_name, None):
                importref = get_importrefrence_for_component_with_tag(rigidname, tag, spec_xml_dict[sdds_name][pubspec])
                version = get_version_for_component_with_importref(rigidname, importref, spec_xml_dict[sdds_name][importspec])
                return version
        except:
            pass
    else:
        # print(relevant_sdds_names)
        # print(spec_xml_dict.keys())
        raise AssertionError(f"Did not find {rigidname} in {spec_xml_dict}")

def get_version_of_component_with_tag_from_spec_xml_from_componentsuite(rigidname, componentsuite_rigid_name, tag, spec_xml_dict, relevant_sdds_names):
    for sdds_name in relevant_sdds_names:
        # print(sdds_name)
        try:
            if spec_xml_dict.get(sdds_name, None):
                importref = get_importrefrence_for_component_with_tag_from_componentsuite(rigidname, componentsuite_rigid_name, tag, spec_xml_dict[sdds_name][pubspec])
                version = get_version_for_component_with_importref(rigidname, importref, spec_xml_dict[sdds_name][importspec])
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
        self.yesterday = getYesterday()
        self.local_connection_address = None
        self.env_key = env_key
        environment_config = os.environ.get(env_key, None)
        if environment_config:
            # assume ballista warehouse if override given
            user_pass = environment_config.split(":")
            self.username = user_pass[0]
            self.password = user_pass[1]
            self.hashed_credentials = user_pass[2]
            self.relevent_sdds_files = get_sdds_names_from_update_credentials(self.hashed_credentials)
            self.remote_connection_address = BALLISTA_ADDRESS
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
            self.customer_file_domain = "localhost:{}".format(self.local_customer_file_port)
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
        if self.use_local_warehouses and self.remote_connection_address != BALLISTA_ADDRESS:
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

    def generate_warehouse_policy_from_template(self, template_file_name, proposed_output_path = None):
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
            logger.info("ALC policy will be placed at: {}".format(proposed_output_path))
            output_policy = proposed_output_path

        if not os.path.isfile(template_policy):
            raise OSError("{} does not exist".format(template_policy))

        password_marker = "@PASSWORD@"
        username_marker = "@USERNAME@"
        connection_address_marker = "@CONNECTIONADDRESS@"
        algorithm_marker = "@ALGORITHM@"
        yesterday_marker = "@YESTERDAY@"

        with open(template_policy) as template_file:
            template_string = template_file.read()
            template_string_with_replaced_values = template_string.replace(password_marker, self.password) \
                .replace(username_marker, self.username) \
                .replace(connection_address_marker, self.get_connection_address()) \
                .replace(algorithm_marker, self.algorithm) \
                .replace(yesterday_marker, self.yesterday)
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
        return output_policy

    def install_sophos_alias_file(self):
        _install_sophos_alias_file(self.get_connection_address())

    def install_sophos_alias_file_if_using_ostia(self):
        if self.remote_connection_address in OSTIA_ADDRESSES:
            self.install_sophos_alias_file()

    def get_basename_from_url(self):
        return self.remote_connection_address.rsplit("/", 1)[-1]

    def get_relevant_sdds_filenames(self):
        return self.relevent_sdds_files

class WarehouseUtils(object):
    """
    Class to setup ALC Policy files used in tests.
    """

    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    ROBOT_LISTENER_API_VERSION = 2
    #os.environ['VUT_PREV']= "CSP7I0S0GZZE:CCADasoUC50JcnRFdhJR+mhonNzZ872yyT0W8e2/3dGohT2bPmkQy/baXddi+RzbTxg=:db55fcf8898da2b3f3c06f26e9246cbb"
    os.environ['VUT_PREV']= "QA767596:CCCirMa73nCbF+rU9aHeyqCasmwZR9GpWyPav3N0ONhr56KqcJR8L7OdlrmdHJLXc08=:105f4d87e14c91142561bf3d022e55b9"

    WAREHOUSE_SPEC_XML = get_spec_xml_dict_from_filer6()
    template_configuration_values = {
        "base_and_mtr_0_6_0.xml": TemplateConfig("BASE_AND_MTR_0_6_0", "mtr_user_0_6_0", PROD_BUILD_CERTS, OSTIA_0_6_0_ADDRESS),
        "base_and_edr_999.xml": TemplateConfig("BASE_AND_EDR_999", "edr_user_999", PROD_BUILD_CERTS, OSTIA_EDR_999_ADDRESS),
        "mtr_edr_vut_and_base_999.xml": TemplateConfig("BASE_999", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_BASE_999_ADDRESS),
        "old_query_pack.xml": TemplateConfig("QUERY_PACK", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_QUERY_PACK_ADDRESS),
        "base_edr_vut_and_mtr_999.xml": TemplateConfig("BASE_EDR_AND_MTR_999", "mtr_user_999", PROD_BUILD_CERTS, OSTIA_MTR_999_ADDRESS ),
        "base_mtr_vut_and_edr_999.xml": TemplateConfig("BASE_MTR_AND_EDR_999", "user_mtr_vut_edr_999", PROD_BUILD_CERTS, OSTIA_EDR_999_ADDRESS),
        "base_vut_and_mtr_edr_999.xml": TemplateConfig("BASE_AND_MTR_EDR_999", "mtr_and_edr_user_999", PROD_BUILD_CERTS, OSTIA_EDR_AND_MTR_999_ADDRESS),
        "base_vut_and_mtr_edr_av_999.xml": TemplateConfig("BASE_AND_MTR_EDR_AV_999", "av_user_999", PROD_BUILD_CERTS, OSTIA_EDR_AND_MTR_999_ADDRESS),
        "base_and_mtr_VUT.xml": TemplateConfig("BALLISTA_VUT", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_and_mtr_VUT-1.xml": TemplateConfig("VUT_PREV", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_and_av_VUT.xml": TemplateConfig("BALLISTA_VUT", "av_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_and_av_VUT-1.xml": TemplateConfig("VUT_PREV", "av_user_vut", PROD_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_and_mtr_and_av_VUT.xml": TemplateConfig("BASE_AND_MTR_AND_AV_VUT", "av_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_and_mtr_and_av_VUT-1.xml": TemplateConfig("VUT_PREV", "av_user_vut", PROD_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_edr_and_mtr_and_av_VUT.xml": TemplateConfig("BASE_EDR_AND_MTR_AND_AV_VUT", "av_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_edr_and_mtr_and_av_VUT-1.xml": TemplateConfig("VUT_PREV", "av_user_vut", PROD_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_edr_and_mtr_VUT-1.xml": TemplateConfig("VUT_PREV", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_and_edr_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_edr_and_mtr.xml": TemplateConfig("BALLISTA_VUT", "mtr_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_0_6_0.xml": TemplateConfig("BASE_ONLY_0_6_0", "base_user_0_6_0", PROD_BUILD_CERTS, OSTIA_0_6_0_ADDRESS),
        "base_only_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_fixed_version_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_fixed_version_999.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_weeklyScheduleVUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_VUT_no_ballista_override.xml": TemplateConfig("NO_OVERRIDE", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_only_VUT-1.xml": TemplateConfig("VUT_PREV", "base_user_vut", PROD_BUILD_CERTS, OSTIA_PREV_ADDRESS),
        "base_VUT_and_fake_plugins.xml": TemplateConfig("BALLISTA_VUT", "fake_plugin_user", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_paused_update_VUT-1.xml": TemplateConfig("BASE_PAUSED_VUT_PREV", "base_user_paused", PROD_BUILD_CERTS, OSTIA_PAUSED_ADDRESS),
        "base_paused_update_VUT.xml": TemplateConfig("BALLISTA_VUT", "base_user_paused", PROD_BUILD_CERTS, OSTIA_PAUSED_ADDRESS),
        "base_paused_update_999.xml": TemplateConfig("BASE_PAUSED_999", "base_user_paused", PROD_BUILD_CERTS, OSTIA_PAUSED_ADDRESS),
        "base_only_VUT_without_SDU_Feature.xml": TemplateConfig("BALLISTA_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_VUT_ADDRESS),
        "base_beta_only.xml": TemplateConfig("BASE_ONLY_VUT", "base_user_vut", PROD_BUILD_CERTS, OSTIA_BETA_ONLY_ADDRESS),
        "base_edr_old_wh_format.xml": TemplateConfig("BASE_EDR_OLD_WH", "base_user_vut", PROD_BUILD_CERTS, OSTIA_BASE_EDR_OLD_WH_ADDRESS),
    }

    RIGIDNAMES_AGAINST_PRODUCT_NAMES_IN_VERSION_INI_FILES = {
        "ServerProtectionLinux-Plugin-AV": "Sophos Server Protection Linux - av",
        "ServerProtectionLinux-Plugin-liveresponse": "Sophos Live Response",
        "ServerProtectionLinux-MDR-Control-Component": "Sophos Managed Threat Response plug-in" ,
        "ServerProtectionLinux-Plugin-EventJournaler": "EventJournaler",
        "ServerProtectionLinux-Base-component": "Sophos Server Protection Linux - Base Component",
        "ServerProtectionLinux-Plugin-EDR": "Sophos Endpoint Detection and Response plug-in"
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
        if os.environ.get("INTERNAL_HOST_REDIRECT"):
            logger.info("using internal redirect")
            self.update_server.modify_host_file_for_local_updating(new_hosts_file_content=INTERNAL_OSTIA_HOST_REDIRECT, backup_filename=OSTIA_HOSTS_BACKUP_FILENAME)
        else:
            self.update_server.modify_host_file_for_local_updating(new_hosts_file_content=OSTIA_HOST_REDIRECT, backup_filename=OSTIA_HOSTS_BACKUP_FILENAME)

    def start_all_local_update_servers(self):
        self.update_server.stop_update_server()

        for address, port in list(OSTIA_ADDRESSES.items()):
            branch_name = os.path.basename(address)
            local_warehouse_branch = os.path.join(LOCAL_WAREHOUSES, branch_name)
            list_of_files = os.listdir(local_warehouse_branch)
            logger.info(list_of_files)
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

    def create_ballista_config_from_template_policy(self, template_policy, username, password):

        env_key = "BALLISTA_CONFIG"
        generated_ballista_policy_name = "ballista.xml"
        os.environ[env_key] = "{}:{}".format(username, password)
        ballista_config = TemplateConfig(env_key, None, PROD_BUILD_CERTS, BALLISTA_ADDRESS)

        if os.path.isabs(template_policy) or os.path.dirname(template_policy):
            template_policy_name = os.path.basename(template_policy)
        else:
            template_policy_name = template_policy

        generated_ballista_policy_path = os.path.join(GENERATED_FILE_DIRECTORY, generated_ballista_policy_name)
        ballista_config.generate_warehouse_policy_from_template(template_policy_name, proposed_output_path=generated_ballista_policy_path)
        self.template_configuration_values["template_policy_name"] = ballista_config
        return generated_ballista_policy_path


    def __get_localwarehouse_path_for_branch(self, branch):
        return os.path.join("/tmp/system-product-test-inputs/local_warehouses/dev/sspl-warehouse",
                            branch,
                            "warehouse/warehouse/catalogue")


    def Disable_Product_Warehouse_to_ensure_we_only_perform_a_supplement_update(self, branch="develop"):
        # /tmp/system-product-test-inputs/local_warehouses/dev/sspl-warehouse/develop/warehouse/warehouse/catalogue
        # LOCAL_WAREHOUSES=/tmp/system-product-test-inputs/local_warehouses/dev/sspl-warehouse
        # templateConfig = self._get_template_config_from_dictionary_using_path(template_path)
        # base = templateConfig.get_basename_from_url()
        logger.info("Disable_Product_Warehouse_to_ensure_we_only_perform_a_supplement_update for {}".format(branch))
        path = self.__get_localwarehouse_path_for_branch(branch)

        PROTECTED_SUPPLEMENT_WAREHOUSES = [
            'sdds.ssplflags-wh.xml'
        ]

        for x in os.listdir(path):
            if x in PROTECTED_SUPPLEMENT_WAREHOUSES:
                logger.debug("Not renaming supplement warehouse: {}".format(x))
                continue
            src = os.path.join(path, x)
            bak = src + ".bak"
            if not os.path.isfile(bak):
                logger.debug("Renaming {} to {}".format(src, bak))
                os.rename(src, bak)

    def Restore_Product_Warehouse(self, branch="develop"):
        logger.info("Restore_Product_Warehouse - after breaking for supplement-only update - for {}".format(branch))
        path = self.__get_localwarehouse_path_for_branch(branch)

        for x in os.listdir(path):
            if x.endswith(".bak"):
                src = os.path.join(path, x)
                target = src[:-4]
                logger.debug("Renaming {} to {}".format(src, target))
                os.rename(os.path.join(path, x), target)

    def get_version_from_warehouse_for_rigidname(self, template_policy, rigidname, tag="RECOMMENDED"):
        template_config = self._get_template_config_from_dictionary_using_path(template_policy)
        relevant_sdds_files = template_config.get_relevant_sdds_filenames()
        return get_version_of_component_with_tag_from_spec_xml(rigidname, tag, self.WAREHOUSE_SPEC_XML, relevant_sdds_files)

    def get_version_from_warehouse_for_rigidname_in_componentsuite(self, template_policy, rigidname, componentsuite_rigidname, tag="RECOMMENDED"):
        template_config = self._get_template_config_from_dictionary_using_path(template_policy)
        relevant_sdds_files = template_config.get_relevant_sdds_filenames()
        return get_version_of_component_with_tag_from_spec_xml_from_componentsuite(rigidname, componentsuite_rigidname, tag, self.WAREHOUSE_SPEC_XML, relevant_sdds_files)

    def get_version_for_rigidname_in_vut_warehouse(self, rigidname):
        warehouse_root = os.path.join(LOCAL_WAREHOUSES_ROOT, "dev", "sspl-warehouse", "develop", "warehouse", "warehouse")
        product_name = self.RIGIDNAMES_AGAINST_PRODUCT_NAMES_IN_VERSION_INI_FILES[rigidname]
        version = subprocess.check_output(f'grep -r "PRODUCT_NAME = {product_name}" /tmp/system-product-test-inputs/local_warehouses/dev/sspl-warehouse/develop/warehouse/warehouse/ | awk -F: \'{{print $1}}\' | xargs grep "PRODUCT_VERSION" | sed "s/PRODUCT_VERSION\ =\ //"', shell=True)
        return version.strip().decode()

# If ran directly, file sets up local warehouse directory from filer6
if __name__ == "__main__":
    _make_local_copy_of_warehouse()