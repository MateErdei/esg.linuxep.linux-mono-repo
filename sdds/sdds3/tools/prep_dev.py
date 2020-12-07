#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Generate bazel rules to create SDDS3 packages and suites from YAML defs"""

import xml.etree.ElementTree as ET

from glob import glob

import hashlib
import json
import logging
import os
import shutil
import subprocess
import sys
import time
import yaml

from retry import retry


TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)
ROOT = os.path.dirname(BASE)

# Map of fileset -> package target
FILESET_TO_PKGTARGET = {}
# Map of package target -> fileset
PKGTARGET_TO_FILESET = {}


def say(msg, level=logging.INFO):
    logging.log(level=level, msg=msg)


def hash_file(name):
    sha256 = hashlib.sha256()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    return sha256.hexdigest()


def emit_sdds_import_buildfile(fileset, prebuilt):
    buildfile = os.path.join(fileset, 'BUILD')
    with open(buildfile, 'w') as f:
        print(f"""# AUTO-GENERATED BUILD FILE. DO NOT EDIT.
filegroup(
    name = "sdds_import",
    srcs = ["SDDS-Import.xml"],
    visibility = ["//visibility:public"],
)
filegroup(
    name = "prebuilt_package",
    srcs = ["{prebuilt}"],
    visibility = ["//visibility:public"],
)
""", file=f)


def bazelize_sdds_import_target(fileset, target):
    abspath = os.path.abspath(fileset)
    relpath = os.path.relpath(abspath, ROOT).replace(os.path.sep, '/')
    return f"//{relpath}:{target}"


def _fixup_suite_sdds_import(compdef):
    # Edit the suite's SDDS-Import.xml file to make suites work properly
    # Read the SDDS-Import.xml file to grok the name, version and nonce
    sdds_import = os.path.join(BASE, compdef['fileset'], 'SDDS-Import.xml')
    with open(sdds_import) as f:
        xml = ET.fromstring(f.read())

    line_id = compdef['line-id']
    xml.find('Component/RigidName').text = line_id
    xml.find('Component/Name').text = line_id

    # AutoVersion the suites. Use the specified version as the base.
    now = time.gmtime()
    baseversion = f'{now.tm_year}.{now.tm_mon}.{now.tm_mday}'
    result = subprocess.run(
        ['versioning_client', '-c', f'dev_sdds3_{line_id}', '-v', baseversion],
        check=True,
        capture_output=True,
        universal_newlines=True,
        encoding='utf-8',
    )
    version = result.stdout.strip()

    say(f'{line_id}: setting version to {version}')
    xml.find('Component/Version').text = version

    with open(sdds_import, 'wb') as f:
        ET.ElementTree(element=xml).write(f, encoding='UTF-8', xml_declaration=True)


def _get_package_info(compdef):
    # Read the SDDS-Import.xml file to grok the name, version and nonce
    sdds_import = os.path.join(BASE, compdef['fileset'], 'SDDS-Import.xml')
    nonce = hash_file(sdds_import)[:10]
    with open(sdds_import) as f:
        xml = ET.ElementTree(None, f)
    name = xml.findtext('Component/Name').replace(' ', '')
    version = xml.findtext('Component/Version')

    return name, version, nonce


def _get_package_target_from_name_version_nonce(name, version, nonce):
    return f'{name}_{version}.{nonce}'


def _get_package_target_from_compdef_and_suite(compdef):
    name, version, nonce = _get_package_info(compdef)
    return _get_package_target_from_name_version_nonce(name, version, nonce)


def emit_package_rule(rulefh, component, compdef, suite=None):
    if 'fileset' not in compdef:
        raise SyntaxError(f'{component}: Missing "fileset" in components.yaml')

    if suite:
        _fixup_suite_sdds_import(compdef)

    name, version, nonce = _get_package_info(compdef)
    target = _get_package_target_from_name_version_nonce(name, version, nonce)

    FILESET_TO_PKGTARGET[compdef['fileset']] = target
    compdef['package_name'] = name
    compdef['package_version'] = version
    compdef['package_nonce'] = nonce

    if target in PKGTARGET_TO_FILESET:
        if compdef['fileset'] != PKGTARGET_TO_FILESET[target]:
            say(f'duplicate package detected: {target}. '
                + f'Building from {PKGTARGET_TO_FILESET[target]}; ignoring {compdef["fileset"]}',
                level=logging.WARN)
        return
    PKGTARGET_TO_FILESET[target] = compdef['fileset']

    package = f'{target}.zip'
    prebuilt = os.path.join(BASE, compdef['fileset'], package)

    fileset = os.path.join(BASE, compdef['fileset'])
    emit_sdds_import_buildfile(fileset, os.path.basename(prebuilt))

    if os.path.exists(prebuilt):
        bazel_prebuilt_package = bazelize_sdds_import_target(fileset, 'prebuilt_package')
        print(f"""
copy_prebuilt_sdds3_package(
    name = "{target}",
    package = "package/{package}",
    prebuilt_package = "{bazel_prebuilt_package}",
)
""", file=rulefh)
    elif suite:
        bazel_sdds_import = bazelize_sdds_import_target(fileset, 'sdds_import')
        print(f"""
build_sdds3_package(
    name = "{target}",
    package = "package/{package}",
    version = "{version}",
    sdds_import = "{bazel_sdds_import}",
)
""", file=rulefh)
    else:
        bazel_sdds_import = bazelize_sdds_import_target(fileset, 'sdds_import')
        print(f"""
build_sdds3_package(
    name = "{target}",
    package = "package/{package}",
    sdds_import = "{bazel_sdds_import}",
)
""", file=rulefh)


def emit_package_rules(rulefh, sdds3_suites_def, common_component_data):
    for suite in sdds3_suites_def['suites']:
        suitedef = sdds3_suites_def['suites'][suite]
        emit_package_rule(rulefh, suite, common_component_data[suite], suite=suitedef)

        if 'subcomponents' not in suitedef:
            continue
        for component in suitedef['subcomponents']:
            compdef = common_component_data[component]
            emit_package_rule(rulefh, component, compdef)


def flatten_supplement_meta(comp, common_supplement_data):
    if 'supplements' not in comp:
        return
    old_list = comp['supplements']
    flat_list = []
    for sup in old_list:
        flat_list.append(common_supplement_data[sup])
    comp['supplements'] = flat_list


def flatten_suite_meta(suitemeta, suitedef):
    old_import = {}
    if 'import' in suitemeta:
        old_import = suitemeta['import']
        del suitemeta['import']
    for k in suitedef:
        suitemeta[k] = suitedef[k]
    for k in old_import:
        suitemeta[k] = old_import[k]


def add_subcomponents(suitemeta, sdds_imports, packages, common_component_data, common_supplement_data):
    if 'subcomponents' in suitemeta:
        for component in suitemeta['subcomponents']:
            compdef = common_component_data[component]
            target = _get_package_target_from_compdef_and_suite(compdef)
            fileset = PKGTARGET_TO_FILESET[target]
            compdef['fileset'] = fileset    # adjust the fileset to what we're actually using
            sdds_imports.append(bazelize_sdds_import_target(os.path.join(BASE, fileset), 'sdds_import'))
            packages.append(f':{target}')

            if suitemeta['subcomponents'][component] is None:
                suitemeta['subcomponents'][component] = {}
            for k in compdef:
                suitemeta['subcomponents'][component][k] = compdef[k]
            flatten_supplement_meta(suitemeta['subcomponents'][component], common_supplement_data)


def write_mock_sus_responses(launchdarkly_flags):
    mock_sus_dir = os.path.join(ROOT, 'output', 'sus')
    if os.path.exists(mock_sus_dir):
        shutil.rmtree(mock_sus_dir)
    os.makedirs(mock_sus_dir)

    for product in launchdarkly_flags:
        mock_sus_response = os.path.join(mock_sus_dir, f'mock_sus_response_{product}.json')

        suites = []
        for flag in launchdarkly_flags[product]:
            for tag in launchdarkly_flags[product][flag]:
                suite = launchdarkly_flags[product][flag][tag]['suite']
                if suite not in suites:
                    suites.append(suite)

        with open(mock_sus_response, 'w') as f:
            obj = {
                'suites': suites,
                'release-groups': [],
            }
            json.dump(obj, f, indent=2, sort_keys=True)


def add_launchdarkly_flag(launchdarkly_flags, sus, suitemeta, suite_src):
    subscription = suitemeta['line-id']
    suitekey = f'release.{sus}.{subscription}'

    if sus not in launchdarkly_flags:
        launchdarkly_flags[sus] = {}

    flags = launchdarkly_flags[sus]

    latest = {
        'suite': suite_src,
        'version': suitemeta['package_version'],
    }

    flags[suitekey] = {
        'RECOMMENDED': latest,
        'BETA': latest,
        'BETA2': latest,
    }


def write_launchdarkly_flags(launchdarkly_flags):
    flagsdir = os.path.join(ROOT, 'output', 'launchdarkly')
    if os.path.exists(flagsdir):
        shutil.rmtree(flagsdir)
    os.makedirs(flagsdir)
    for entry in launchdarkly_flags:
        for flag in launchdarkly_flags[entry]:
            mock_flag_value = os.path.join(flagsdir, f'{flag}.json')
            with open(mock_flag_value, 'w') as f:
                json.dump(launchdarkly_flags[entry][flag], f, indent=2, sort_keys=True)


def emit_suite_rules(rulefh, sdds3_suites_def, common_component_data, common_supplement_data):
    # Generate LaunchDarkly flags content so we can configure the dev version of LD.
    launchdarkly_flags = {}

    for suite in sdds3_suites_def['suites']:
        # To build a suite, we need:
        #
        # 1. The suite's metadata (its version/description, and where to attach supplements)
        #    => we emit this right now and declare it as an input
        #    => also note that emit_package_rules() updated the common_component_data with names/versions/nonces
        # 2. All subcomponents' SDDS-Import.xml files (to build the suite's metadata)
        # 3. All subcomponents' SDDS3 packages (to include their size/sha256)

        bazeltarget = suite.replace('-', '_')
        suitemeta = sdds3_suites_def['suites'][suite]

        sdds_imports = []
        packages = []

        suite_target = _get_package_target_from_compdef_and_suite(common_component_data[suite])
        sdds_imports.append(bazelize_sdds_import_target(
            os.path.join(BASE, PKGTARGET_TO_FILESET[suite_target]), 'sdds_import'))
        packages.append(f':{suite_target}')

        add_subcomponents(suitemeta, sdds_imports, packages, common_component_data, common_supplement_data)

        flatten_supplement_meta(suitemeta, common_supplement_data)
        flatten_suite_meta(suitemeta, common_component_data[suite])

        suitemeta['marketing_version'] = suitemeta['package_version']

        meta = f'{BASE}/.output/suite_{bazeltarget}_metadata.yaml'
        with open(meta, 'w') as metafh:
            yaml.dump(suitemeta, metafh)

        suite_src = f"sdds3.{suitemeta['package_name']}_{suitemeta['package_version']}.{hash_file(meta)[:10]}.dat"

        print(f"""
filegroup(
    name = "suite_{bazeltarget}_metadata",
    srcs = [".output/suite_{bazeltarget}_metadata.yaml"],
)
build_sdds3_suite(
    name = "suite_{bazeltarget}",
    suite = "suite/{suite_src}",
    metadata = ":suite_{bazeltarget}_metadata",
    sdds_imports = {sdds_imports},
    packages = {packages},
)
""", file=rulefh)

        for product in suitemeta['sus']:
            add_launchdarkly_flag(launchdarkly_flags, product, suitemeta, suite_src)

    write_launchdarkly_flags(launchdarkly_flags)
    write_mock_sus_responses(launchdarkly_flags)


def emit_supplement_rules(rulefh, sdds3_supplements_def, common_component_data):
    for sup in sdds3_supplements_def['supplements']:
        metadata = sdds3_supplements_def['supplements'][sup]
        for component in metadata['components']:

            tags = []
            for tag in metadata['components'][component]['release-tags']:
                tags.append(tag['tag'])

            bazeltarget = component.replace('-', '_')

            compdef = common_component_data[component]
            emit_package_rule(rulefh, component, compdef)

            supplement_name = compdef['line-id']

            sdds_imports = []
            packages = []
            target = _get_package_target_from_compdef_and_suite(compdef)
            sdds_imports.append(bazelize_sdds_import_target(
                os.path.join(BASE, PKGTARGET_TO_FILESET[target]), 'sdds_import'))
            packages.append(f':{target}')

            print(f"""
build_sdds3_supplement(
    name = "supplement_{bazeltarget}",
    supplement = "supplement/sdds3.{supplement_name}.dat",
    sdds_imports = {sdds_imports},
    packages = {packages},
    release_tags = {tags},
)
""", file=rulefh)


@retry(exceptions=subprocess.CalledProcessError, tries=5, delay=2, backoff=2)
def sync_sdds2_supplement(supplement, catalogue):
    subprocess.run([
        sys.executable, '-u', f'{TOOLS}/import_sdds2_supplements.py',
        '--mirror', f'{ROOT}/inputs/{supplement}',
        '--catalogue', catalogue,
    ], check=True)


def emit_sdds2_supplement_rules(rulefh):
    supplements = {
        'hmpa_data': 'https://d3.sophosupd.com/update/catalogue/sdds.hmpa_data.xml',
        'epips_data': 'https://d3.sophosupd.com/update/catalogue/sdds.epips_data.xml',
        'behave': 'https://d3.sophosupd.com/update/catalogue/sdds.behave.xml',
        'ixdata': 'https://d3.sophosupd.com/update/catalogue/sdds.ixdata.xml',
        'FIMFEED': 'https://d1.sophosupd.com/update/catalogue/sdds.FIMFEED.xml',
        'SLDFEED_d1': 'https://d1.sophosupd.com/update/catalogue/sdds.SLDFEED_d1.xml',
        'ml_models': 'https://d3.sophosupd.com/update/catalogue/sdds.ml_models.xml',
        'doc_models': 'https://d3.sophosupd.com/update/catalogue/sdds.doc_models.xml',
    }
    for supplement in supplements:
        # For each rigidname that appears in the SDDS2 supplement warehouse,
        # the script downloads every fileset which has a tag, and generates
        # an SDDS-Import.xml file for each.
        #
        # The filesets are written to {ROOT}/inputs/{supplement}/{rigidName}/{version}
        #
        # The mirror script also writes a description of which tags to apply
        # to each fileset, at {ROOT}/inputs/{supplement}/{rigidName}/meta.yaml
        #
        # Example file:
        #
        # ---
        # versions:
        #     "1.0.0.2": ['LATEST', ...]
        #     "1.0.0.3": ['PREVIOUS', ...]
        sync_sdds2_supplement(supplement, supplements[supplement])

        for path in glob(fr'{ROOT}/inputs/{supplement}/*'):
            rigidname = os.path.basename(path)

            meta = yaml.safe_load(open(fr'{ROOT}/inputs/{supplement}/{rigidname}/meta.yaml').read())

            sdds_imports = []
            packages = []
            tags = []

            for ver in meta['versions']:
                fileset = fr'{ROOT}/inputs/{supplement}/{rigidname}/{ver}'

                nonce = hash_file(fr'{fileset}/SDDS-Import.xml')[:10]
                package = f'{rigidname}_{ver}.{nonce}.zip'

                emit_sdds_import_buildfile(fileset, 'noPrebuiltPackage')
                bazel_sdds_import = bazelize_sdds_import_target(fileset, 'sdds_import')
                print(f"""
build_sdds3_package(
    name = "{rigidname}_{ver}",
    package = "package/{package}",
    sdds_import = "{bazel_sdds_import}",
)
""", file=rulefh)

                sdds_imports.append(bazel_sdds_import)
                packages.append(f':{rigidname}_{ver}')
                tags.append(','.join(meta['versions'][ver]))

            print(f"""
build_sdds3_supplement(
    name = "supplement_{rigidname}",
    supplement = "supplement/sdds3.{rigidname}.dat",
    sdds_imports = {sdds_imports},
    packages = {packages},
    release_tags = {tags},
)
""", file=rulefh)


def emit_bazel_build_files():
    # Slurp in all of the definitions
    common_component_data = yaml.safe_load(open(fr'{ROOT}\def\common\components.yaml').read())
    common_supplement_data = yaml.safe_load(open(fr'{ROOT}\def\common\supplements.yaml').read())
    sdds3_suites_def = yaml.safe_load(open(fr'{ROOT}\def\sdds3-suites-def.yaml').read())
    sdds3_supplements_def = yaml.safe_load(open(fr'{ROOT}\def\sdds3-supplements-def.yaml').read())

    with open(f'{BASE}/BUILD', 'w') as build:
        print("""####################################################################################
# AUTO-GENERATED BUILD FILE. DO NOT EDIT.
load("//sdds3/tools:tools.bzl",
    "build_sdds3_package",              # build sdds3 package from an input's SDDS-Import.xml
    "copy_prebuilt_sdds3_package",      # copy a prebuilt sdds3 package from an input
    "build_sdds3_suite",                # build sdds3 suite
    "build_sdds3_supplement",           # build sdds3 supplement
)
####################################################################################
""", file=build)

        say('Generating rules for in-house packages')
        emit_package_rules(build, sdds3_suites_def, common_component_data)
        say('Generating rules for in-house suites')
        emit_suite_rules(build, sdds3_suites_def, common_component_data, common_supplement_data)
        say('Generating rules for in-house supplements')
        emit_supplement_rules(build, sdds3_supplements_def, common_component_data)
        say('Generating rules for external SDDS2 supplements')
        emit_sdds2_supplement_rules(build)


def cleanup_output_folders():
    sdds3_output = os.path.join(BASE, '.output')
    if os.path.exists(sdds3_output):
        shutil.rmtree(sdds3_output)
    os.makedirs(sdds3_output)

    top_output = os.path.join(ROOT, 'output')
    if os.path.exists(top_output):
        shutil.rmtree(top_output)
    os.makedirs(top_output)


def main():
    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
    cleanup_output_folders()
    emit_bazel_build_files()


if __name__ == '__main__':
    main()
