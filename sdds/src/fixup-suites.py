#!/usr/bin/env python

"""
Fixup component suite filesets

The sddsimport tool can't override the version numbers specified
in the sddsimport files of the components it imports. For the specialty
warehouse set we need 9 different version numbers, so this script
copies and fixes up the component suite filesets.
"""

import sys
import optparse
import os
from shutil import copy2, copytree
import re
import string
from sdds import ImportFile

USAGE = "%prog [options] <suite fileset folder>"


def process_command_line(argv):
    """
    Return a 2-tuple: (settings object, args list).
    `argv` is a list of arguments, or `None` for ``sys.argv[1:]``.
    """
    if argv is None:
        argv = sys.argv[1:]

    # initialize the parser object:
    parser = optparse.OptionParser(
        formatter=optparse.TitledHelpFormatter(width=78),
        add_help_option=None, usage=USAGE)

    # define options here:
    parser.add_option(
        '-v', '--force-version', dest="version", default=None,
        help="Use this version number, not the one in the fileset")
    parser.add_option(
        '-s', '--stem', dest="stem", default=None,
        help="The stem to make folder names for the copies")
    parser.add_option(
        '-n', '--number', dest="number", type="int", default=3,
        help="The number of copies to make (default 3)")
    parser.add_option(
        '--no-stubs', dest="no_stubs", action="store_true", default=False,
        help="Don't make stubs")
    parser.add_option(
        '-c', '--no-correct', dest='correct',
        action="store_false", default=True,
        help="Don't correct the starting version number")
    # customized description; put --help last
    parser.add_option('-h', '--help', action='help',
                      help='Show this help message and exit')

    options, args = parser.parse_args(argv)

    # check number of arguments, verify values, etc.:
    if not args:
        parser.error('program requires a command-line argument')
    if len(args) > 1:
        parser.error('program takes exactly 1 command-line argument; '
                     '"%s" ignored.' % (args[1:],))

    # further process settings & args if necessary

    return options, args


def copy_and_fixup(src, stem, product, version, tag):
    version_string = "%s.%s.%s" % version
    dst = os.path.join(stem, string.join((product, tag), "-"))
    copytree(src, dst)

    import_file = ImportFile(os.path.join(dst, "SDDS-Import.xml"))
    import_file.fixup_version(version_string)
    import_file.write()


def make_stub(src, stem, product, version, tag):
    version_string = "%s.%s.%s.1" % version
    dst = os.path.join(stem, string.join((product, tag), "-"))
    if not os.path.exists(dst):
        os.makedirs(dst)
    copy2(os.path.join(src, "SDDS-Import.xml"), dst)

    import_file = ImportFile(os.path.join(dst, "SDDS-Import.xml"))
    import_file.fixup_version(version_string)
    import_file.remove_eps_role()
    file_list = import_file.file_list()
    for f in file_list:
        file_list.remove(f)
    import_file.write()


def main(argv=None):
    options, args = process_command_line(argv)
    import_file = ImportFile(os.path.join(args[0], "SDDS-Import.xml"))

    if not options.version:
        seed_version = import_file.get_version()
    else:
        seed_version = options.version

    match = re.match("(\d+)\.(\d+)\.(\d+)", seed_version)
    (major, minor, patch) = match.group(1, 2, 3)

    if options.correct and int(patch) < (options.number - 1):
        patch = str(options.number - 1)

    if not options.stem:
        stem = import_file.get_name()
    else:
        stem = options.stem

    for i in range(options.number):
        new_patch = str(int(patch) - i)
        copy_and_fixup(args[0], ".", stem, (
            major, minor, new_patch), "%s" % new_patch)
        if not options.no_stubs:
            make_stub(args[0], ".", stem, (
                major, minor, new_patch), "%s-STUB" % new_patch)

    return 0        # success

if __name__ == '__main__':
    status = main()
    sys.exit(status)
