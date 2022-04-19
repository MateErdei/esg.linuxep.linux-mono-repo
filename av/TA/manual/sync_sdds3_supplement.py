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

# What this script does:
# - you give it an SDDS2 supplement catalogue as a file or URL, and it creates an SDDS3 supplement
#   for each rigidname found (as well as any packages required)

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
        resp = SESSION.get(origin, stream=True)
        if resp:
            tmptarget = f'{target}.tmp'
            with open(tmptarget, 'wb') as f:
                for chunk in resp.iter_content(65536):
                    f.write(chunk)
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


def sync_sdds3_supplement(supplement, builder, mirror):
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
            capture_output=True,
            universal_newlines=True,
            encoding='utf-8',
        )
    except subprocess.CalledProcessError as e:
        trace(f"FAILED CALL: {e.returncode},\n{e.stderr}\n{e.stdout}")
        raise e
    root = ET.fromstring(result.stdout)
    for pkgref in root.findall('package-ref'):
        src = pkgref.attrib['src']
        if os.path.exists(f'{mirror}/package/{src}'):
            trace(f'Skipping {repo}/package/{src}: using cached copy')
            continue
        copy_file_or_url(f'{repo}/package/{src}', f'{mirror}/package/{src}')


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
