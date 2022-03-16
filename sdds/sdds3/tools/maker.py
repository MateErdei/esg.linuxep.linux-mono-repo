#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020-2021 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Create SDDS3 suites and supplements from the YAML definitions"""

import xml.etree.ElementTree as ET

import argparse
import hashlib
import itertools
import logging
import os
import re
import subprocess
import tempfile
import time
import yaml

from retry import retry


TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)


def say(msg, level=logging.INFO):
    logging.log(level=level, msg=msg)


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


def hash_file(file):
    file_size = os.stat(file).st_size
    sha256 = hashlib.sha256()
    with open(file, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    file_sha256 = sha256.hexdigest()
    return file_size, file_sha256


def calculate_suite_nonce(args):
    _, metadata_sha256 = hash_file(args.meta)
    return metadata_sha256[:10]


def find_metadata(fileset, metadata, by_views=False):
    def _patheq(patha, pathb):
        return patha.lower() == pathb.lower()
    views = []
    for view in metadata['views']:
        if 'platforms' not in view:
            raise NameError('Internal error: view has no platforms')
        vdef = view['def']
        view_fileset = os.path.abspath(os.path.join(BASE, vdef['fileset']))
        if _patheq(fileset, view_fileset):
            if not by_views:
                return vdef
            views.append(vdef)
        for comp in view.get('subcomponents', []):
            cdef = view['subcomponents'][comp]
            cdef_fileset = os.path.abspath(os.path.join(BASE, cdef['fileset']))
            if _patheq(fileset, cdef_fileset):
                if not by_views:
                    return cdef
                views.append(cdef)
    if len(views) == 0:
        raise SyntaxError(f'Did not find {fileset} in metadata?')
    return views


def find_supplements(meta):
    return meta['supplements'] if 'supplements' in meta else []


def _map_suite_packages(args, metadata):
    if len(args.imports) != len(args.packages):
        raise ValueError('Mismatched number of --import and --package args')

    # Next, map each SDDS-Import.xml file with its built SDDS3 package (so we
    # know which size/sha256 to associate with each package)
    #
    # Then load the metadata file and write the XML to a temporary file
    #
    # Finally create the package using the sdds3-builder.exe tool.

    # map package name to tuple(parsed-sdds-import, package-filename, fileset, list-of-views)
    packages = {}
    suiteids = []
    suitename = None
    while args.imports:
        imp = args.imports.pop(0)
        pkg = args.packages.pop(0)

        fileset = os.path.abspath(os.path.dirname(imp))

        views = find_metadata(fileset, metadata, by_views=True)

        name = views[0]['package_name']
        version = views[0]['package_version']
        nonce = views[0]['package_nonce']
        key = f'{name}_{version}.{nonce}'

        if key in packages:
            raise SyntaxError(f'Duplicate package {name}')

        with open(imp) as f:
            xml = ET.ElementTree(None, f)

        packages[key] = (xml, pkg, os.path.abspath(os.path.dirname(imp)), views)

        # The first package is always the top-level suite package
        if suitename is None:
            suitename = name
        if name == suitename:
            suiteids.append(key)
    return packages, suiteids


def _make_pkgref(root, pkgfile):
    size, sha256 = hash_file(pkgfile)
    pkgref = ET.SubElement(root, 'package-ref')
    pkgref.attrib['size'] = str(size)
    pkgref.attrib['sha256'] = sha256
    pkgref.attrib['src'] = os.path.basename(pkgfile)
    return pkgref


def _override_or(table, field, default):
    return table.get(field) or default()


def _optional_intersection_of(collection, table, field):
    filter_by = _override_or(table, field, lambda: [])
    return [val for val in collection if val in filter_by] if len(filter_by) > 0 else collection


def _get_platforms(pkgroot):
    return [x.attrib['Name'] for x in pkgroot.findall('Component/Platforms/Platform')]


def _choose_platforms(pkgroot, meta):
    if 'override_platforms' in meta:
        return meta['override_platforms']
    return _optional_intersection_of(_get_platforms(pkgroot), meta, 'platforms')


def _choose_features(pkgroot, meta):
    if 'override_features' in meta:
        return meta['override_features']
    return _optional_intersection_of(pkgroot.findall('Component/Features/Feature'), meta, 'features')


def _add_pkgref_meta(pkgref, pkgroot, override):
    ET.SubElement(pkgref, 'name').text = _override_or(
        override,
        'package_name',
        lambda: pkgroot.findtext('Component/Name').replace(' ', ''))
    ET.SubElement(pkgref, 'version').text = _override_or(
        override,
        'package_version',
        lambda: pkgroot.findtext('Component/Version'))
    ET.SubElement(pkgref, 'line-id').text = pkgroot.findtext('Component/RigidName')
    ET.SubElement(pkgref, 'nonce').text = override['package_nonce']
    ET.SubElement(pkgref, 'description').text = _override_or(
        override,
        'description',
        lambda: pkgroot.findtext('Dictionary/Name/Label/Language[@lang="en"]/LongDesc'))
    compat_code = _override_or(
        override,
        'compat_code',
        lambda: pkgroot.findtext('Component/CompatCode'))
    if compat_code is not None:
        ET.SubElement(pkgref, 'compat-code').text = compat_code
    if 'thumbprint-include-root' in override:
        thumbprint = ET.SubElement(pkgref, 'thumbprint')
        thumbprint.attrib['include-root'] = 'true' if override['thumbprint-include-root'] else 'false'


def _add_pkgref_platform_features(pkgref, pkgroot, meta):
    ET.SubElement(pkgref, 'decode-path').text = _override_or(
        meta,
        'mountpoint',
        lambda: pkgroot.findtext('Component/DefaultHomeFolder'))

    symbolic_paths = _override_or(meta, 'define-symbolic-paths', lambda: [])
    if len(symbolic_paths) > 0:
        paths = ET.SubElement(pkgref, 'define-symbolic-paths')
        for sym in sorted(symbolic_paths):
            ET.SubElement(paths, 'symbolic-path').attrib['id'] = sym

    platforms = _choose_platforms(pkgroot, meta)
    if len(platforms) > 0:
        plats = ET.SubElement(pkgref, 'platforms')
        for platform in platforms:
            ET.SubElement(plats, 'platform').attrib['name'] = platform
    features = _choose_features(pkgroot, meta)
    if len(features) > 0:
        feats = ET.SubElement(pkgref, 'features')
        for feature in features:
            ET.SubElement(feats, 'feature').attrib['name'] = feature.attrib['Name']


def _add_pkgref_supplements(pkgref, meta):
    supplements = find_supplements(meta)
    if len(supplements) > 0:
        for supplement in supplements:
            supref = ET.SubElement(pkgref, 'supplement-ref')
            supref.attrib['src'] = supplement['line-id']
            supref.attrib['tag'] = supplement['tag']
            supref.attrib['decode-path'] = _override_or(supplement, 'decode-path', lambda: '.')


def _parse_integrity_line(line):
    verb, rest = line.split(maxsplit=1)
    line = rest

    vals = []
    while len(line) > 0:
        match = re.search(r'^([0-9a-f]{4}):', line)
        if match:
            length = int(match[1], base=16)
            vals.append(line[5:5 + length])
            line = line[5 + length + 1:]
        else:
            try:
                var, rest = line.split(maxsplit=1)
                vals.append(var)
                line = rest
            except ValueError:
                vals.append(line)
                line = ''

    return verb, vals


def _verify_integrity_statement(integrity_statements, location, name, token, vals):
    if not token.startswith('Protect'):
        return

    if token not in integrity_statements:
        integrity_statements[token] = {}
    protect_statements = integrity_statements[token]

    canonical_object = vals[0].lower()

    if canonical_object in protect_statements:
        previous = protect_statements[canonical_object]
        if previous['name'] != name:
            raise SyntaxError(
                f'Duplicate {token} {vals[0]}: declared in {location} and also in {previous["location"]}')

    integrity_statements[token][canonical_object] = {'name': name, 'location': location}


def _verify_integrity_file(allowed_duplicates, integrity_statements, pkgfile, pkgroot, tmpdir, integrity):
    path = os.path.join(tmpdir, integrity)
    okay = True
    with open(path, 'r') as f:
        name = None
        for line in f.readlines():
            token, vals = _parse_integrity_line(line)
            if token == 'ProtectDetails':
                name = vals[3]
            else:
                for platform in _get_platforms(pkgroot):
                    if platform not in integrity_statements:
                        integrity_statements[platform] = {}

                    try:
                        _verify_integrity_statement(
                            integrity_statements[platform], f'{pkgfile}/{integrity}', name, token, vals)
                    except SyntaxError as e:
                        if not (token in allowed_duplicates and vals[0].lower() in allowed_duplicates[token]):
                            say(f'{platform}: {e}', level=logging.ERROR)
                            okay = False
                        else:
                            # say(f'{platform}: {e}', level=logging.WARN)
                            pass
    return okay


def _load_allowed_integrity_duplicates(args):
    allowed_duplicates = {}
    with open(args.allowed_integrity_duplicates, 'r') as f:
        for line in f.readlines():
            line = line.strip()
            if not line.startswith('Protect'):
                continue
            token, val = line.split(maxsplit=1)
            if token not in allowed_duplicates:
                allowed_duplicates[token] = []
            allowed_duplicates[token].append(val.lower())
    return allowed_duplicates


def _verify_integrity_files(args, integrity_statements, pkgfile, pkgroot):
    # TODO remove?
    # allowed_duplicates = _load_allowed_integrity_duplicates(args)

    files = subprocess.check_output(
        [args.sdds3_builder,
         '--print-package-manifest',
         '--package', pkgfile],
        encoding='utf-8', stderr=subprocess.PIPE)

    integrity_files = []
    for line in files.splitlines():
        if 'integrity' in line and line.endswith('.dat'):
            _, _, _, filename = line.split(maxsplit=3)
            integrity_files.append(filename)
    if not integrity_files:
        return True

    with tempfile.TemporaryDirectory() as tmpdir:
        cmd = [args.sdds3_builder,
               '--extract-package',
               '--package', pkgfile,
               '--dir', tmpdir] + list(itertools.chain.from_iterable(('--file', f) for f in integrity_files))
        subprocess.run(cmd, capture_output=True, check=True)

        # TODO remove?
        # okay = True
        # for integrity in integrity_files:
        #     if not _verify_integrity_file(
        #             allowed_duplicates, integrity_statements, pkgfile, pkgroot, tmpdir, integrity):
        #         okay = False
        # return okay


# pylint: disable=R0914     # too many local variables.
def make_sdds3_suite(args):
    if not args.meta:
        raise NameError('The --metadata option is required with --suite-dir')

    metadata = yaml.safe_load(open(args.meta).read())
    print(metadata)
    packages, suiteids = _map_suite_packages(args, metadata)

    root = ET.Element('suite')
    root.attrib['name'] = metadata['def']['package_name']
    root.attrib['version'] = metadata['def']['package_version']
    root.attrib['marketing-version'] = metadata['marketing_version']

    okay = True
    for pkg in sorted(packages):
        pkgroot, pkgfile, _, views = packages[pkg][:]

        for meta in views:
            integrity_statements = {}
            pkgref = _make_pkgref(root, pkgfile)
            _add_pkgref_meta(pkgref, pkgroot, meta)

            # Load integrity.dat statements, and check for duplicates
            if not _verify_integrity_files(args, integrity_statements, pkgfile, pkgroot):
                okay = False

            _add_pkgref_platform_features(pkgref, pkgroot, meta)

            # If the platform list is empty, discard this entire pkgref!
            if pkgref.find('platforms') is None:
                root.remove(pkgref)
                continue

            _add_pkgref_supplements(pkgref, meta)

    if not okay:
        raise SyntaxError('One or more errors found - stopping')

    with tempfile.TemporaryDirectory() as tmpdir:
        suite_xml = os.path.join(tmpdir, 'suite.xml')
        with open(suite_xml, 'wb') as f:
            ET.ElementTree(element=root).write(f, encoding='UTF-8', xml_declaration=True)

        cmd = [args.sdds3_builder,
               '--build-suite',
               '--xml', suite_xml,
               '--dir', args.suite,
               '--nonce', calculate_suite_nonce(args)]
        if args.verbose:
            cmd.append('--verbose')
        run_with_retries(cmd)


def _map_supplement_packages(args):
    if len(args.imports) != len(args.packages):
        raise ValueError('Mismatched number of --import and --package args')
    if len(args.imports) != len(args.tags):
        raise ValueError('Mismatched number of --import and --tag args')

    packages = {}
    supplement_name = None
    while args.imports:
        imp = args.imports.pop(0)
        pkg = args.packages.pop(0)
        tag = args.tags.pop(0)
        tags = tag.split(',') if tag else []

        _, sha256 = hash_file(imp)
        nonce = sha256[:10]
        with open(imp) as f:
            xml = ET.ElementTree(None, f)

        supplement_name = xml.findtext('Component/RigidName')

        version = xml.findtext('Component/Version')
        dupname = f'{supplement_name}_{version}'
        if dupname in packages:
            raise SyntaxError(f'Duplicate package {dupname}')

        packages[dupname] = (xml, pkg, tags, nonce)
    return packages, supplement_name


def make_sdds3_supplement(args):
    packages, supplement_name = _map_supplement_packages(args)

    root = ET.Element('supplement')
    root.attrib['name'] = supplement_name
    root.attrib['timestamp'] = time.strftime("%FT%TZ", time.gmtime())

    for pkg in sorted(packages):
        pkgref = _make_pkgref(root, packages[pkg][1])
        _add_pkgref_meta(pkgref, packages[pkg][0], {'package_nonce': packages[pkg][3]})

        ET.SubElement(pkgref, 'decode-path').text = '.'

        tags = ET.SubElement(pkgref, 'tags')
        for pkgtag in packages[pkg][2]:
            ET.SubElement(tags, 'tag').attrib['name'] = pkgtag

    with tempfile.TemporaryDirectory() as tmpdir:
        xml = os.path.join(tmpdir, 'supplement.xml')
        with open(xml, 'wb') as f:
            ET.ElementTree(element=root).write(f, encoding='UTF-8', xml_declaration=True)

        cmd = [args.sdds3_builder,
               '--build-supplement',
               '--xml', xml,
               '--dir', args.supplement]
        if args.verbose:
            cmd.append('--verbose')
        run_with_retries(cmd)


@retry(exceptions=subprocess.CalledProcessError, tries=5, delay=2, backoff=2)
def run_with_retries(cmd):
    subprocess.check_call(cmd)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--metadata', dest='meta')
    parser.add_argument('--suite-dir', dest='suite')
    parser.add_argument('--supplement-dir', dest='supplement')
    parser.add_argument('--allowed-integrity-duplicates')
    parser.add_argument('--verbose', action='store_true')
    parser.add_argument('--package', dest='packages', action='append')
    parser.add_argument('--import', dest='imports', action='append')
    parser.add_argument('--tag', dest='tags', action='append')
    parser.add_argument('--sdds3-builder', required=True)
    args = parser.parse_args()

    if args.suite:
        make_sdds3_suite(args)
    elif args.supplement:
        make_sdds3_supplement(args)
    else:
        raise ValueError('Invalid invocation: must specify --suite-dir or --supplement-dir')


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
    main()
