#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import re
import sys


def get_base_path(script_path, BASE=None):
    if BASE is None:
        BASE = os.path.dirname(script_path)  # <plugin>/redist/pluginapi
        BASE = os.path.dirname(BASE)  # <plugin>/redist
        BASE = os.path.dirname(BASE)  # <plugin>

    return BASE

def get_possible_auto_version_sub_paths():
    possible_sub_paths = [os.path.join("products", "distribution", "include",
                                       "AutoVersioningHeaders", "AutoVersion.ini"),
                          os.path.join("AutoVersioningHeaders", "AutoVersion.ini")
                          ]
    return possible_sub_paths

def get_vaild_auto_version_path(BASE):
    for path in get_possible_auto_version_sub_paths():
        full_path = os.path.join(BASE, path)
        if os.path.isfile(full_path):
            return full_path
    return None

def readVersionIniFile(BASE=None):
    """
    __file__ is either <plugin>/redist/pluginapi/distribution/generateSDDSImport.py
    or <base>/products/distribution/generateSDDSImport.py

    So we need to handle either.

    :param BASE: $BASE environment variable, or argv[3]
    :return: version from ini file or None
    """

    scriptPath = os.path.dirname(os.path.realpath(__file__))  # <plugin>/redist/pluginapi/distribution
    version = None

    autoVersionFile = os.path.join(scriptPath, "include", "AutoVersioningHeaders", "AutoVersion.ini")

    if not os.path.isfile(autoVersionFile):
        BASE = get_base_path(scriptPath, BASE)

        if os.path.isfile(os.path.join(BASE, "Jenkinsfile")):  # Check we have a correct directory
            autoVersionFile = get_vaild_auto_version_path(BASE)

    if autoVersionFile is not None:
        print ("Reading version from {}".format(autoVersionFile))
        with open(autoVersionFile, "r") as f:
            for line in f.readlines():
                if "ComponentAutoVersion=" in line:
                    version = line.strip().split("=")[1]
    else:
        print("Failed to get AutoVersion from {}".format(autoVersionFile))

    if version == "1.0.999":
        print("Ignoring template version {} from {}".format(version, autoVersionFile))
        version = None

    return version


def readVersionFromJenkinsFile(BASE=None):
    scriptPath = os.path.dirname(os.path.realpath(__file__))
    ## Try reading from Jenkinsfile
    productsDir = os.path.dirname(scriptPath)
    srcdir = os.path.dirname(productsDir)
    jenkinsfile = os.path.join(srcdir, "Jenkinsfile")
    if not os.path.isfile(jenkinsfile):
        ## if we are in a plugin then the script is in a sub-directory 1 deeper.
        srcdir = os.path.dirname(srcdir)
        jenkinsfile = os.path.join(srcdir, "Jenkinsfile")
    if BASE is not None and not os.path.isfile(jenkinsfile):
        ## Try base
        jenkinsfile = os.path.join(BASE, "Jenkinsfile")


    if os.path.isfile(jenkinsfile):
        lines = open(jenkinsfile).readlines()
        print("Reading version from {}".format(jenkinsfile))
        LINE_RE=re.compile(r"version_key = '([\d.]+)'$")
        for line in lines:
            line = line.strip()
            mo = LINE_RE.match(line)
            if mo:
                return mo.group(1)
    else:
        print("Failed to find Jenkinsfile {} from {}".format(jenkinsfile, scriptPath))

    return None


def defaultVersion():
    defaultValue = "0.5.1"
    print ("Using default {}".format(defaultValue))
    return defaultValue


def readVersion(BASE=None):
    """
    10 possible use cases:
    (Base or Plugin) *
    (
        manual build using cmake directly - e.g. CLion
        local build.sh
        Local Jenkins
        ESG-CI (Artisan-CI)
        Production Artisan
    )

    manual build has to find version without $BASE being set
        Base finds it relative to the script dir
        Plugins find it up a directory from the script dir
    other builds should have BASE set correctly.

    :return:
    """
    temp = os.environ.get("PRODUCT_VERSION", None)
    if temp is not None:
        return temp

    if BASE is None:
        BASE = os.environ.get("BASE", None)
    version = readVersionIniFile(BASE) or readVersionFromJenkinsFile(BASE) or defaultVersion()
    print("Using version {}".format(version))
    return version


def main(argv):
    version = readVersion()
    if len(argv) > 1:
        open(argv[1], "wb").write(version)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
