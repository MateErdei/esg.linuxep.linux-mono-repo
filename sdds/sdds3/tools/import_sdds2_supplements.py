#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Wrapper script to create a set of SDDS3 supplements and associated packages from an SDDS2 supplement warehouse"""

import xml.etree.ElementTree as ET

import argparse
import codecs
import hashlib
import os
import shutil
import subprocess
import tempfile
import yaml

import requests

# What this script does:
# - you give it an SDDS2 supplement catalogue as a file or URL, and it creates an SDDS3 supplement
#   for each rigidname found (as well as any packages required)

_EXE = '.exe' if os.name == 'nt' else ''
PACKAGE_BUILDER_EXE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sdds3-builder{_exe}".format(_exe=_EXE))

SESSION = requests.Session()


class AutoRemove:
    def __init__(self, files):
        self.files = files

    def __enter__(self):
        pass

    def __exit__(self, dontcaretype, dontcarevalue, dontcaretb):
        for f in self.files:
            try:
                os.remove(f)
            except FileNotFoundError:
                pass


def is_url(string):
    return string.startswith('http://') or string.startswith('https://')


def trace(msg):
    print(msg)


def run(args):
    trace('Running: ' + str(args))
    subprocess.check_call(args)


def hash_file(file):
    file_size = os.stat(file).st_size
    sha256 = hashlib.sha256()
    with open(file, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    file_sha256 = sha256.hexdigest()
    return file_size, file_sha256


def get_url(url):
    """Return the content at url"""
    resp = SESSION.get(url)
    if resp:
        return resp.text
    trace(f'{resp.status_code}: {url}')
    raise requests.HTTPError(request=resp.request, response=resp.status_code)


def get_file(location):
    """Return contents of a file"""
    with codecs.open(location, 'r', encoding='utf-8') as f:
        return f.read()


def get_file_or_url(location):
    if is_url(location):
        return get_url(location.replace('\\', '/'))
    return get_file(location)


def copy_file_or_url(origin, target, quiet=False):
    """Copy a file from a path or https:// URL"""
    if is_url(origin):
        origin = origin.replace(os.sep, '/')
        target = target.replace('/', os.sep)

        topdir = os.path.dirname(target)
        if not os.path.exists(topdir):
            os.makedirs(topdir)

        if not quiet:
            trace(f'downloading {origin} -> {target}')
        resp = SESSION.get(origin, stream=True)
        if resp:
            with open(target, 'wb') as f:
                for chunk in resp.iter_content(65536):
                    f.write(chunk)
            return True
        trace(f'Error: {resp.status_code} {resp.reason}: GET {origin} -> {target}')
        return False
    if os.path.exists(origin):
        if not quiet:
            trace(f'copying: {origin} -> {target}')
        topdir = os.path.dirname(target)
        if not os.path.exists(topdir):
            os.makedirs(topdir)
        shutil.copy(origin, target)
        return True
    return False


def copy_fileset_and_create_sdds_import(repo, objcache, folder, contents, root):
    """Copy an entire fileset to a specified folder so that the package builder tool can package it up"""

    files = ET.SubElement(root.find('Component'), 'FileList')

    for directory in contents.findall('{badger:contents}directories/{badger:contents}directory'):
        for dentry in directory.findall('{badger:contents}file'):
            target = os.path.join(folder, dentry.attrib['name'])

            md5 = dentry.find('{badger:contents}md5')
            dat = f'{md5.text}{md5.attrib["extent"]}.dat'
            cache = os.path.join(objcache, dat)
            if not os.path.exists(cache):
                copy_file_or_url(os.path.join(repo, dat), cache, quiet=True)
            copy_file_or_url(cache, target, quiet=True)

            filenode = ET.SubElement(files, 'File')
            filenode.attrib['MD5'] = md5.text
            filenode.attrib['Name'] = dentry.attrib['name']
            filenode.attrib['Size'] = dentry.attrib['size']

            if 'offset' in dentry.attrib:
                raise AttributeError('Unexpected "offset" attribute')

    with open(os.path.join(folder, 'SDDS-Import.xml'), 'wb') as f:
        ET.ElementTree(element=root).write(f, encoding="UTF-8", xml_declaration=True)


def get_sdds2_xml(parent, xpath, repo, rigidname=None):
    node = parent.find(xpath)
    if node is None:
        raise TypeError(f'node {parent} has no children found by xpath expression "{xpath}"')

    name = f'{node.text}{node.attrib["extent"]}.xml'
    url = repo
    if rigidname:
        url += f'/{rigidname}'
    url += f'/{name}'

    return get_file_or_url(url)


def import_sdds2_supplement(args):
    """Import an SDDS2 supplement into a set of folders, writing a YAML metadata file"""
    # strip the '/catalogue' to get the base URL
    repo = args.catalogue[:args.catalogue.find('/catalogue')]

    def rigidname_version(ver):
        """Return a tuple containing the majorRollout and minorRollout"""
        rollout = ver.find('{badger:rigidName}rollOut')
        return (rollout.attrib['majorRollOut'], rollout.attrib['minorRollOut'])

    def has_release_tags(attr):
        return None is not attr.find(
            '{badger:ReleaseAttributes}Attribute[@name="ReleaseTags"]/'
            + '{badger:ReleaseAttributes}ReleaseTag/{badger:ReleaseAttributes}Tag')

    def get_release_tags(attr):
        return [t.text for t in attr.findall(
            '{badger:ReleaseAttributes}Attribute[@name="ReleaseTags"]/'
            + '{badger:ReleaseAttributes}ReleaseTag/{badger:ReleaseAttributes}Tag')]

    def make_root(pkgname, pkgversion):
        root = ET.Element('ComponentData')
        comp = ET.SubElement(root, 'Component')
        ET.SubElement(comp, 'Name').text = pkgname        # NOTE: good enough for current supplements
        ET.SubElement(comp, 'RigidName').text = pkgname
        ET.SubElement(comp, 'Version').text = pkgversion
        ET.SubElement(comp, 'Build').text = 0
        return root

    def mirror_versions_with_release_tags(pkgname, rigidnode):
        rigidname = ET.fromstring(
            get_sdds2_xml(rigidnode, '{badger:ultimate}md5', repo, rigidnode.attrib['rigidName']))

        versions = {}

        sorted_versions = sorted(
            rigidname.findall('{badger:rigidName}versions/{badger:rigidName}version'),
            key=rigidname_version
        )
        for ver in sorted_versions:
            attr = ET.fromstring(get_sdds2_xml(ver, '{badger:rigidName}attributes/{badger:rigidName}md5', repo))
            if not has_release_tags(attr):
                continue

            pkgversion = ver.find('{badger:rigidName}rollOut').attrib['version-id']

            print(f'Importing {pkgname} version {pkgversion} from {args.catalogue}...')

            pkg = ET.fromstring(get_sdds2_xml(ver, '{badger:rigidName}md5', repo))
            copy_fileset_and_create_sdds_import(
                repo, workdir,
                os.path.join(args.mirror, pkgname, pkgversion),
                ET.fromstring(get_sdds2_xml(pkg, '{badger:package}contents/{badger:package}md5', repo)),
                make_root(pkgname, pkgversion)
            )

            versions[pkgversion] = get_release_tags(attr)
        return versions

    with tempfile.TemporaryDirectory() as workdir:
        # trace(f'Loading top-level catalogue: {args.catalogue}')
        top = ET.fromstring(get_file_or_url(args.catalogue))
        ult = ET.fromstring(get_sdds2_xml(top, '{badger:catalogue}file/{badger:catalogue}md5', repo))
        for rigidnode in ult.findall('{badger:ultimate}rigidNames/*'):
            pkgname = rigidnode.attrib['rigidName']
            versions = mirror_versions_with_release_tags(pkgname, rigidnode)

            meta = os.path.join(args.mirror, pkgname, 'meta.yaml')
            with open(meta, 'w') as metafh:
                yaml.dump({'versions': versions}, metafh)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mirror', dest='mirror', required=True)
    parser.add_argument('--catalogue', dest='catalogue', required=True)
    parser.add_argument('--verbose', action='store_true')

    args = parser.parse_args()

    import_sdds2_supplement(args)


if __name__ == "__main__":
    main()
