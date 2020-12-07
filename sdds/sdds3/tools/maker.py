#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Create SDDS3 suites and supplements from the YAML definitions"""

import xml.etree.ElementTree as ET

import argparse
import hashlib
import os
import subprocess
import tempfile
import time
import yaml


TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)


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


def find_metadata(fileset, metadata):
    def _patheq(patha, pathb):
        return patha.lower() == pathb.lower()
    suite_fileset = os.path.abspath(os.path.join(BASE, metadata['fileset']))
    if _patheq(fileset, suite_fileset):
        return metadata
    for comp in metadata['subcomponents']:
        cdef = metadata['subcomponents'][comp]
        cdef_fileset = os.path.abspath(os.path.join(BASE, cdef['fileset']))
        if _patheq(fileset, cdef_fileset):
            return cdef
    raise SyntaxError(f'Did not find {fileset} in metadata?')


def find_supplements(fileset, metadata):
    meta = find_metadata(fileset, metadata)
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

    # map package name to tuple(parsed-sdds-import, package-filename)
    packages = {}
    suiteid = None
    while args.imports:
        imp = args.imports.pop(0)
        pkg = args.packages.pop(0)

        fileset = os.path.abspath(os.path.dirname(imp))
        meta = find_metadata(fileset, metadata)

        name = meta['package_name']

        if name in packages:
            raise SyntaxError(f'Duplicate package {name}')

        with open(imp) as f:
            xml = ET.ElementTree(None, f)
        packages[name] = (xml, pkg, os.path.abspath(os.path.dirname(imp)))

        # The first package is always the top-level suite package
        if suiteid is None:
            suiteid = name
    return packages, suiteid


def _make_pkgref(root, pkgfile):
    size, sha256 = hash_file(pkgfile)
    pkgref = ET.SubElement(root, 'package-ref')
    pkgref.attrib['size'] = str(size)
    pkgref.attrib['sha256'] = sha256
    pkgref.attrib['src'] = os.path.basename(pkgfile)
    return pkgref


def _override_or(table, field, default):
    return table.get(field) or default()


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


def _add_pkgref_platform_features(pkgref, pkgroot, meta):
    ET.SubElement(pkgref, 'decode-path').text = pkgroot.findtext('Component/DefaultHomeFolder')

    symbolic_paths = _override_or(meta, 'define-symbolic-paths', lambda: [])
    if len(symbolic_paths) > 0:
        paths = ET.SubElement(pkgref, 'define-symbolic-paths')
        for sym in sorted(symbolic_paths):
            ET.SubElement(paths, 'symbolic-path').attrib['id'] = sym

    platforms = pkgroot.findall('Component/Platforms/Platform')
    if len(platforms) > 0:
        plats = ET.SubElement(pkgref, 'platforms')
        for platform in platforms:
            ET.SubElement(plats, 'platform').attrib['name'] = platform.attrib['Name']
    features = pkgroot.findall('Component/Features/Feature')
    if len(features) > 0:
        feats = ET.SubElement(pkgref, 'features')
        for feature in features:
            ET.SubElement(feats, 'feature').attrib['name'] = feature.attrib['Name']


def _add_pkgref_supplements(pkgref, fileset, metadata):
    supplements = find_supplements(fileset, metadata)
    if len(supplements) > 0:
        for supplement in supplements:
            supref = ET.SubElement(pkgref, 'supplement-ref')
            supref.attrib['src'] = supplement['line-id']
            supref.attrib['tag'] = supplement['tag']
            supref.attrib['decode-path'] = supplement['decode-path']


def make_sdds3_suite(args):
    if not args.meta:
        raise NameError('The --metadata option is required with --suite-dir')

    metadata = yaml.safe_load(open(args.meta).read())

    packages, suiteid = _map_suite_packages(args, metadata)

    root = ET.Element('suite')
    suite_meta = find_metadata(packages[suiteid][2], metadata)
    root.attrib['name'] = suite_meta['package_name']
    root.attrib['version'] = suite_meta['package_version']
    root.attrib['marketing-version'] = suite_meta['marketing_version']

    for pkg in sorted(packages):
        pkgroot, pkgfile, fileset = packages[pkg][:]

        pkgref = _make_pkgref(root, pkgfile)
        _add_pkgref_meta(pkgref, pkgroot, find_metadata(fileset, metadata))

        # If this package is the top-level suite's package, *IGNORE* the platforms and features.
        # We *always* want to download and decode the suite package.
        if pkg == suiteid:
            ET.SubElement(pkgref, 'decode-path').text = '.'
        else:
            _add_pkgref_platform_features(pkgref, pkgroot, find_metadata(fileset, metadata))

        _add_pkgref_supplements(pkgref, fileset, metadata)

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
        subprocess.check_call(cmd)


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
        subprocess.check_call(cmd)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--metadata', dest='meta')
    parser.add_argument('--suite-dir', dest='suite')
    parser.add_argument('--supplement-dir', dest='supplement')
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
    main()
