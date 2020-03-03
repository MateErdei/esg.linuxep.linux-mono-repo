#!/usr/bin/env python2
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import os
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
import xml.dom.minidom


# This can throw
def get_value_from_ini_file(file_path, key):
    with open(file_path) as ini_file:
        for line in ini_file.readlines():
            line = line.strip()
            if key in line:
                value = line.split("=")[-1].strip()
                logger.info("Found, {}:{} in file: {}".format(key, value, file_path))
                return value

def get_machine_id_or_default(default=None):
    try:
        return _get_machine_id()
    except Exception as ex:
        logger.info("Could not find machine ID - Returning default, error: {}".format(str(ex)))
        return default


# This can throw, use either get_machine_id_or_fail or get_machine_id_or_default
def _get_machine_id():
    machine_id_file_path = os.path.join(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"), "base", "etc", "machine_id.txt")
    with open(machine_id_file_path) as machine_id_file:
        machine_id = machine_id_file.read().strip()
        return machine_id


def get_customer_id_or_default(default=None):
    try:
        return _get_customer_id()
    except Exception as ex:
        logger.info("Could not find customer ID - Returning default, error: {}".format(str(ex)))
        return default


# This can throw, use either get_customer_id_or_fail or get_customer_id_or_default
def _get_customer_id():
    alc_policy_file_path = os.path.join(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"), "base", "mcs", "policy", "ALC-1_policy.xml")
    with open(alc_policy_file_path, "r") as alc_policy_file:
        dom = xml.dom.minidom.parseString(alc_policy_file.read())
        customer_element = dom.getElementsByTagName("customer")[0]
        customer_id = customer_element.getAttribute("id")
        return customer_id


def get_base_version_or_default(default=None):
    try:
        return _get_base_version()
    except Exception as ex:
        logger.info("Could not find version - Returning default, error: {}".format(str(ex)))
        return default


def _get_base_version():
    version_location = os.path.join(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"), "base", "VERSION.ini")
    return get_value_from_ini_file(version_location, "PRODUCT_VERSION")


def get_endpoint_id_or_default(default=None):
    try:
        return _get_endpoint_id()
    except Exception as ex:
        logger.info("Could not find endpoint ID - Returning default, error: {}".format(str(ex)))
        return default


# This can throw
def _get_endpoint_id():
    mcs_config_location = os.path.join(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"), "base", "etc", "sophosspl", "mcs.config")
    return get_value_from_ini_file(mcs_config_location, "MCSID")

def get_install():
    try:
        return format(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"))
    except RobotNotRunningError:
        try:
            return os.environ.get("SOPHOS_INSTALL", "/opt/sophos-spl")
        except KeyError:
            return "/opt/sophos-spl"
