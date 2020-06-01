#!/usr/bin/env python

from __future__ import print_function
import os
import sys

UPSTREAM_PROD = r'\\uk-filer5.prod.sophos\PRODRO\bir'
UPSTREAM_DEV = r'\\uk-filer6.eng.sophos\bfr'


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


def get_last_good_component_build(path, name):
    if name == 'sed':
        # TODO remove this when SED stamps builds correctly
        good_build_file_name = os.path.join(path, 'lastgoodbuild.txt')
    else:
        good_build_file_name = os.path.join(path, name + '_lastgoodbuild.txt')

    try:
        with open(good_build_file_name) as f:
            build = f.readline().strip()
    except IOError:
        return None

    return build


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
        build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name + "_linux11")
        if not build:
            build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name)
            if not build:
                return not_found

    path = os.path.join(path, build, name + "_linux11")

    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        path = os.path.join(path, build, name)
        if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
            return not_found
    if not version:
        version = get_latest_version(os.path.join(UPSTREAM_DEV, path))
        if not version:
            return not_found
    path = os.path.join(path, version)

    return UPSTREAM_DEV, path, build


def locate_artisan_package_on_filer6(name, branch, build, build_type, version):
    not_found = None, None, None, None
    path = name
    print("Path name = {}".format(os.path.join(UPSTREAM_DEV, path)))
    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        return not_found
    if not branch:
        branch = 'develop'
    path = os.path.join(path, branch)
    print("Path name and branch = {}".format(os.path.join(UPSTREAM_DEV, path)))
    if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
        return not_found
    if not build:
        if build_type:
            build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name + '_linux11-' + build_type)
        else:
            build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name + '_linux11-release')
            if build:
                build_type = 'release'
            else:
                build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name + "_linux11")

        if not build:
            if build_type:
                build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name + build_type)
                else:
                build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name + '-release')
                if build:
                    build_type = 'release'
                else:
                    build = get_last_good_component_build(os.path.join(UPSTREAM_DEV, path), name)
            if not build:
                return not_found

    print("Build = {}".format(build))

    if build_type:
        path = os.path.join(path, build, name + '-' + build_type)
    else:
        path = os.path.join(path, build, name)

    if build_type:
        path_linux = os.path.join(path, build, name + '-' + build_type)
    else:
        path_linux = os.path.join(path, build, name)

    if not os.path.exists(os.path.join(UPSTREAM_DEV, path_linux)):
        if not os.path.exists(os.path.join(UPSTREAM_DEV, path)):
            print("Path build name = {}".format(os.path.join(UPSTREAM_DEV, path)))
            return not_found
    else:
        path = path_linux
        print("Path build name = {}".format(os.path.join(UPSTREAM_DEV, path)))

    if not version:
        version = get_latest_version(os.path.join(UPSTREAM_DEV, path))
        if not version:
            return not_found
    path = os.path.join(path, version)

    print("path version = {}".format(os.path.join(UPSTREAM_DEV, path)))

    return UPSTREAM_DEV, path, branch, build

def locate_package_from_clues(name, branch, version, build, build_type):
    """
    Find a build on the production or dev filer from build metadata

    name - the component name (mandatory)
    branch - the git branch, or build controller name (default: develop)
    version - the version to use (dash separated, default: latest)
    build - the build identifier of the build to use (default: last good component build)
    build_type - release or debug (default: release)
    """

    print ('Looking for {}:{}:{}:{}:{}'.format(name, branch, version, build, build_type))

    try:
        # production builds are fully specified <name>/<version>/<build>
        if name and not branch and version and build and not build_type:
            filer, package = locate_on_filer5(os.path.join(name, version.replace(".", "-"), build))
            if package:
                print ('Found on filer5')
                return filer, package, "production", '/'.join([version, build]), None

        #  not a filer5 package, try filer6, artisan builds take priority
        #  Artisan packages are <name>/<branch>/<build>/<name>-<build-type>/<version>
        filer, package, found_branch, found_build = locate_artisan_package_on_filer6(name, branch, build, build_type, version)
        
        print("artisan Filer {}, package {}, found_branch {}, found_build {}".format(filer, package, found_branch, found_build))
        
        if package:
            commit = found_build.split('-')[1]
            print('Found an Artisan build on filer6')
            return filer, package, found_branch, found_build, commit

        #  Scaffold packages are <branch>/<build>/<name>/<version>
        filer, package, found_build = locate_scaffold_package_on_filer6(branch, build, name, version)
        print("Scaffold Filer {}, package {}, found_branch {}, found_build {}".format(filer, package, found_branch, found_build))
        if package:
            changelist = found_build.split('-')[1]
            print('Found a Scaffold build on filer6')
            return filer, package, None, found_build, changelist
    except TypeError:
        raise TypeError('Package not found {}:{}:{}:{}:{}'.format(name, branch, version, build, build_type))
    raise OSError(2, 'Package not found {}:{}:{}:{}:{}'.format(name, branch, version, build, build_type))


def main(args):
    if not args:
        print('Usage filer.py package ...')
        sys.exit(1)

    for arg in args:
        filer, path = locate_package(arg)
        print(os.path.join(filer, path))


if __name__ == '__main__':
    main(sys.argv[1:])
