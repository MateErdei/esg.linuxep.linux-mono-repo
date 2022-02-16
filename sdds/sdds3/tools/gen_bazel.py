#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020-2021 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Generate bazel rules to create SDDS3 packages and suites from YAML defs"""

# pylint: disable=C0302     # too many lines (>1000)

import xml.etree.ElementTree as ET

from glob import glob

import argparse
import codecs
import copy
import hashlib
import json
import logging
import os
import re
import shutil
import subprocess
import sys
import time
import zipfile

import requests
import yaml

from retry import retry

ARTIFACTORY_URL = f'https://{os.environ["TAP_PROXY_ARTIFACT_AUTHORITY_EXTERNAL"]}/artifactory' \
                  if 'TAP_PROXY_ARTIFACT_AUTHORITY_EXTERNAL' in os.environ \
                  else 'https://artifactory.sophos-ops.com'
SDDS3_URL = f'https://{os.environ.get("SDDS3_PROXY_AUTHORITY", "sdds3.sophosupd.com")}'
SESSION = requests.Session()
HEADERS = {}

TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)
ROOT = os.path.dirname(BASE)
ARTIFACT_CACHE = os.path.join(ROOT, 'inputs', 'artifact')

VERSION = yaml.safe_load(open(os.path.join(ROOT, "def-sdds3", "version.yaml")).read())['version']

# Map of fileset -> package target
FILESET_TO_PKGTARGET = {}
# Map of package target -> fileset
PKGTARGET_TO_FILESET = {}


def say(msg, level=logging.INFO):
    logging.log(level=level, msg=msg)


def set_artifactory_auth_headers():
    if 'BUILD_JWT_PATH' in os.environ:
        with codecs.open(os.environ['BUILD_JWT_PATH'], 'r', encoding='utf-8') as f:
            jwt = f.read().strip()
        HEADERS['Authorization'] = f'Bearer {jwt}'


def copy_file_or_url(origin, target):
    if origin.startswith('https://'):
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

    if os.path.exists(origin):
        # say(f'copying {origin} -> {target}')
        topdir = os.path.dirname(target)
        os.makedirs(topdir, exist_ok=True)
        shutil.copy(origin, target)
        return True
    return False


def hash_file(name):
    sha256 = hashlib.sha256()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    return sha256.hexdigest()


EMITTED_BUILDFILES = {}


def emit_buildfile_for_imported_fileset(fileset, **kwargs):
    print("JAKE1B")
    buildfile = os.path.join(fileset, 'BUILD')
    if buildfile in EMITTED_BUILDFILES:
        raise FileExistsError(f'Error: BUILD file already exists {buildfile}')
    EMITTED_BUILDFILES[buildfile] = True
    with open(buildfile, 'w') as f:
        print(f"""# AUTO-GENERATED BUILD FILE. DO NOT EDIT.
filegroup(
    name = "sdds_import",
    srcs = ["SDDS-Import.xml"],
    visibility = ["//visibility:public"],
)
""", file=f)
        for key in kwargs:
            val = kwargs[key]
            print(f"""filegroup(
    name = "{key}",
    srcs = ["{val}"],
    visibility = ["//visibility:public"],
)
""", file=f)


def bazelize_sdds_import_target(fileset, target):
    abspath = os.path.abspath(fileset)
    relpath = os.path.relpath(abspath, ROOT).replace(os.path.sep, '/')
    return f"//{relpath}:{target}"


def _get_branch():
    branch = 'unknown branch'
    if 'SOURCE_CODE_BRANCH' in os.environ:
        branch = os.environ['SOURCE_CODE_BRANCH']
    else:
        result = subprocess.run(
            ['git', 'rev-parse', '--abbrev-ref', 'HEAD'],
            check=False,
            capture_output=True,
            universal_newlines=True,
            encoding='utf-8',
        )
        branch = result.stdout.strip()
    return branch


AUTOVERSION_CACHE = {}


def _autoversion(comp, baseversion, suffix):
    if not baseversion.endswith('.'):
        return baseversion

    cache_key = (comp, baseversion)
    if cache_key not in AUTOVERSION_CACHE:
        try:
            result = subprocess.run(
                ['versioning_client', '-c', comp, '-v', baseversion[:-1]],
                check=True,
                capture_output=True,
                universal_newlines=True,
                encoding='utf-8',
            )
            AUTOVERSION_CACHE[cache_key] = result.stdout.strip()
        except subprocess.CalledProcessError as e:
            say(f'WARNING: autoversion failed: {e} ==> using fallback')
            AUTOVERSION_CACHE[cache_key] = f'{baseversion[:-1]}.9999'
    if suffix is None:
        return AUTOVERSION_CACHE[cache_key]
    return f'{AUTOVERSION_CACHE[cache_key]}.{suffix}'


def create_suite_autoversion(suitedef, suite, mode, multiple_instances):
    """Define a new suite version, autoversioning as needed.

    This sets suite['def'] with the package_name and package_version.
    """

    # If we only have a single instance of this suite (i.e. there isn't an EAP going on that necessitates
    # multiple suites) then the suite will be versioned like so:
    #
    #     sdds3.SuiteName_{version.major}.{version.minor}.{version.patch}.{autoversion}.{nonce}.dat
    #
    # If there are multiple instances (i.e. multiple tags RECOMMENDED, BETA), then the suite should be versioned
    # like so:
    #
    #     sdds3.SuiteName_{version.major}.{version.minor}.{version.patch}.{autoversion}.{#instance}.{nonce}.dat
    #
    # This ensures you can always add an extra instance later in the dev cycle of a particular release, without
    # risk of overlapping versions from previous or later builds.

    # AutoVersion the suite itself
    if mode == 'dev':
        now = time.gmtime()
        suite['baseversion'] = f'{now.tm_year}.{now.tm_mon}.{now.tm_mday}.'
    else:
        suite['baseversion'] = f'{VERSION["major"]}.{VERSION["minor"]}.'

    line_id = suitedef['line-id']
    version = _autoversion(f'{mode}_suite_{line_id}', suite['baseversion'], suffix=None)

    fullversion = version
    if multiple_instances:
        fullversion = '.'.join([version, str(suite["_instance"])])

    suite['def'] = {
        'line-id': line_id,
        'package_name': line_id,
        'package_version': fullversion,
    }

    if 'marketing_version' not in suite:
        raise NameError(f'{line_id}_{version}: missing marketing_version (required for prod builds)')
    if '{version}' not in suite['marketing_version'] and '{fullversion}' not in suite['marketing_version']:
        raise NameError(f'{line_id}_{version}: marketing_version does not contain {{version}} or {{fullversion}}')
    if mode == 'dev':
        suite['marketing_version'] = f"{version} ({_get_branch()})"
    else:
        suite['marketing_version'] = suite['marketing_version'].format(
            version=version,
            fullversion=fullversion,
            sprint=f'SPRINT {VERSION["sprint"]}' if 'sprint' in VERSION else '')

    suite['marketing_version'] = re.sub(pattern=r'\s+', string=suite['marketing_version'], repl=' ').strip()

    say(f'Suite {line_id} {suite["def"]["package_version"]}: marketing version {suite["marketing_version"]}')


# Keep track of allocated suite packages, by fileset.
SUITE_PACKAGES = {
    'line-id': {
        'count': 1,
        'fileset': 'version',
    }
}


def create_suite_package(compdef, suite, view, mode):
    """Define a new suite *package* version, autoversioning as needed.

    There can be fewer packages than instances of suites, because several suite
    instances can reuse the same package binary.

    Or, if there is a single suite with multiple different package filesets,
    there can be more suite packages than suites.

    This sets view['def'] with the information needed to reference the built
    package, but uses copy.deepcopy() to ensure that it has a unique set of
    platforms and features.
    """
    line_id = compdef['line-id']
    if line_id not in SUITE_PACKAGES:
        SUITE_PACKAGES[line_id] = {
            'count': 0,
        }

    fileset = compdef['fileset']
    if fileset not in SUITE_PACKAGES[line_id]:
        now = time.gmtime()
        version = _autoversion(
            comp=f'{mode}_suitepkg_{line_id}',
            baseversion=f'{now.tm_year}.{now.tm_mon}.{now.tm_mday}.' if mode == 'dev' else suite['baseversion'],
            suffix=f'.{SUITE_PACKAGES[line_id]["count"]}' if SUITE_PACKAGES[line_id]["count"] > 0 else None)
        SUITE_PACKAGES[line_id][fileset] = version
        SUITE_PACKAGES[line_id]['count'] += 1

        # Edit the suite's SDDS-Import.xml file to make suites work properly
        # Read the SDDS-Import.xml file to grok the name, version and nonce
        sdds_import = os.path.join(BASE, compdef['fileset'], 'SDDS-Import.xml')
        with open(sdds_import) as f:
            xml = ET.fromstring(f.read())

        pkg_version = version
        pkg_fileset = os.path.join(BASE, os.path.dirname(fileset), pkg_version)
        if os.path.exists(pkg_fileset):
            shutil.rmtree(pkg_fileset)
        shutil.copytree(src=os.path.dirname(sdds_import), dst=pkg_fileset)

        say(f'{line_id} {pkg_version}: setting name/line-id to {line_id}')
        xml.find('Component/RigidName').text = line_id
        xml.find('Component/Name').text = line_id
        say(f'{line_id} {pkg_version}: setting version to {pkg_version}')
        xml.find('Component/Version').text = pkg_version

        for f in xml.findall('Component/FileList/File'):
            if 'Offset' not in f.attrib and f.attrib['Name'] == 'version':
                say(f'{line_id}: setting version file to {suite["marketing_version"]}')
                version_file = os.path.join(BASE, pkg_fileset, 'version')
                with open(version_file, 'w') as f:
                    f.write(suite['marketing_version'])
                break

        sdds_import = os.path.join(pkg_fileset, 'SDDS-Import.xml')
        with open(sdds_import, 'wb') as f:
            ET.ElementTree(element=xml).write(f, encoding='UTF-8', xml_declaration=True)

    # Set things up so that maker.py can identify the platforms and supplements of this
    # particular view's suite package.
    view['def'] = copy.deepcopy(compdef)
    view['def']['platforms'] = view['platforms']
    view['def']['supplements'] = view['supplements'] if 'supplements' in view else []
    pkg_version = SUITE_PACKAGES[line_id][fileset]
    pkg_fileset = os.path.join(BASE, os.path.dirname(fileset), pkg_version)
    view['def']['fileset'] = os.path.relpath(pkg_fileset, BASE)
    view['def']['description'] = suite['marketing_version']


def _get_package_info_from_sdds_import_xml(compdef):
    # Read the SDDS-Import.xml file to grok the name, version and nonce
    sdds_import = os.path.join(BASE, compdef['fileset'], 'SDDS-Import.xml')
    nonce = hash_file(sdds_import)[:10]
    with open(sdds_import) as f:
        xml = ET.ElementTree(None, f)
    name = xml.findtext('Component/Name').replace(' ', '')
    version = xml.findtext('Component/Version')
    return name, version, nonce


def _get_package_info_from_prebuilt(prebuilt):
    # Parse a prebuilt SDDS3 package filename to extract the name, version and nonce
    match = re.match(r'(\S+)_([\d.]+)\.(\w+)\.zip', prebuilt)
    if match is None:
        raise NameError(f'Could not extract name/version/nonce from prebuilt SDDS3 package {prebuilt}')
    return match[1], match[2], match[3]


def _get_package_target_from_name_version_nonce(name, version, nonce):
    return f'{name}_{version}.{nonce}'


def _get_package_target_from_compdef_and_suite(compdef):
    name, version, nonce = _get_package_info_from_sdds_import_xml(compdef)
    return _get_package_target_from_name_version_nonce(name, version, nonce)


def emit_copy_prebuilt_package_rule(rulefh, component, compdef, prebuilt, package_folder='package'):
    if 'fileset' not in compdef:
        raise SyntaxError(f'{component}: Missing "fileset" in components.yaml')
    prebuilt_package = os.path.join(BASE, compdef['fileset'], prebuilt)
    if not os.path.exists(prebuilt_package):
        raise FileNotFoundError(f'{component}: Missing prebuilt package {prebuilt_package}')

    name, version, nonce = _get_package_info_from_prebuilt(prebuilt)
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

    fileset = os.path.join(BASE, compdef['fileset'])

    bazel_prebuilt_package = bazelize_sdds_import_target(fileset, f'prebuilt_{name}_{version}.{nonce}')
    print(f"""
copy_prebuilt_sdds3_package(
    name = "{target}",
    package = "{package_folder}/{prebuilt}",
    prebuilt_package = "{bazel_prebuilt_package}",
)
""", file=rulefh, flush=True)


def emit_package_rule(rulefh, component, compdef, package_folder='package'):
    if 'fileset' not in compdef:
        raise SyntaxError(f'{component}: Missing "fileset" in components.yaml')
    if 'prebuilt' in compdef:
        emit_copy_prebuilt_package_rule(rulefh, component, compdef, compdef['prebuilt'], package_folder)
        return

    name, version, nonce = _get_package_info_from_sdds_import_xml(compdef)
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
    emit_buildfile_for_imported_fileset(fileset, prebuilt_package=os.path.basename(prebuilt))

    if os.path.exists(prebuilt):
        bazel_prebuilt_package = bazelize_sdds_import_target(fileset, 'prebuilt_package')
        print(f"""
copy_prebuilt_sdds3_package(
    name = "{target}",
    package = "{package_folder}/{package}",
    prebuilt_package = "{bazel_prebuilt_package}",
)
""", file=rulefh, flush=True)
    else:
        bazel_sdds_import = bazelize_sdds_import_target(fileset, 'sdds_import')
        print(f"""
build_sdds3_package(
    name = "{target}",
    package = "{package_folder}/{package}",
    sdds_import = "{bazel_sdds_import}",
)
""", file=rulefh, flush=True)


def emit_package_rules(rulefh, suites, common_component_data, mode):
    for suite in suites:
        suitedef = suites[suite]

        for i, instance in enumerate(suitedef['instances']):
            instance['_instance'] = i

            # Define a new suite version. This uses the suite's line-id and the instance's autoversion.
            create_suite_autoversion(suitedef, instance, mode, multiple_instances=(len(suitedef['instances']) > 1))

            for j, view in enumerate(instance['views']):
                instance['_view'] = j

                create_suite_package(common_component_data[view['component']], instance, view, mode)

                emit_package_rule(rulefh, suite, view['def'])

                if 'subcomponents' not in view:
                    continue
                for component in view['subcomponents']:
                    compdef = common_component_data[component]
                    emit_package_rule(rulefh, component, compdef)


def add_subcomponents(suitemeta, sdds_imports, packages, common_component_data):
    for view in suitemeta['views']:
        if 'subcomponents' not in view:
            continue
        if 'platforms' not in view:
            raise NameError(f'Error: missing platforms list in {view}')

        for component in view['subcomponents']:
            compdef = common_component_data[component]
            target = _get_package_target_from_compdef_and_suite(compdef)
            fileset = PKGTARGET_TO_FILESET[target]
            compdef['fileset'] = fileset    # adjust the fileset to what we're actually using

            sdds_import = bazelize_sdds_import_target(os.path.join(BASE, fileset), 'sdds_import')
            if sdds_import not in sdds_imports:
                sdds_imports.append(sdds_import)
            if f':{target}' not in packages:
                packages.append(f':{target}')

            if view['subcomponents'][component] is None:
                view['subcomponents'][component] = {}
            for k in compdef:
                view['subcomponents'][component][k] = compdef[k]

            view['subcomponents'][component]['platforms'] = view['platforms']


def write_mock_sus_responses(launchdarkly_flags):
    mock_sus_dir = os.path.join(ROOT, 'output', 'sus')
    if os.path.exists(mock_sus_dir):
        shutil.rmtree(mock_sus_dir)
    os.makedirs(mock_sus_dir)

    for product in launchdarkly_flags:
        for tag in ('RECOMMENDED', 'BETA'):
            mock_sus_response = os.path.join(mock_sus_dir, f'mock_sus_response_{product}_{tag}.json')

            suites = []
            for flag in launchdarkly_flags[product]:
                flagdef = launchdarkly_flags[product][flag]
                flagtag = tag if tag in flagdef else 'RECOMMENDED'
                suite = flagdef[flagtag]['suite']
                if suite not in suites:
                    suites.append(suite)

            with open(mock_sus_response, 'w') as f:
                obj = {
                    'suites': suites,
                    'release-groups': ["0"],
                }
                json.dump(obj, f, indent=2, sort_keys=True)


def add_launchdarkly_flag(launchdarkly_flags, sus, tag, suitedef, suite_src):
    subscription = suitedef['line-id']
    suitekey = f'release.{sus}.{subscription}'

    if sus not in launchdarkly_flags:
        launchdarkly_flags[sus] = {}
    flags = launchdarkly_flags[sus]
    if suitekey not in flags:
        flags[suitekey] = {}
    flags[suitekey][tag] = {
        'suite': suite_src,
        'version': suitedef['package_version'],
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


# pylint: disable=R0914     # too many local variables. Honestly.
def emit_suite_rules(rulefh, suites, common_component_data):
    # Generate LaunchDarkly flags content so we can configure the dev version of LD.
    launchdarkly_flags = {}

    for suite in suites:
        suitemeta = suites[suite]
        for instance in suitemeta['instances']:
            # To build a suite, we need:
            #
            # 1. The suite's metadata (its version/description, and where to attach supplements)
            #    => we emit this right now and declare it as an input
            #    => also note that emit_package_rules() updated the common_component_data with names/versions/nonces
            # 2. All subcomponents' SDDS-Import.xml files (to build the suite's metadata)
            # 3. All subcomponents' SDDS3 packages (to include their size/sha256)

            sdds_imports = []
            packages = []

            for view in instance['views']:
                view_target = _get_package_target_from_compdef_and_suite(view['def'])
                sdds_import = bazelize_sdds_import_target(
                    os.path.join(BASE, PKGTARGET_TO_FILESET[view_target]), 'sdds_import')
                if sdds_import not in sdds_imports:
                    sdds_imports.append(sdds_import)
                    packages.append(f':{view_target}')

            add_subcomponents(instance, sdds_imports, packages, common_component_data)

            suite_name = f'{instance["def"]["package_name"]}_{instance["def"]["package_version"]}'
            meta = os.path.join(BASE, ".output", f"{suite_name}_metadata.yaml")
            with open(meta, 'w') as metafh:
                print(f"dumping yaml to {meta}")
                yaml.dump(instance, metafh)

            suite_src = f"sdds3.{suite_name}.{hash_file(meta)[:10]}.dat"

            print(f"""
filegroup(
    name = "suite_{suite_name}_metadata",
    srcs = [".output/{suite_name}_metadata.yaml"],
)
build_sdds3_suite(
    name = "suite_{suite_name}",
    suite = "suite/{suite_src}",
    metadata = ":suite_{suite_name}_metadata",
    sdds_imports = {sdds_imports},
    packages = {packages},
)
""", file=rulefh, flush=True)

            for product in suitemeta['sus']:
                for tag in instance['tags']:
                    add_launchdarkly_flag(launchdarkly_flags, product, tag, instance['def'], suite_src)

    write_launchdarkly_flags(launchdarkly_flags)
    write_mock_sus_responses(launchdarkly_flags)


def emit_supplement_rules(rulefh, sdds3_supplements_def, common_component_data, build_mode):
    # SDDS3 supplements are named f'sdds3.{line-id}.dat', unlike SDDS2, which can have
    # arbitrary lines stored in the same supplement file.
    #
    # Group the supplements by line, then emit rules for each line.
    supplements = {}

    package_folder = 'package'
    # supplements in different folder for prod to upload in build-asset
    if build_mode == 'prod':
        package_folder = 'supplement_package'

    for sup in sdds3_supplements_def['supplements']:
        metadata = sdds3_supplements_def['supplements'][sup]

        for component in metadata['components']:
            compdef = common_component_data[component]
            emit_package_rule(rulefh, component, compdef, package_folder)

            supplement_name = compdef['line-id']
            if supplement_name not in supplements:
                supplements[supplement_name] = {
                    'prebuilt': metadata['prebuilt'] if 'prebuilt' in metadata else None,
                    'sdds_imports': [],
                    'packages': [],
                    'tags': [],
                }

            supdef = supplements[supplement_name]
            if supdef['prebuilt']:
                continue

            mytags = []
            for tag in metadata['components'][component]['release-tags']:
                mytags.append(tag['tag'])
            supdef['tags'].append(','.join(mytags))

            target = _get_package_target_from_compdef_and_suite(compdef)
            supdef['sdds_imports'].append(bazelize_sdds_import_target(
                os.path.join(BASE, PKGTARGET_TO_FILESET[target]), 'sdds_import'))
            supdef['packages'].append(f':{target}')

    for supplement_name in supplements:
        supdef = supplements[supplement_name]
        bazeltarget = supplement_name.replace('-', '_')
        if supdef['prebuilt']:
            prebuilt = supdef['prebuilt']
            prebuilt_target = bazelize_sdds_import_target(os.path.dirname(prebuilt), 'prebuilt_supplement')
            print(f"""
copy_prebuilt_sdds3_supplement(
    name = "supplement_{bazeltarget}",
    supplement = "supplement/sdds3.{supplement_name}.dat",
    prebuilt_supplement = "{prebuilt_target}",
)
""", file=rulefh, flush=True)
        else:
            print(f"""
build_sdds3_supplement(
    name = "supplement_{bazeltarget}",
    supplement = "supplement/sdds3.{supplement_name}.dat",
    sdds_imports = {supdef['sdds_imports']},
    packages = {supdef['packages']},
    release_tags = {supdef['tags']},
)
""", file=rulefh, flush=True)


def import_scit_supplement(fromdir, supplements, components):
    base_content_dir = f'{fromdir}/content'
    scit_manifest = f'{fromdir}/manifest.xml'
    if not os.path.exists(scit_manifest):
        raise FileNotFoundError(f'Error: cannot import scit supplement manifest: {scit_manifest}')
    with open(scit_manifest) as f:
        manifest = ET.fromstring(f.read())

    supname = manifest.attrib['name']
    sup = {}
    supplements['supplements'][supname] = {'components': sup}

    say(f'Importing {supname} supplement from {fromdir}')

    now = time.gmtime()
    baseversion = f'{now.tm_year}.{now.tm_mon}.{now.tm_mday}.'
    suffix = 0
    for pkg in manifest.findall(f'package'):
        suffix += 1
        if 'version' in pkg.attrib:
            version = pkg.attrib['version']
        else:
            version = _autoversion(supname, baseversion, suffix)

        tags = [t.attrib['name'] for t in pkg.findall('tags/tag')]

        # The name of this package is the first tag (by convention)
        name = tags[0]
        sdds_import_xml = f'{base_content_dir}/{name}/SDDS-Import.xml'

        # Define the component
        comp_id = f'{supname}_{name}'
        components[comp_id] = {
            'fileset': os.path.relpath(os.path.dirname(sdds_import_xml), BASE),
            'line-id': supname,
        }

        # Append this component to the repairkit supplement
        sup[comp_id] = {
            'release-tags': [{'tag': t} for t in tags]
        }

        # Now generate an SDDS-Import.xml for this package
        root = ET.Element('ComponentData')
        comp = ET.SubElement(root, 'Component')
        ET.SubElement(comp, 'Name').text = supname
        ET.SubElement(comp, 'RigidName').text = supname
        ET.SubElement(comp, 'Version').text = version
        files = ET.SubElement(comp, 'FileList')
        for entry in pkg.findall('files/file'):
            package_path = entry.attrib['package-path']

            filenode = ET.SubElement(files, 'File')
            filenode.attrib['Name'] = os.path.basename(package_path)
            if '/' in package_path:
                filenode.attrib['Offset'] = os.path.dirname(package_path).replace(os.sep, '/')
            filenode.attrib['Size'] = entry.attrib['size']
        # Generate the description
        dictionary = ET.SubElement(root, 'Dictionary')
        dname = ET.SubElement(dictionary, 'Name')
        label = ET.SubElement(dname, 'Label')
        ET.SubElement(label, 'Token').text = f'{supname}#{version}'
        lang = ET.SubElement(label, 'Language', {"lang": "en"})
        ET.SubElement(lang, 'ShortDesc').text = version
        ET.SubElement(lang, 'LongDesc').text = f'{supname} Supplement Data v{version}'
        with open(sdds_import_xml, 'wb') as f:
            ET.ElementTree(element=root).write(f, encoding='UTF-8', xml_declaration=True)

        # Write the bazel BUILD file for this package
        emit_buildfile_for_imported_fileset(os.path.dirname(sdds_import_xml))


@retry(exceptions=subprocess.CalledProcessError, tries=5, delay=2, backoff=2)
def sync_sdds3_supplement(supplement, syncdir):
    args = [
        sys.executable, '-u', f'{TOOLS}/sync_sdds3_supplement.py',
        '--mirror', syncdir,
        '--supplement', f'{SDDS3_URL}/supplement/sdds3.{supplement}.dat',
        '--builder', f'{ROOT}/redist/sdds3/sdds3-builder',
    ]
    say(f"Running {args}")
    subprocess.run(args, check=True)
    return syncdir


def load_sdds3_supplement(supplement):
    args = [
        os.path.join(ROOT, "redist", "sdds3", "sdds3-builder"),
        '--unpack-signed-file',
        supplement
    ]
    result = subprocess.run(
        args,
        check=True,
        capture_output=True,
        universal_newlines=True,
        encoding='utf-8',
    )
    return ET.fromstring(result.stdout)


def import_external_supplements(supplements, components):
    say(f'Importing external supplements')
    if 'external_supplements' not in supplements['supplements']:
        return

    external_supplements = supplements['supplements']['external_supplements']
    del supplements['supplements']['external_supplements']

    for supplement in external_supplements:
        syncdir = f'{ROOT}/inputs/supplements/{supplement}'
        sync_sdds3_supplement(supplement, syncdir)

        info = {
            'prebuilt': f'{syncdir}/supplement/sdds3.{supplement}.dat',
            'components': {},
        }
        supplements['supplements'][supplement] = info
        emit_buildfile_for_imported_fileset(f'{syncdir}/supplement', prebuilt_supplement=f'sdds3.{supplement}.dat')

        # Load the embedded supplement XML so that we can extract the release-tags (for validating supplement-refs)
        root = load_sdds3_supplement(f'{syncdir}/supplement/sdds3.{supplement}.dat')

        bazel_targets = {}
        for pkg in root.findall('package-ref[@src]'):
            src = pkg.attrib['src']
            name, version, nonce = _get_package_info_from_prebuilt(src)
            bazel_targets[f'prebuilt_{name}_{version}.{nonce}'] = src
            comp = src[:-4]   # strip .zip
            info['components'][comp] = {
                'prebuilt': f'{syncdir}/package/{src}',
                'release-tags': [{'tag': t.attrib['name']} for t in pkg.findall('tags/tag')]
            }
            components[comp] = {
                'fileset': os.path.relpath(f'{syncdir}/package', BASE),
                'line-id': supplement,
                'prebuilt': src,
            }
        emit_buildfile_for_imported_fileset(f'{syncdir}/package', **bazel_targets)


def import_internal_supplements(supplements, components):
    if 'internal_supplements' not in supplements['supplements']:
        return

    internal_supplements = supplements['supplements']['internal_supplements']
    del supplements['supplements']['internal_supplements']
    print("JAKE1")
    for path in internal_supplements:
        found = False
        print(f"JAKE1A: {f'{ROOT}/inputs/supplements/{path}'}")
        for manifest in glob(f'{ROOT}/inputs/supplements/{path}'):
            import_scit_supplement(os.path.dirname(manifest), supplements, components)
            found = True
        if not found:
            raise FileNotFoundError(f'Failed to import internal supplement {ROOT}/inputs/supplements/{path}')
    print("JAKE2")


def insert_telemetry_supplements(supplements, components):
    sdds_import = fr'{ROOT}/inputs/supplements/telemetrysup/SDDS-Import.xml'
    if not os.path.exists(sdds_import):
        raise FileNotFoundError(f"Missing TELEMSUP supplement: {sdds_import}")

    rigidname = 'TELEMSUP'
    comp_id = f'{rigidname}-DEV'

    # Define the component
    components[comp_id] = {
        'fileset': os.path.relpath(os.path.dirname(sdds_import), BASE),
        'line-id': rigidname,
    }

    # Define and/or append to the supplement
    supplements['supplements'][rigidname] = {'components': {}}
    supplements['supplements'][rigidname]['components'][comp_id] = {
        'release-tags': [{'tag': 'RECOMMENDED'}]
    }


def sanitize_artifact(artifact):
    if not artifact.startswith(('esg-releasable-candidate/', 'esg-build-store-trusted/')):
        raise NameError(f'artifactory_url: refusing to accept non-promoted artifact: {artifact}')
    if '//' in artifact:
        raise NameError(f'artifactory_url: refusing to accept url with two slashes: {artifact}')
    if artifact.endswith('/'):
        artifact = artifact[:-1]  # chop trailing slash
    return artifact


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


def download_and_extract_artifact(rel_artifact):
    destdir = os.path.join(ARTIFACT_CACHE, rel_artifact)
    if os.path.exists(destdir):
        return

    bld_artifact = convert_releasable_to_build_artifact(rel_artifact)
    bld_parts = bld_artifact.split('/')
    rel_parts = rel_artifact.split('/')
    while len(bld_parts) > 0:
        bld_zip = '/'.join(bld_parts) + '.zip'
        bld_file = os.path.join(ARTIFACT_CACHE, bld_zip)
        if copy_file_or_url(f'{ARTIFACTORY_URL}/{bld_zip}', bld_file):
            with zipfile.ZipFile(bld_file, 'r') as zfile:
                zfile.extractall(os.path.join(ARTIFACT_CACHE, '/'.join(rel_parts)))
                return
        bld_parts.pop()
        rel_parts.pop()
    raise NameError(f"download_artifact {rel_artifact}: not found")


def mirror_tree(src, dst):
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(src, dst)


def fetch_component_artifacts(common_component_data, mode):
    say(f'Using ARTIFACTORY_URL: {ARTIFACTORY_URL}')

    def _get_artifact(comp, compdef, field):
        artifact = sanitize_artifact(compdef[field])
        say(f'{comp}: downloading {artifact}')
        download_and_extract_artifact(artifact)
        importpath = os.path.join(ARTIFACT_CACHE, artifact)
        destdir = os.path.join(BASE, '..', 'inputs', comp.replace('.', os.sep))
        mirror_tree(importpath, destdir)
        compdef['fileset'] = os.path.relpath(destdir, BASE)
        del compdef[field]

    for comp in common_component_data:
        compdef = common_component_data[comp]

        if mode == 'prod' and comp.startswith('supplement.'):
            continue

        # If we're in 'dev' mode, we have three sources of inputs:
        #
        # 1. fileset: means `tap fetch` already put the latest package in the right place.
        # 2. artifact: means download the artifact and set `fileset` to the download location.
        # 3. import: means copy the filer location to the `fileset`.
        #
        # If we're in 'prod' mode, we have three sources of inputs:
        #
        # 1. prod-artifact: means download the artifact and set `fileset` to the download location.
        # 2. artifact: same as dev
        # 3. import: same as dev

        if mode == 'prod' and 'prod-artifact' in compdef:
            _get_artifact(comp, compdef, 'prod-artifact')
        elif 'artifact' in compdef:
            _get_artifact(comp, compdef, 'artifact')
        elif 'fileset' not in compdef:
            compdef['fileset'] = os.path.join('..', 'redist', comp.replace('.', os.sep))

        if not os.path.exists(os.path.join(BASE, compdef['fileset'])):
            raise FileNotFoundError(f'{comp}: fileset {compdef["fileset"]} not found')


def expand_supplement_refs(suites, supplement_refs):
    def _expand_supplements(supplements):
        for i, sup in enumerate(supplements):
            if isinstance(sup, str):
                if sup in supplement_refs:
                    supplements[i] = supplement_refs[sup]
                else:
                    raise NameError(f'Error: undefined supplement-ref {sup}')

    def _expand_supplements_in(thing):
        if thing and 'supplements' in thing:
            _expand_supplements(thing['supplements'])

    for suite in suites:
        for instance in suites[suite]['instances']:
            for view in instance['views']:
                _expand_supplements_in(view)
                print(view)
                if 'subcomponents' in view:
                    for comp in view['subcomponents']:
                        _expand_supplements_in(view['subcomponents'][comp])


def _assert_no_dangling_supplement_refs_in(thing, defined_supplements):
    if not thing or 'supplements' not in thing:
        return 0
    undefined = 0
    for sup in thing['supplements']:
        lineid = sup['line-id']
        tag = sup['tag']
        lookup = f'{lineid}#{tag}'
        if lookup not in defined_supplements:
            say(f'Undefined supplement-ref {lookup}: {sup}', level=logging.ERROR)
            undefined += 1
    return undefined


def assert_no_undefined_supplement_refs(suites, supplements, components):
    defined_supplements = {}
    for sup in supplements['supplements']:
        for pkg in supplements['supplements'][sup]['components']:
            if pkg not in components:
                raise NameError(f'Undefined supplement package {pkg}')
            pkgdef = components[pkg]
            lineid = pkgdef['line-id']
            tags = supplements['supplements'][sup]['components'][pkg]['release-tags']
            for tagelem in tags:
                tag = tagelem['tag']
                defined_supplements[f'{lineid}#{tag}'] = True

    undefined = 0
    for suite in suites:
        for instance in suites[suite]['instances']:
            for view in instance['views']:
                undefined += _assert_no_dangling_supplement_refs_in(view, defined_supplements)
                if 'subcomponents' in view:
                    for comp in view['subcomponents']:
                        undefined += _assert_no_dangling_supplement_refs_in(
                            view['subcomponents'][comp], defined_supplements)
    if undefined > 0:
        raise LookupError(f'ERROR: found {undefined} undefined supplement-refs')


def load_components():
    components = {}
    for path in glob(fr'{ROOT}/def-sdds3/components/*.yaml'):
        data = yaml.safe_load(open(path).read())
        if not isinstance(data, dict):
            raise TypeError(f'Error loading {path}: unexpected format')
        for comp in data:
            if comp in components:
                raise KeyError(f'Duplicate component {comp} defined in {path} (already in {components[comp]["_path"]})')

            compdata = data[comp]
            if compdata is None:
                compdata = {}
            compdata['_path'] = path
            components[comp] = compdata

    return components


def load_suites():
    suites = {}
    for path in glob(fr'{ROOT}/def-sdds3/suites/*.yaml'):
        data = yaml.safe_load(open(path).read())
        if not isinstance(data, dict):
            raise TypeError(f'Error loading {path}: unexpected format')
        suites[os.path.basename(path)] = data
    return suites


def emit_bazel_build_files(mode):
    # Slurp in all of the definitions
    common_component_data = load_components()
    suites = load_suites()
    supplements = yaml.safe_load(open(os.path.join(ROOT, "def-sdds3", "supplements.yaml")).read())
    supplement_refs = yaml.safe_load(open(os.path.join(ROOT, "def-sdds3", "supplement-refs.yaml")).read())

    expand_supplement_refs(suites, supplement_refs)

    with open(f'{BASE}/BUILD', 'w') as build:
        print("""####################################################################################
# AUTO-GENERATED BUILD FILE. DO NOT EDIT. SEE sdds3/tools/prep_dev.py
load("//sdds3/tools:tools.bzl",
    "build_sdds3_package",
    "copy_prebuilt_sdds3_package",
    "copy_prebuilt_sdds3_supplement",
    "build_sdds3_suite",
    "build_sdds3_supplement",
)
####################################################################################
""", file=build)

        say('Downloading legacy packages')
        fetch_component_artifacts(common_component_data, mode)

        say('Generating package rules')
        emit_package_rules(build, suites, common_component_data, mode)

        say('Generating suite rules')
        emit_suite_rules(build, suites, common_component_data)

        say('Generating supplement rules')
        import_internal_supplements(supplements, common_component_data)
        import_external_supplements(supplements, common_component_data)

        # NOT NEEDED FOR SSPL
        # insert_telemetry_supplements(supplements, common_component_data)
        emit_supplement_rules(build, supplements, common_component_data, mode)
        say('Ensuring that all supplement-refs are defined')
        assert_no_undefined_supplement_refs(suites, supplements, common_component_data)


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

    parser = argparse.ArgumentParser()
    parser.add_argument('--mode', choices=['dev', 'prod'], default='dev')
    args = parser.parse_args()

    set_artifactory_auth_headers()
    cleanup_output_folders()
    emit_bazel_build_files(args.mode)


if __name__ == '__main__':
    main()
