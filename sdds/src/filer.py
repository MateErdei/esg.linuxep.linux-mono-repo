#!/usr/bin/env python

from __future__ import print_function
import os
import re
import sys

UPSTREAM_PROD = r'\\uk-filer5.prod.sophos\PRODRO\bir'
UPSTREAM_DEV = r'\\uk-filer6.eng.sophos\bfr'

UNIFIED_PIPELINE_INPUTS = ['sau_xg', 'esg']
# Those are the inputs where the last good component build file is created too
# early, for various reasons. For those, look into last good build file.
UNRELIABLE_DEV_INPUTS = ['sed', 'ssp-release']


def decompose_path(path):
    elements = []
    head = tail = os.path.normpath(path)
    while tail:
        head, tail = os.path.split(head)
        elements.append(tail if tail else head)

    elements.reverse()
    return elements


def replace_placeholder(host, elements, index, replacement_file):
    root = os.path.join(host, *elements[:index])
    with open(os.path.join(root, replacement_file), 'r') as f:
        elements[index] = f.readline().strip()

    return elements


def read_file(path):
    try:
        with open(path) as f:
            return f.readline().strip()
    except IOError:
        return None


def get_last_good_component_build(path, name):
    build = ''
    lgb_path = os.path.join(path, 'lastgoodbuild.txt')
    if os.path.exists(lgb_path):
        build = read_file(lgb_path)
    # regex for unified pipeline build name format
    if(re.search("[0-9]{14}-[0-9a-zA-Z]{40}-[0-9a-zA-Z]{6}", build) or name in UNRELIABLE_DEV_INPUTS):
        if os.path.isdir(os.path.join(path, build, name)):
            return build
        else:
            return None

    component_lgb_path = os.path.join(path, name + '_lastgoodbuild.txt')
    return read_file(component_lgb_path)


def get_latest_version(path):
    versions = os.listdir(path)
    if len(versions) == 0:
        return None
    if len(versions) ==1:
        return versions[0]

    current = versions[0]

    for v in versions:
        if v > current:
            current = v

    return current

def locate_on_filer5(path):
    package = os.path.join(UPSTREAM_PROD, path)
    print("Inspecting", package)
    if os.path.exists(package):
        return UPSTREAM_PROD, path
    return None, None


def locate_on_filer6(path):
    elements = decompose_path(path)

    if elements.count('%LASTGOODBUILD%'):
        i = elements.index('%LASTGOODBUILD%')
        elements = replace_placeholder(UPSTREAM_DEV, elements, i, 'lastgoodbuild.txt')
    elif elements.count('%LASTGOODCOMPONENTBUILD%'):
        i = elements.index('%LASTGOODCOMPONENTBUILD%')
        elements = replace_placeholder(UPSTREAM_DEV, elements, i, elements[i + 1] + '_lastgoodbuild.txt')

    package = os.path.join(*elements)
    if os.path.exists(os.path.join(UPSTREAM_DEV, package)):
        return UPSTREAM_DEV, package
    return None, None


def locate_package(path):
    filer, package = locate_on_filer5(path)
    if package is None:
        filer, package = locate_on_filer6(path)
    if package is None:
        raise OSError(2, 'Package not found', path)
    return filer, package


def locate_scaffold_package_on_filer6(branch, build, name, version):
    not_found = None, None, None
    path = branch
    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        return not_found
    if not build:
        build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name)
        if not build:
            return not_found
    path = os.path.join(path, build, name)
    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        return not_found
    if not version:
        version = get_latest_version(os.path.join(UPSTREAM_DEV, path))
        if not version:
            return not_found
    path = os.path.join(path, version)

    return UPSTREAM_DEV, path, build


def locate_artisan_package_on_filer6(name, branch, build, build_type, version, sub_component):
    not_found = None, None, None, None
    path = name
    component = name if sub_component is None else sub_component
    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        return not_found
    if not branch:
        branch = 'develop'
    path = os.path.join(path, branch)
    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        return not_found
    if not build:
        if build_type:
            build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), component + '-' + build_type)
        else:
            build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), component + '-release')
            if build:
                build_type = 'release'
            else:
                build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), component)
        if not build:
            return not_found

    if build_type:
        path = os.path.join(path, build, component + '-' + build_type)
    else:
        path = os.path.join(path, build, component)

    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        print('expected path does not exist: ' + os.path.join(UPSTREAM_DEV, path))
        return not_found

    if not version:
        version = get_latest_version(os.path.join(UPSTREAM_DEV, path))
        if not version:
            return not_found
    path = os.path.join(path, version)

    return UPSTREAM_DEV, path, branch, build


def latest_directory_in_path(path):
    entries = os.listdir(path)
    if not entries:
        raise RuntimeError(f'Unexpected empty folder: {path}')
    folders = [f for f in entries if os.path.isdir(os.path.join(path, f))]
    if not folders:
        raise RuntimeError(f'Path contains no folders: {path}')
    return sorted(folders, key=lambda f: os.path.getmtime(os.path.join(path, f)))[-1]


def locate_package_from_clues(name, version, build_id, output):
    """
    Find a build on the production or dev filer from build metadata

    name - the component name (mandatory)
    branch - the git branch, or build controller name (default: develop)
    version - the version to use (dash separated, default: latest)
    build - the build identifier of the build to use (default: last good component build)
    build_type - release or debug (default: release)
    sub_component - component name. incase there are multiple components in a repository
    """

    print(f'Looking for name={name} version={version} build_id={build_id} output={output}')

    assert name, f'Expected a package name, got "{name}"'

    base_path = os.path.join(UPSTREAM_PROD, name)
    if not os.path.exists(base_path):
        raise RuntimeError(f'Path not found: {base_path}')

    if not version:
        version = latest_directory_in_path(base_path)
    else:
        version = version.replace('.', '-')
    base_path = os.path.join(base_path, version)

    if not build_id:
        build_id = latest_directory_in_path(base_path)

    full_path = os.path.join(base_path, build_id)
    if output:
        full_path = os.path.join(full_path, output)

    if not os.path.exists(full_path):
        raise RuntimeError(f'Path not found: {full_path}')

    return full_path


def main(args):
    if not args:
        print('Usage filer.py package ...')
        sys.exit(1)

    for arg in args:
        filer, path = locate_package(arg)
        print(os.path.join(filer, path))


if __name__ == '__main__':
    main(sys.argv[1:])