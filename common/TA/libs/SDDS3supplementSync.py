#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020-2022 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Sync SDDS3 supplements"""

import xml.etree.ElementTree as ET

import argparse
import codecs
import os
import shutil
import subprocess
import sys

import requests

SESSION = requests.Session()

def is_url(string):
    return string.startswith('http://') or string.startswith('https://')


def trace(msg):
    print(msg)


def run(args):
    trace('Running: ' + str(args))
    subprocess.check_call(args)


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
            trace(f'Downloading {origin} -> {target}')
        with SESSION.get(origin, stream=True) as resp:
            if resp:
                actual_size = 0
                expected_size = int(resp.headers['content-length'])
                tmptarget = f'{target}.tmp'
                with open(tmptarget, 'wb') as f:
                    for chunk in resp.iter_content(65536):
                        f.write(chunk)
                        actual_size += len(chunk)

                if actual_size != expected_size:
                    trace(f'Error: Read {actual_size} but expected {expected_size}: GET {origin} -> {target}')
                    os.remove(tmptarget)
                    return False

                if os.path.exists(target):
                    os.remove(target)
                os.rename(tmptarget, target)
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


def is_pkgref_for_release_group(pkgref, tag, release_group):
    tags = pkgref.findall("./tags/tag")

    for t in tags:
        if t.get("name") != tag:
            continue
        groups = t.findall("./release-group")
        for g in groups:
            if g.get("name") == release_group:
                return True

    return False


def sync_sdds3_supplement(supplement, builder, mirror, tag="LINUX_SCAN", release_group="0"):
    """Mirror an SDDS3 supplement into a folder"""
    # strip the '/catalogue/supplement' to get the base URL
    repo = supplement[:supplement.find('/supplement/')]
    target = f'{mirror}/supplement/{os.path.basename(supplement)}'
    trace(f'Mirroring {supplement} -> {target}')
    copy_file_or_url(supplement, target)
    subprocess.call(['chmod', '+x', builder])
    runargs = [
        builder,
        '--unpack-signed-file',
        target,
    ]
    try:
        result = subprocess.run(
            runargs,
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            encoding='utf-8',
        )
    except subprocess.CalledProcessError as e:
        trace(f"FAILED CALL: {e.returncode},\n{e.stderr}\n{e.stdout}")
        raise

    root = ET.fromstring(result.stdout)
    dest = target+".decoded.xml"
    open(dest, "w").write(result.stdout)

    for pkgref in root.findall('package-ref'):
        src = pkgref.attrib['src']
        if not is_pkgref_for_release_group(pkgref, tag, release_group):
            trace(f"Ignoring {src} as it doesn't match tags/release_group")
            continue

        dest = f'{mirror}/package/{src}'

        if os.path.exists(dest):
            trace(f'Skipping {repo}/package/{src}: using cached copy')
            return dest

        copy_file_or_url(f'{repo}/package/{src}', dest)
        return dest

    trace("Failed to find any matching tag/release_group packages in " + result.stdout)
    trace("Looking for tag=" + tag)
    trace("With release_group=" + release_group)
    return None


def unpack(builder, dest_zipfile, unpack_dir):
    """
    sdds3/sdds3-builder --extract-package --package /tmp/download_supplements/sdds3/package/ML_MODEL3_LINUX_2022.9.28.13.10.47.1.92ce572aa3.zip --dir /tmp/ML_MODEL3_LINUX
    """
    print("Unpacking", dest_zipfile)

    shutil.rmtree(unpack_dir, ignore_errors=True)

    command = [builder, "--extract-package", "--package", dest_zipfile, "--dir", unpack_dir]
    subprocess.run(command, check=True)
    for (base, dirs, files) in os.walk(unpack_dir):
        for f in files:
            os.chmod(os.path.join(base, f), 0o644)
        for d in dirs:
            os.chmod(os.path.join(base, d), 0o755)
    os.chmod(unpack_dir, 0o755)


def sync_sdds3_supplement_name(name, builder, destination, tag, release_group="0"):
    if not name.startswith("sdds3."):
        print("Forcing supplement name to start with sdds3.")
        name = "sdds3." + name

    supplement_name = name
    supplement = "https://sdds3.sophosupd.com/supplement/" + supplement_name + ".dat"

    return sync_sdds3_supplement(supplement, builder, destination, tag, release_group)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--mirror', dest='mirror', required=True)
    parser.add_argument('--supplement', dest='supplement', required=True)
    parser.add_argument('--builder', dest='builder', required=True)
    parser.add_argument('--verbose', action='store_true')

    args = parser.parse_args()

    sync_sdds3_supplement(args.supplement, args.builder, args.mirror)


if __name__ == '__main__':
    sys.exit(main())
