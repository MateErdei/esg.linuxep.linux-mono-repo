#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
# ------------------------------------------------------------------------------
"""Generate bazel rules to create SDDS3 packages and suites from pubspecs"""

import xml.etree.ElementTree as ET

from timeit import default_timer as timer

import codecs
import hashlib
import io
import json
import logging
import os
import shutil
import subprocess
import tempfile
import zipfile

from retry import retry

import requests
import yaml

ARTIFACTORY_URL = f'https://{os.environ["TAP_PROXY_ARTIFACT_AUTHORITY_EXTERNAL"]}/artifactory' \
                  if 'TAP_PROXY_ARTIFACT_AUTHORITY_EXTERNAL' in os.environ \
                  else 'https://artifactory.sophos-ops.com'
SESSION = requests.Session()
HEADERS = {}

TOOLS = os.path.dirname(os.path.abspath(__file__))
BASE = os.path.dirname(TOOLS)
ROOT = os.path.dirname(BASE)


def say(msg, level=logging.INFO):
    logging.log(msg=msg, level=level)


def set_artifactory_auth_headers():
    if 'BUILD_JWT_PATH' in os.environ:
        with codecs.open(os.environ['BUILD_JWT_PATH'], 'r', encoding='utf-8') as f:
            jwt = f.read().strip()
        HEADERS['Authorization'] = f'Bearer {jwt}'


def hash_bytes(blob):
    sha256 = hashlib.sha256()
    sha256.update(blob)
    return sha256.hexdigest()


def hash_string(string):
    return hash_bytes(string.encode('utf-8'))


def hash_file(name):
    sha256 = hashlib.sha256()
    with open(name, 'rb') as f:
        for byte_block in iter(lambda: f.read(65536), b""):
            sha256.update(byte_block)
    return sha256.hexdigest()


def nonce_from_sha256(sha256):
    return sha256[:10]


def load_xml_from(path, filename):
    return ET.fromstring(read_utf8_file(os.path.join(path, filename)))


def load_yaml_from(path, filename):
    return yaml.safe_load(read_utf8_file(os.path.join(path, filename)))


def read_utf8_url(url):
    resp = SESSION.get(url, headers=HEADERS)
    if resp:
        return resp.text
    say(f'{resp.status_code} {resp.reason}: GET {url}')
    raise requests.HTTPError(request=resp.request, response=resp.status_code)


def read_utf8_file(location):
    with codecs.open(location, 'r', encoding='utf-8') as f:
        return f.read()


def read_utf8_file_or_url(location):
    if location.startswith('https://'):
        return read_utf8_url(location.replace(os.sep, '/'))
    return read_utf8_file(location)


def copy_file_or_url(origin, target):
    if origin.startswith('https://'):
        origin = origin.replace(os.sep, '/')
        target = target.replace('/', os.sep)

        topdir = os.path.dirname(target)
        if not os.path.exists(topdir):
            os.makedirs(topdir)

        @retry(exceptions=subprocess.CalledProcessError, tries=5, delay=0.2)
        def makerequest():
            resp = SESSION.get(origin, stream=True, headers=HEADERS)
            if resp:
                with open(target, 'wb') as f:
                    for chunk in resp.iter_content(65536):
                        f.write(chunk)
                return True
            say(f'{resp.status_code} {resp.reason}: GET {origin}', level=logging.WARN)
            return False
        return makerequest()

    if os.path.exists(origin):
        # say(f'copying {origin} -> {target}')
        topdir = os.path.dirname(target)
        if not os.path.exists(topdir):
            os.makedirs(topdir)
        shutil.copy(origin, target)
        return True
    return False


def emit_launchdarkly_flag(key, variation):
    flagsdir = os.path.join(ROOT, 'output', 'launchdarkly')
    if not os.path.exists(flagsdir):
        os.makedirs(flagsdir)
    with open(os.path.join(flagsdir, key), 'w') as f:
        json.dump(variation, f, indent=2, sort_keys=True)


def emit_bazel_build_files(build):
    """Convert specs into bazel rules.

    Generate rules for tagged componentsuites. SDDS2 clients use tags to find
    the latest suites in the warehouse. SDDS3 clients ask the
    SophosUpdateService (SUS), which looks up the suites in LaunchDarkly.

    SUS means we don't need to build non-tagged components: they've already
    been built, and they'll be served by SUS as appropriate.

    Approach (repeated for each directory in specs/{winep,winsrv}/*):

    * load the pubspec.xml and importspec.xml
    * search for each tagged componentsuite
    * generate a bazel rule for each referenced component (including the suite itself)
        * if the component has an SDDS3 package already, emit copy_prebuilt_sdds_package
        * if not, emit build_sdds3_package
        * in both cases, we'll need to import the fileset
    * generate a metadata.yaml file for each suite
        * used to populate the supplement-refs for every package-ref in the suite
    * generate a bazel rule for each suite
        * requires the target name, the metadata, the sdds-imports, and the packages

    """

    # Link support. links{} contains links which are resolved after the main loop.
    links = {}
    # Link support. cache{} contains pubspec results, allowing links to be resolved.
    cache = {}

    for path, _, files in os.walk(os.path.join(ROOT, 'specs')):
        for f in files:
            lower = f.lower()

            # Link to another pubspec. Remember the link, and resolve it later by
            # emitting another copy of the LaunchDarkly flag which refers to the
            # same version and suite.
            if lower == 'link':
                target = read_utf8_file(os.path.join(path, f)).strip()
                target = target.replace('/', os.path.sep)
                links[os.path.join(path, f)] = os.path.join(ROOT, target)
                continue

            if lower != 'pubspec.xml':
                continue

            product = os.path.basename(os.path.dirname(path))

            pubspec = load_xml_from(path, f)
            impfile = pubspec.attrib['importspec']
            importspec = load_xml_from(path, impfile)

            say(f'===== Generating bazel rules for product={product} pubspec={path}/{f}')
            info = bazelize_pubspec(build, product, os.path.basename(path), pubspec, importspec)
            cache[os.path.join(path, f)] = info
            say(f'===== Generating SDDS2 specs for product={product} pubspec={path}/{f}')
            emit_sdds2_specs(pubspec, importspec, path)
            say('')

    for link in links:
        target = links[link]
        if target not in cache:
            raise NameError(f'Error: link {link} to {target}: link target not found')

        product = os.path.basename(os.path.dirname(os.path.dirname(link)))
        for info in cache[target]:
            emit_launchdarkly_flag(f'release.{product}.{info["line_id"]}.json', info['flag_variation'])


def emit_sdds2_specs(pubspec, importspec, path):
    specsdir = os.path.join(ROOT, 'output', os.path.relpath(path, ROOT))
    if not os.path.exists(specsdir):
        os.makedirs(specsdir)

    sdds2_xform_remove_sdds3_only_components(pubspec)
    sdds2_xform_flags_supplements(pubspec, os.path.basename(path))
    sdds2_xform_supplements_relativeto(pubspec)
    sdds2_xform_winep_suites(importspec)

    with codecs.open(os.path.join(specsdir, 'pubspec.xml'), 'w', encoding='utf-8') as f:
        f.write(rewrite_xml(pubspec))
    with codecs.open(os.path.join(specsdir, pubspec.attrib['importspec']), 'w', encoding='utf-8') as f:
        f.write(rewrite_xml(importspec))


def sdds2_xform_winep_suites(importspec):
    for line in importspec.findall('imports/line[componentsuite]'):
        line_id = line.attrib['id']

        for suite in line.findall('componentsuite[@source="auto"]'):
            # Add an artifact corresponding to this build's eventually-promoted output
            buildid = os.environ['BUILD_INSTANCE'] if 'BUILD_INSTANCE' in os.environ else 'noid'
            branch = os.environ['SOURCE_CODE_BRANCH'] if 'SOURCE_CODE_BRANCH' in os.environ else 'nobranch'
            branch = branch.replace('/', '--')

            # Example path:
            # esg-releasable-candidate/winep.warehouse/$BRANCH/$BUILD_ID/winep_suites/SDDS-Ready-Packages/$LINE_ID/data/
            artifact = 'esg-releasable-candidate/winep.warehouse/' \
                + f'{branch}/{buildid}/winep_suites/SDDS-Ready-Packages/{line_id}/data'
            suite.attrib['artifact'] = artifact
            del suite.attrib['source']


def sdds2_xform_remove_sdds3_only_components(pubspec):
    for suite in pubspec.findall('componentsuites/line/componentsuite'):
        for comp in suite.findall('*'):
            if not include_in_mode('sdds2', comp):
                suite.remove(comp)
            if 'mode' in comp.attrib:
                del comp.attrib['mode']
            # Delete any child elements and any text
            comp.text = ''
            for extra in comp.findall('*'):
                comp.remove(extra)
    for wwh in pubspec.findall('warehouses/warehouse'):
        for line in wwh.findall('line'):
            if not include_in_mode('sdds2', line):
                wwh.remove(line)
            if 'mode' in line.attrib:
                del line.attrib['mode']


def include_in_mode(mode, element):
    if 'mode' not in element.attrib:
        return True
    return mode in element.attrib['mode'].split(',')


def sdds2_xform_flags_supplements(pubspec, flagsname):
    """Transforms flags= attributes into equivalent SDDS2 <genericsupplement>s"""
    for line in pubspec.findall('warehouses/warehouse/line[componentsuite]'):
        for suite in line.findall('componentsuite[@flags]'):
            # What is checked in:
            #
            #    <componentsuite flags="3-8-1-EAP" ...>
            #
            # How it is emitted:
            #
            #    <componentsuite ...>
            #      <genericsupplement decodepath="." line="CIXFLAGS" tag="3-8-1-EAP" warehousename="cix_flags"/>

            # Copy the 'text' and 'tail' from the first child element, and insert after it.
            # If the componentsuite *has* no children, create some default values that look reasonable.
            sup = suite.makeelement('genericsupplement', {
                'decodepath': '.',
                'warehousename': f'{flagsname.lower()}_flags',
                'line': f'{flagsname.upper()}FLAGS',
                'tag': suite.attrib['flags'],
            })
            del suite.attrib['flags']

            if line.text is not None:
                sup.tail = line.text
                if len(suite) > 0:
                    sup.tail += '  '
                if suite.text is None:
                    suite.text = line.text + '  '
            suite.insert(0, sup)    # insert at the front


def sdds2_xform_supplements_relativeto(pubspec):
    """Transforms <genericsupplement> elements with relativeto into equivalent SDDS2 <genericsupplement>s"""
    for instance in pubspec.findall('warehouses/warehouse/line/*'):
        for sup in instance.findall('genericsupplement[@relativeto]'):
            # What is checked in:
            #
            # <genericsupplement
            #             relativeto="12D32E6C-4CFF-435B-BC98-5E14CCB54950"
            #             decodepath="telemetry"
            #             line="995645D3-DA2A-407E-AC5D-7267B8B43975"
            #             tag="USER_TELEM"
            #             warehousename="ml_models" />
            #
            # How it is emitted:
            #
            # <genericsupplement
            #             decodepath="<decode-path-of-target-component>/<original-decode-path>"
            #             line="995645D3-DA2A-407E-AC5D-7267B8B43975"
            #             tag="USER_TELEM"
            #             warehousename="ml_models" />
            tgt_id = sup.attrib['relativeto']
            comp = pubspec.find(f'componentsuites/line/componentsuite/component[@line="{tgt_id}"]')
            if comp is None:
                raise FileNotFoundError('Error processing genericsupplement[@relativeto] element: '
                                        + f'target-component {tgt_id} not found')
            sup.attrib['decodepath'] = os.path.join(comp.attrib['mountpoint'], sup.attrib['decodepath'])
            del sup.attrib['relativeto']


def bazelize_pubspec(build, product, name, pubspec, importspec):
    info = []

    # Find componentsuite lines
    for line in pubspec.findall('warehouses/warehouse/line[componentsuite]'):
        ld_flag_variation = {}

        # Find componentsuite instances with tags
        for suite in line.findall('componentsuite[releasetag]'):
            tags = [t.attrib['tag'] for t in suite.findall('releasetag')]

            meta, suite_name = bazelize_suite(build, name, line.attrib['id'], suite, pubspec, importspec)

            for tag in tags:
                ld_flag_variation[tag] = {
                    'suite': f'sdds3.{suite_name}.dat',
                    'version': meta['package_version'],
                }

        suitekey = f'release.{product}.{line.attrib["id"]}.json'
        emit_launchdarkly_flag(suitekey, ld_flag_variation)
        info.append({
            'line_id': line.attrib['id'],
            'flag_variation': ld_flag_variation,
        })

    return info


def bazelize_suite(build, name, line_id, instance, pubspec, importspec):
    metadata = {
        'subcomponents': {},
    }
    sdds_imports = []
    packages = []

    # Look up components that belong to this instance of the componentsuite
    # For each component, spit out either build_sdds3_package or copy_prebuilt_sdds3_package.
    # We'll download the package source to the 'inputs/prod' folder

    suitepkg = bazelize_package(0, build, metadata, line_id, instance.attrib['importreference'], importspec)
    sdds_imports.append(f"{suitepkg['bzlinput']}:sdds_import")
    packages.append(suitepkg['rulename'])

    bazelize_componentsuite_packages(build, line_id, instance.attrib['importreference'],
                                     pubspec, importspec, metadata, sdds_imports, packages)

    bazelize_componentsuite_supplements(name, line_id, instance.attrib['importreference'], pubspec, metadata)

    metafh = io.StringIO()
    yaml.dump(metadata, metafh)
    suite_nonce = nonce_from_sha256(hash_string(metafh.getvalue()))
    suite_name = f'{suitepkg["name"]}_{suitepkg["version"]}.{suite_nonce}'

    with open(f'{BASE}/.output/suite_{suite_name}_metadata.yaml', 'wb') as f:
        f.write(metafh.getvalue().encode('utf-8'))

    print(f"""
filegroup(
    name = "suite_{suite_name}_metadata",
    srcs = [".output/suite_{suite_name}_metadata.yaml"],
)
build_sdds3_suite(
    name = "suite_{suite_name}",
    suite = "suite/sdds3.{suite_name}.dat",
    metadata = "suite_{suite_name}_metadata",
    sdds_imports = {sdds_imports},
    packages = {packages},
)
""", file=build)

    return metadata, suite_name


def bazelize_componentsuite_packages(build, line_id, importref, pubspec, importspec, metadata, sdds_imports, packages):
    suite = pubspec.find(f'componentsuites/line[@id="{line_id}"]/'
                         + f'componentsuite[@importreference="{importref}"]')

    # If the suite doesn't appear in /componentsuites, it has no packages (e.g. WindowsCloudClean)
    if suite is None:
        return
    for component in suite.findall('*'):
        if not include_in_mode('sdds3', component):
            continue
        pkgmeta = {}
        pkg = bazelize_package(1, build, pkgmeta, component.attrib['line'],
                               component.attrib['importreference'], importspec)
        sdds_imports.append(f'{pkg["bzlinput"]}:sdds_import')
        packages.append(f'{pkg["rulename"]}')

        metadata['subcomponents'][pkg['id']] = pkgmeta


def bazelize_componentsuite_supplements(name, line_id, importref, pubspec, metadata):
    """Extract supplements from the pubspec and insert them into the metadata    """

    # Find the root of the suite's structure element (where we can find the suite's child references)
    suite = pubspec.find(f'componentsuites/line[@id="{line_id}"]/'
                         + f'componentsuite[@importreference="{importref}"]')

    # First check the suite's supplements:
    add_supplements(name, 0, line_id, importref, pubspec, metadata, suite)

    # If the suite appears in /componentsuites, it has subcomponents to consider:
    if suite is not None:
        for component in suite.findall('*'):
            if not include_in_mode('sdds3', component):
                continue
            pkgid = f'{component.attrib["line"]}#{component.attrib["importreference"]}'
            pkgmeta = metadata['subcomponents'][pkgid]
            add_supplements(name, 1, component.attrib['line'], component.attrib['importreference'], pubspec, pkgmeta)
            add_symbolic_paths(1, component, pkgmeta)


def load_hints(line, importref):
    hints = load_yaml_from(os.path.join(ROOT, 'specs'), 'hints.yaml')

    key = f'{line}#{importref}'
    if key in hints:
        return hints[key]
    if line in hints:
        return hints[line]
    return None


def add_symbolic_paths(level, component, pkgmeta):
    hints = load_hints(component.attrib['line'], component.attrib['importreference'])

    if not hints:
        return
    if 'define-symbolic-paths' not in hints:
        return
    for sym in hints['define-symbolic-paths']:
        if 'define-symbolic-paths' not in pkgmeta:
            pkgmeta['define-symbolic-paths'] = []
        say(f'{indent(level)} {component.attrib["line"]}#{component.attrib["importreference"]}:'
            + f'define symbolic path {sym}')
        pkgmeta['define-symbolic-paths'].append(sym)


def add_supplements(name, level, line_id, importref, pubspec, pkgmeta, suite=None):
    comp = pubspec.find(f'warehouses/warehouse/line[@id="{line_id}"]/*[@importreference="{importref}"]')
    if comp is None:
        return

    # First, check for a 'flags' attribute on the component itself
    if 'flags' in comp.attrib and suite is not None:
        supplement = {
            'line-id': f'{name.upper()}FLAGS',
            'tag': comp.attrib['flags'],
            'decode-path': '@flags',
        }
        add_supplement(level, f'{line_id}#{importref}', pkgmeta, supplement)

    # Now find each genericsupplement
    for sup in comp.findall('genericsupplement'):
        pkgid = f'{line_id}#{importref}'
        meta = pkgmeta
        lvl = level
        supplement = {
            'line-id': sup.attrib['line'],
            'tag': sup.attrib['tag'],
            'decode-path': sup.attrib['decodepath'],
        }
        if 'relativeto' in sup.attrib:
            pkgid, meta = find_subcomponent(pkgid, sup.attrib['relativeto'], suite, pkgmeta)
            lvl += 1
        if 'baseversion' in sup.attrib:
            supplement['base-version'] = sup.attrib['baseversion']
        add_supplement(lvl, pkgid, meta, supplement)


def find_subcomponent(pkgid, target_component, suite, pkgmeta):
    if suite is None:
        raise NotADirectoryError(f'<genericsupplement relativeto= appeared on {pkgid}, which has no subcomponents')

    pkg = suite.find(f'*[@line="{target_component}"]')
    if pkg is None:
        raise FileNotFoundError('<genericsupplement>: '
                                + f'suite {pkgid} does not contain component {target_component}')
    newpkgid = f'{target_component}#{pkg.attrib["importreference"]}'
    if newpkgid not in pkgmeta['subcomponents']:
        raise FileNotFoundError('<genericsupplement>: '
                                + f'suite {pkgid} does not contain component {newpkgid}')
    return newpkgid, pkgmeta['subcomponents'][newpkgid]


def add_supplement(level, pkgid, pkgmeta, supplement):
    say(f'{indent(level)} supplement attached to {pkgid}: {supplement}')
    if 'supplements' not in pkgmeta:
        pkgmeta['supplements'] = []
    pkgmeta['supplements'].append(supplement)


def bazelize_package(level, build, metadata, line_id, importref, importspec):
    # Look up the component in the importspec.

    comp, info = import_package(level, importspec, line_id, importref)
    info['id'] = f'{line_id}#{importref}'

    if not info['emitted']:
        info['emitted'] = True
        if info['prebuilt']:
            print(f"""
copy_prebuilt_sdds3_package(
    name = "{info['rulename']}",
    package = "package/{info['package_name']}",
    prebuilt_package = "{info['bzlinput']}:prebuilt_package",
)
""", file=build)
        else:
            print(f"""
build_sdds3_package(
    name = "{info['rulename']}",
    package = "package/{info['package_name']}",
    sdds_import = "{info['bzlinput']}:sdds_import",
)
""", file=build)

    metadata['id'] = info['id']
    metadata['fileset'] = info['fileset']
    metadata['package_name'] = info['name']
    metadata['package_version'] = info['version']
    metadata['package_nonce'] = info['nonce']
    metadata['package_line_id'] = info['line_id']

    if info['suite']:
        if 'marketingversion' in comp.attrib:
            metadata['marketing_version'] = comp.attrib['marketingversion']
        elif 'longdic' in comp.attrib:
            metadata['marketing_version'] = comp.attrib['longdic']
        elif 'shortdic' in comp.attrib:
            metadata['marketing_version'] = comp.attrib['shortdic']
        else:
            metadata['marketing_version'] = metadata['package_version']

    return info


def import_package(level, importspec, line_id, importref):
    xpath = f'imports/line[@id="{line_id}"]/*[@id="{importref}"]'
    comp = importspec.find(xpath)

    if comp is None:
        raise FileNotFoundError(f'{line_id}#{importref}: did not find xpath {xpath}')

    if 'fileset' in comp.attrib:
        info = import_fileset(level, line_id, f'fileset {comp.attrib["fileset"]}', comp.attrib['fileset'], comp)
    elif 'artifact' in comp.attrib:
        info = import_artifact(level, line_id, importref, comp)
    elif 'source' in comp.attrib:
        fileset = os.path.join(ROOT, 'inputs', 'winep_suites', line_id)
        if os.path.exists(os.path.join(fileset, 'sdds3')):
            fileset = os.path.join(fileset, 'sdds3')
        else:
            fileset = os.path.join(fileset, 'data')
        info = import_fileset(level, line_id, f'fileset {fileset}', fileset, comp)
    else:
        raise FileNotFoundError(
            f'Could not find "fileset", "artifact", or "source" in importspec {line_id}#{importref}')

    return comp, info


# A map of (line_id, fileset, importspec-element) to objects like this one:
#
# {
#   # The bazel rulename for the package build rule
#   'rulename': "SDU_6.7.324.324.4f0d1a8018",
#   # The bazel package where the package's fileset lives
#   'bzlinput': "//inputs/prod/1FE3E7DF-EFFA-408A-A1B0-89F15BA61F31/4f0d1a8018",
#   # The parsed SDDS-Import.xml
#   'import': <parsed sdds-import.xml>,
# }
#
SDDS_IMPORT_CACHE = {}


def rewrite_xml(sdds_import_root):
    tmpio = io.BytesIO()
    ET.ElementTree(element=sdds_import_root).write(tmpio, encoding='UTF-8', xml_declaration=True)
    return tmpio.getvalue().decode('utf-8')


def override_version(level, name, sdds_import_root, comp):
    version = comp.attrib['version']
    if version.endswith('.'):
        version = autoversion(name, version[:-1])
    sdds_import_root.find('Component/Version').text = version
    say(f'{indent(level + 1)} overriding version with {version}')


def override_shortdic(level, sdds_import_root, comp):
    shortdic = comp.attrib['shortdic']
    sdds_import_root.find('Dictionary/Name/Label/Language[@lang="en"]/ShortDesc').text = shortdic
    say(f'{indent(level + 1)} overriding shortdic with {shortdic}')


def override_longdic(level, sdds_import_root, comp):
    longdic = comp.attrib['longdic']
    sdds_import_root.find('Dictionary/Name/Label/Language[@lang="en"]/LongDesc').text = longdic
    say(f'{indent(level + 1)} overriding longdic with {longdic}')


def override_name_with_line_id(level, sdds_import_root, line_id):
    sdds_import_root.find('Component/Name').text = line_id
    sdds_import_root.find('Component/RigidName').text = line_id
    say(f'{indent(level + 1)} overriding package name/line-id with {line_id}')


def override_marketing_version(level, sdds_import_root, comp):
    for f in sdds_import_root.findall('Component/FileList/File'):
        if f.attrib['Name'] == 'version' and 'Offset' not in f.attrib:
            say(f'{indent(level + 1)} overriding marketingversion with {comp.attrib["marketingversion"]}')
            # replace the Size and MD5 elements with those of the marketingversion
            f.attrib['Size'] = str(len(comp.attrib['marketingversion']))
            md5 = hashlib.md5()
            md5.update(comp.attrib['marketingversion'].encode('utf-8'))
            f.attrib['MD5'] = md5.hexdigest()
            break


# pylint: disable=R0914     # too many local variables. Honestly.
def import_fileset(level, line_id, description, fileset, comp):
    cache_key = calculate_import_cache_key(line_id, fileset, comp.attrib)
    if cache_key in SDDS_IMPORT_CACHE:
        return summarize_import(level, SDDS_IMPORT_CACHE[cache_key])

    say(f'{indent(level)} {line_id}: importing {description}')

    # 1. Pull down the SDDS-Import.xml and parse it
    # 2. If this importspec is versioned, override the version
    # 3. If this is a componentsuite, and the fileset includes a 'version' file,
    #    then we need to rewrite it with the contents of the 'marketingversion' attribute
    # 4. If this is a componentsuite, we need to ensure the package name is the line_id.
    # 5. Then, finally: calculate the package version & nonce

    sdds_import_root = ET.fromstring(read_utf8_file_or_url(os.path.join(fileset, 'SDDS-Import.xml')))

    replace_version_file = False
    if comp.tag == 'componentsuite':
        override_name_with_line_id(level, sdds_import_root, line_id)
        if 'marketingversion' in comp.attrib:
            replace_version_file = True
            override_marketing_version(level, sdds_import_root, comp)

    name = sdds_import_root.findtext('Component/Name').replace(' ', '')

    if 'version' in comp.attrib:
        override_version(level, name, sdds_import_root, comp)
    if 'shortdic' in comp.attrib:
        override_shortdic(level, sdds_import_root, comp)
    if 'longdic' in comp.attrib:
        override_longdic(level, sdds_import_root, comp)

    nonce = nonce_from_sha256(hash_string(rewrite_xml(sdds_import_root)))
    version = sdds_import_root.findtext('Component/Version')
    base_name = f'{name}_{version}.{nonce}'
    zip_name = f'{base_name}.zip'

    info = {
        'suite': comp.tag == 'componentsuite',
        'package_name': zip_name,
        'emitted': False,
        'rulename': base_name,
        'bzlinput': f'//inputs/prod/{line_id}/{base_name}',
        'import': sdds_import_root,
        'prebuilt': False,
        'fileset': f'../inputs/prod/{line_id}/{base_name}',
        'line_id': line_id,
        'name': name,
        'version': version,
        'nonce': nonce,
    }

    destdir = os.path.join(ROOT, 'inputs', 'prod', line_id, base_name)
    if not os.path.exists(destdir):
        os.makedirs(destdir)

    # Write the SDDS-Import.xml file itself
    with codecs.open(os.path.join(destdir, 'SDDS-Import.xml'), 'w', encoding='utf-8') as f:
        f.write(rewrite_xml(sdds_import_root))

    copy_fileset_to_destdir(level + 1, fileset, destdir, zip_name, replace_version_file, comp, sdds_import_root, info)

    with open(os.path.join(destdir, 'BUILD'), 'w') as f:
        print(f"""####################################################################################
# AUTO-GENERATED BUILD FILE. DO NOT EDIT.
filegroup(
    name = "sdds_import",
    srcs = ["SDDS-Import.xml"],
    visibility = ["//visibility:public"],
)
filegroup(
    name = "prebuilt_package",
    srcs = ["{zip_name}"],
    visibility = ["//visibility:public"],
)
""", file=f)

    SDDS_IMPORT_CACHE[cache_key] = info
    say(f'{indent(level + 1)} using fileset {info["fileset"]}')
    return info


def import_artifact(level, line_id, importref, comp):
    rel_artifact = comp.attrib['artifact']
    if not rel_artifact.startswith('esg-releasable-candidate'):
        raise NameError(f'artifactory_url: refusing to accept non-promoted <release-asset>: {rel_artifact}')

    cache_key = calculate_import_cache_key(line_id, None, comp.attrib)
    if cache_key in SDDS_IMPORT_CACHE:
        return summarize_import(level, SDDS_IMPORT_CACHE[cache_key])

    if rel_artifact.endswith('/'):
        rel_artifact = rel_artifact[:-1]  # chop trailing slash

    # Download and extract the zipfile into a temporary directory, then call import_fileset on that
    with tempfile.TemporaryDirectory() as tempdir:

        # Convert the URL into an esg-build-candidate (because we don't have access to the former).
        # releasable-candidate artifacts look like this:
        #    esg-releasable-candidate/<repo>/<branch>/<build>/<path-to-fileset>
        # build-candidate artifactrs look like this:
        #    esg-build-candidate/<repo>/<branch>/<build>/build/<path-to-fileset>.zip

        parts = rel_artifact.split('/', 4)
        parts[0] = 'esg-build-candidate'
        parts.insert(-1, 'build')
        parts = '/'.join(parts).split('/')

        suffix = []

        # Support transforming the artifact path
        xformer = get_artifact_transformer(line_id, importref)

        # Part of the annoyance here is that we don't know "where" the artifact is.
        # Try the deepest path first, then retry until we run out of paths.
        while len(parts) > 0:
            bld_artifacts = []
            if xformer:
                bld_artifacts.append(xformer('/'.join(parts)) + '.zip')
            bld_artifacts.append('/'.join(parts) + '.zip')

            for bld_artifact in bld_artifacts:
                if copy_file_or_url(f'{ARTIFACTORY_URL}/{bld_artifact}', f'{tempdir}/pkg.zip'):
                    with zipfile.ZipFile(f'{tempdir}/pkg.zip', 'r') as zpkg:
                        zpkg.extractall(tempdir)
                        info = import_fileset(level, line_id, f'artifact {bld_artifact}',
                                              os.path.join(tempdir, *suffix), comp)
                        SDDS_IMPORT_CACHE[cache_key] = info
                        return info

            suffix.insert(0, parts.pop())


def get_artifact_transformer(line_id, importref):
    hints = load_hints(line_id, importref)
    if not hints or 'artifact-transform' not in hints:
        return None
    # pylint: disable=W0123     # eval isn't evil, honestly
    return eval(hints['artifact-transform'])


def summarize_import(level, info):
    say(f'{indent(level)} {info["line_id"]}: using fileset {info["fileset"]} [CACHED]')
    return info


def calculate_import_cache_key(line_id, fileset, attrib):
    # return a tuple of line_id, fileset, and a sorted tuple of dict key/value pairs
    return (line_id, fileset, tuple((k, attrib[k]) for k in sorted(attrib)))


def copy_fileset_to_destdir(level, fileset, destdir, zip_name, replace_version_file, comp, sdds_import_root, info):
    if comp.tag == 'component' and copy_file_or_url(os.path.join(fileset, zip_name), os.path.join(destdir, zip_name)):
        info['prebuilt'] = True
    else:
        start = timer()
        overall = start
        files = 0
        for f in sdds_import_root.findall('Component/FileList/File'):
            if 'Offset' in f.attrib:
                source = os.path.join(fileset, f.attrib['Offset'], f.attrib['Name'])
                target = os.path.join(destdir, f.attrib['Offset'], f.attrib['Name'])
            elif f.attrib['Name'] == 'version' and replace_version_file:
                with open(os.path.join(destdir, 'version'), 'w') as f:
                    f.write(comp.attrib['marketingversion'])
                    continue
            else:
                source = os.path.join(fileset, f.attrib['Name'])
                target = os.path.join(destdir, f.attrib['Name'])
            filestart = timer()
            copy_file_or_url(source, target)
            files += 1
            end = timer()
            if (end - filestart) >= 5:
                say(f'{indent(level)} copied {source} -> {target} in {end - filestart:.3f}s...')
            if (end - start) >= 10:
                say(f'{indent(level)} copied {files} files in {end - overall:.3f}s...')
                start = end     # reset the clock


AUTOVERSION_CACHE = {}


def autoversion(comp, baseversion):
    cache_key = f'{comp}#{baseversion}'

    if cache_key not in AUTOVERSION_CACHE:
        result = subprocess.run(
            ['versioning_client', '-c', f'sdds3_{comp}', '-v', baseversion],
            check=True,
            capture_output=True,
            universal_newlines=True,
            encoding='utf-8',
        )
        version = result.stdout.strip()

        AUTOVERSION_CACHE[cache_key] = {'version': version, 'seq': 0}

    info = AUTOVERSION_CACHE[cache_key]
    info['seq'] += 1

    return f"{info['version']}.{info['seq']}"


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
    set_artifactory_auth_headers()
    cleanup_output_folders()

    say(f'Using ARTIFACTORY_URL: {ARTIFACTORY_URL}')
    say('')

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

        emit_bazel_build_files(build)


def indent(level):
    text = ''
    for _ in range(level):
        text += '  '
    text += f' '
    return text


if __name__ == '__main__':
    main()
