#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2020-2022 Sophos Limited. All rights reserved.
"""Common utils for building SDDS3 suites and packages"""

import codecs
import hashlib
import logging
import os
import shutil
import subprocess
import sys
import fileinput
import requests

from retry import retry
from xml.etree import ElementTree as ET

TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)
ROOT = os.path.dirname(BASE)

META_DIR = os.path.join(BASE, '.meta')
ARTIFACT_CACHE = os.path.join(ROOT, 'inputs', 'artifact')

ARTIFACTORY_URL = f'https://{os.environ["TAP_PROXY_ARTIFACT_AUTHORITY_EXTERNAL"]}/artifactory' \
                  if 'TAP_PROXY_ARTIFACT_AUTHORITY_EXTERNAL' in os.environ \
                  else 'https://artifactory.sophos-ops.com'

SESSION = requests.Session()
HEADERS = {}


def set_artifactory_auth_headers():
    if 'BUILD_JWT_PATH' in os.environ:
        with codecs.open(os.environ['BUILD_JWT_PATH'], 'r', encoding='utf-8') as f:
            jwt = f.read().strip()
        HEADERS['Authorization'] = f'Bearer {jwt}'


# Convert the URL into an esg-build-candidate (because we don't have access to the former).
# releasable-candidate artifacts look like this:
#    esg-releasable-candidate/<repo>/<branch>/<build>/<path-to-fileset>
# build-candidate artifactrs look like this:
#    esg-build-candidate/<repo>/<branch>/<build>/build/<path-to-fileset>.zip
def convert_releasable_to_build_artifact(artifact):
    if artifact.startswith('esg-build-store-trusted/'):
        return artifact
    parts = artifact.split('/', 4)
    parts[0] = 'esg-build-candidate'
    parts.insert(-1, 'build')
    return '/'.join(parts)


def copy_url_to(origin, target):
    origin = origin.replace(os.sep, '/')
    target = target.replace('/', os.sep)

    topdir = os.path.dirname(target)
    os.makedirs(topdir, exist_ok=True)

    @retry(tries=5, delay=0.2)
    def makerequest():
        resp = SESSION.get(origin, stream=True, headers=HEADERS)
        if resp:
            with open(target, 'wb') as f:
                for chunk in resp.iter_content(65536):
                    f.write(chunk)
            return True
        say(f'{resp.status_code} {resp.reason}: GET {origin}', level=logging.WARN)
        if resp.status_code < 500:
            return False
        raise requests.HTTPError(f'{resp.status_code} {resp.reason}: GET {origin}')
    return makerequest()


def say(msg, level=logging.INFO, indent=0):
    if indent >= 4:
        msg = f'{f"[{indent}]":<{indent}}{msg}'
    elif indent > 0:
        msg = f'{" ":<{indent}}{msg}'
    logging.log(level=level, msg=msg)


_MODE = {}


def get_bazel_output_path(target):
    relpath = os.path.relpath(target, ROOT)
    return os.path.join(ROOT, 'bazel-bin', relpath)


def get_bazel_target_for_path(abspath, target=None):
    relpath = os.path.relpath(abspath, ROOT).replace(os.path.sep, '/')
    bzlpath = f'//{relpath}'
    if target is not None:
        bzlpath += f':{target}'
    return bzlpath


def set_inputs_mode(mode):
    _MODE['_mode'] = os.path.join(ROOT, 'inputs', mode)


def get_relative_fileset(abspath):
    return os.path.relpath(abspath, os.path.join(ROOT, 'inputs', _MODE['_mode']))


def get_absolute_fileset(fileset):
    return os.path.join(ROOT, 'inputs', fileset)


def hash_content(content):
    sha256 = hashlib.sha256()
    sha256.update(content)
    return sha256.hexdigest()


def hash_file(name):
    sha256 = hashlib.sha256()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    return sha256.hexdigest()

def hash_file_sha384(name):
    sha384 = hashlib.sha384()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha384.update(byte_block)
    return sha384.hexdigest()

def hash_file_md5sum(name):
    md5 = hashlib.md5()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            md5.update(byte_block)
    return md5.hexdigest()
_AUTOVERSION_CACHE = {}


def autoversion(comp, baseversion, key=None):
    """Return an auto-incrementing version number unique to the 'comp', starting at {baseversion}.0.

    If key is provided, then repeated calls to _autoversion() with the same comp, baseversion and key
    return the same version. If key is omitted, a new unique version is always provided.
    """
    cache_key = (comp, baseversion)
    if cache_key not in _AUTOVERSION_CACHE:
        _AUTOVERSION_CACHE[cache_key] = {
            '__i': 0,
        }

    # if key was provided, check if we already calculated its version.
    if key and key in _AUTOVERSION_CACHE[cache_key]:
        return _AUTOVERSION_CACHE[cache_key][key]

    # Generate a new version
    ver = f'{baseversion}.{_AUTOVERSION_CACHE[cache_key]["__i"]}'
    _AUTOVERSION_CACHE[cache_key]['__i'] += 1

    # if key was provided, save its version.
    if key:
        _AUTOVERSION_CACHE[cache_key][key] = ver

    return ver


def count_autoversions(comp, baseversion):
    cache_key = (comp, baseversion)
    if cache_key not in _AUTOVERSION_CACHE:
        return 0
    return _AUTOVERSION_CACHE[cache_key]['__i']


def visit_suite_instances(suites):
    for suite in suites:
        suitedef = suites[suite]
        for instance in suitedef['instances']:
            yield suite, suitedef, instance


def visit_instance_views(instance):
    for view in instance['views']:
        yield view


def visit_suite_instance_views(suites):
    for suite, suitedef, instance in visit_suite_instances(suites):
        for view in visit_instance_views(instance):
            yield suite, suitedef, instance, view


def is_static_suite_instance(instance):
    return '_STATIC' in instance['tags']

def change_version_to_999(dist):
    sdds_import = os.path.join(dist, 'SDDS-Import.xml')
    manifest = os.path.join(dist, 'manifest.dat')
    top_level_VERSION_file = os.path.join(dist, 'VERSION.ini')
    VERSION_file = ""

    for root, dirs, files in os.walk(os.path.join(dist, 'files')):
        for file in files:
            if file.endswith("VERSION.ini"):
                VERSION_file = os.path.join(root, file)

    version = '99.99.99'

    # change version to 99.99.99 in VERSION.ini files
    for line in fileinput.input([top_level_VERSION_file], inplace=True):
        if line.strip().startswith('PRODUCT_VERSION ='):
            line = 'PRODUCT_VERSION = ' + version + '\n'
        sys.stdout.write(line)

    shutil.copy(top_level_VERSION_file, VERSION_file)
    sha384 = hash_file_sha384(top_level_VERSION_file)
    md5 = hash_file_md5sum(top_level_VERSION_file)

    #update version ini files in sdds import
    with open(sdds_import) as f:
        xml = ET.fromstring(f.read())

    xml.find('Component/Version').text = version

    sdds_import_filelist = xml.find('Component/FileList')

    for file in sdds_import_filelist:
        if "VERSION.ini" in file.attrib['Name']:
            file.attrib['SHA384'] = sha384
            file.attrib['MD5'] = md5
            file.attrib['Size'] = str(os.path.getsize(top_level_VERSION_file))

    with open(os.path.join(sdds_import), 'wb') as f:
        ET.ElementTree(element=xml).write(f, encoding='UTF-8', xml_declaration=True)

    #generate new manifest
    exclusions = 'SDDS-Import.xml,manifest.dat'
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = "/usr/lib:/usr/lib64"
    env['OPENSSL_PATH'] = "/usr/bin/openssl"

    result = subprocess.run(
        ['sb_manifest_sign', '--folder', dist, '--output', manifest, '--exclusions', exclusions]
        , stderr=subprocess.STDOUT, stdout=subprocess.PIPE, env=env,
        timeout=60
    )

# TODO LINUXDAR-8265 Remove this select once all customers are on Base 1.2.6 or higher.
def change_version_for_platform(dist):
    sdds_import = os.path.join(dist, 'SDDS-Import.xml')

    #update version ini files in sdds import
    with open(sdds_import) as f:
        xml = ET.fromstring(f.read())

    version = xml.find('Component/Version').text

    if len(version.split(".")) == 4:
        if dist.endswith("arm64"):
            version = ".".join(version.split(".")[:-1] + ["0"] + [version.split(".")[-1]])
        elif dist.endswith("x64"):
            version = ".".join(version.split(".")[:-1] + ["1"] + [version.split(".")[-1]])
        xml.find('Component/Version').text = version

        with open(os.path.join(sdds_import), 'wb') as f:
            ET.ElementTree(element=xml).write(f, encoding='UTF-8', xml_declaration=True)
