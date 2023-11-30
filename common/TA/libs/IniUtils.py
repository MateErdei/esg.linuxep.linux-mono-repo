# Copyright 2020-2023 Sophos Limited. All rights reserved.

import configparser

from robot.api import logger


def get_value_from_ini_file(key: str, file: str, section: str = "fake_section") -> str:
    config = configparser.ConfigParser()
    try:
        config.read(file)
    except configparser.MissingSectionHeaderError:
        with open(file) as f:
            file_content = f"[{section}]\n" + f.read()
            config = configparser.RawConfigParser()
            config.read_string(file_content)
    return config[section][key]


def get_version_number_from_ini_file(file):
    with open(file) as ini_file:
        lines = ini_file.readlines()
        version_text = "PRODUCT_VERSION = "
        for line in lines:
            if version_text in line:
                version_string = line[len(version_text) :]
                logger.info(f"Found version string: {version_string}")
                return version_string.strip()
    logger.error("Version String Not Found")
    raise AssertionError("Version String Not Found")


def get_rtd_version_number_from_ini_file(path):
    try:
        return get_value_from_ini_file("RTD_PRODUCT_VERSION", path)
    except KeyError:
        return get_version_number_from_ini_file(path)


def version_number_in_ini_file_should_be(file, expected_version):
    actual_version = get_version_number_from_ini_file(file)
    if actual_version != expected_version:
        raise AssertionError(f"Expected version {actual_version} to be {expected_version}")
