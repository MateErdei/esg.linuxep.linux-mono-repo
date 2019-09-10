#!/usr/bin/env python

"""
Fixup ide filesets

For the specialty warehouse set we need 3 different sets of IDEs, this script
slices up the full test IDE set to make what we need.
"""

import sys
import optparse
import logging
import os
import glob
from shutil import copytree

from sdds import ImportFile

USAGE = "%prog [options] <ide fileset folder>"


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
    parser.add_option('-v', '--verbose', action="store_true", dest='verbose')
    # customized description; put --help last
    parser.add_option('-h', '--help', action='help',
                      help='Show this help message and exit.')

    options, args = parser.parse_args(argv)

    # check number of arguments, verify values, etc.:
    if not args:
        parser.error('program requires a command-line argument')
    if len(args) > 1:
        parser.error('program takes exactly 1 command-line argument; '
                     '"%s" ignored.' % (args[1:],))

    # further process settings & args if necessary

    return options, args


def main(argv=None):
    logging.basicConfig(format="%(levelname)s:%(asctime)s:%(message)s")
    options, args = process_command_line(argv)
    if options.verbose:
        logging.getLogger('').setLevel(logging.DEBUG)

    copytree(args[0], "ides-null")
    files_to_delete = glob.glob(os.path.join("ides-null", "*.ide"))
    for f in files_to_delete:
        os.remove(f)
    files_to_delete = glob.glob(os.path.join("ides-null", "*.dat"))
    for f in files_to_delete:
        os.remove(f)

    import_file = ImportFile(os.path.join("ides-null", "SDDS-Import.xml"))
    import_file.fixup_version("1.0.1")
    for f in import_file.file_list():
        if f.get_name()[-4:] == '.ide'or f.get_name()[-4:] == '.dat':
            import_file.file_list().remove(f)
    import_file.write()

    copytree(args[0], "ides-partial")
    files_to_delete = glob.glob(os.path.join("ides-partial", "*.ide"))
    for f in files_to_delete:
        if os.path.basename(f)[:1] != "a":
            os.remove(f)

    import_file = ImportFile(os.path.join("ides-partial", "SDDS-Import.xml"))
    import_file.fixup_version("1.0.2")
    for f in import_file.file_list():
        if f.get_name()[-4:] == '.ide' and f.get_name()[:1] != "a":
            import_file.file_list().remove(f)
    import_file.write()

    return 0        # success

if __name__ == '__main__':
    status = main()
    sys.exit(status)
