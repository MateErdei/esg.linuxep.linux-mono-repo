#!/usr/bin/env python

# pylint: disable=C0103,R0912,R0914,R0915

"""
Script to create SDDS customer files
"""

import sys
import optparse
import yaml
import os
import logging
import hashlib
import time
import locations

from xmlrpclib import ServerProxy
import socket

OLD_MAGIC_TEXT = """<?xml version="1.0" encoding="utf-8" ?>
<Warehouse>
<WarehouseName>sdds.%s</WarehouseName>
<LicenseDescription>%s</LicenseDescription>
<LastModified>%s</LastModified>
<LastAssigned>%s</LastAssigned>
<FileName>%s</FileName>
%s%s
</Warehouse>
"""

URL_MAGIC_TEXT = "<URL>%s</URL>\n"

WH_BLOCK_MAGIC_TEXT = """<Warehouses>
%s</Warehouses>"""

SPLIT_WH_MAGIC_TEXT = """<WarehouseEntry>
<Name>sdds.%s</Name>
</WarehouseEntry>\n"""

NEW_DCI_MAGIC_TEXT = """<?xml version="1.0" encoding="utf-8" ?>
<Warehouse>
<LicenseDescription>%s</LicenseDescription>
<LastModified>%s</LastModified>
<LastAssigned>%s</LastAssigned>
<FileName>%s</FileName>
%s%s%s
</Warehouse>
"""

REDIRECTS_BLOCK_MAGIC_TEXT = """<Redirects>
%s</Redirects>"""

REDIRECT_MAGIC_TEXT = """<Redirect Pattern="%s" Substitution="%s"></Redirect>"""

USAGE = """%prog <warehouse def file>"""


def sign(dci_file_path):
    """
    Signs text file at the specified file path. The signatures is
    appended at the end of the file.
    """

    server = ServerProxy("http://buildsign-m:8000", allow_none=True)

    try:
        m = open(dci_file_path, "rb")
        file_data = m.read()
        m.close()
    except Exception as e:
        raise Exception("Failed to open file %s (%s)" % (dci_file_path, e))

    dci_file = open(dci_file_path, "a+b")

    try:
        dci_file.write(server.sign_file(file_data, None, None))
    except socket.error, e:
        raise Exception("ERROR - Signing Failed: %s" % e)

    for cert in ("pub", "ca"):
        try:
            dci_file.write(server.get_cert(cert, None))
        except socket.error, e:
            raise Exception("ERROR - Failed to get cert %s: %s" % (cert, e))

    dci_file.close()


def create(warehouse_def_file, options):
    """
    Create customer files according to the specified list in yaml format.
    """
    wh_input = file(warehouse_def_file).read()
    model = yaml.safe_load(wh_input)

    output_folder = locations.CUSTOMER_FILES_WRITE
    logging.info("Creating customer files at %s", output_folder)

    customer_files = model['customer-files']
    if ("redirects" in model):
        redirects = model['redirects']

    substitutions = {'codeline': options.codeline, 'buildid': options.buildid}

    for c in customer_files:
        customer_file_name = hashlib.md5(
            "%s:%s" % (c['user'], c['password'])).hexdigest()
        file_name_element_value = hashlib.md5(customer_file_name).hexdigest()
        if ('output-dir' in c):
            directory = os.path.join(
                output_folder, c['output-dir'], customer_file_name[:1],
                customer_file_name[1:3])
        else:
            directory = os.path.join(
                output_folder, customer_file_name[:1], customer_file_name[1:3])

        if ('license' in c):
            repo_id = c['license']
        else:
            repo_id = locations.REPOSITORY_ID

        urls = ""
        if ('warehouse-urls' in c):
            for template in c['warehouse-urls']:
                url = template['url'] % substitutions
                urls = urls + URL_MAGIC_TEXT % url
        else:
            for template in locations.WAREHOUSE_READ:
                url = template % substitutions
                urls = urls + URL_MAGIC_TEXT % url

        split_whs = ""
        if ('split-warehouses' in c):
            for wh in c['split-warehouses']:
                split_whs = split_whs + SPLIT_WH_MAGIC_TEXT % wh['warehouse']

        wh_block = ""
        if (split_whs != ""):
            wh_block = WH_BLOCK_MAGIC_TEXT % split_whs

        redirects_block = ""

        if ('redirects' in c):
            this_customer_redirect = c['redirects']
            for thisredirect in redirects:
                if (this_customer_redirect in thisredirect):
                    redirects_text = ""
                    thisredirect = thisredirect[this_customer_redirect]
                    for entry in thisredirect:
                        redirect = REDIRECT_MAGIC_TEXT % (
                            entry["pattern"], entry["substitution"])
                        redirects_text = redirects_text + redirect
                    redirects_block = REDIRECTS_BLOCK_MAGIC_TEXT % redirects_text

        if not os.path.exists(directory):
            os.makedirs(directory)
        old_customer_file_name = os.path.join(
            directory, "%s.xml" % customer_file_name)

        if ('last-mod' in c):
            lm = c['last-mod']
            if ('year' in lm):
                time_format = str(lm['year']).zfill(4) + "-"
            else:
                time_format = "%Y-"
            if ('month' in lm):
                time_format = time_format + str(lm['month']).zfill(2) + "-"
            else:
                time_format = time_format + "%m-"
            if ('day' in lm):
                time_format = time_format + str(lm['day']).zfill(2)
            else:
                time_format = time_format + "%d"
        else:
            time_format = "%Y-%m-%d"
        time_format = time_format + "T%H:%M:%SZ"
        current_time_str = time.strftime(time_format, time.gmtime())

        if (c['warehouse'] is not None):
            logging.info("Creating old customer file %s pointing at %s",
                         old_customer_file_name, c['warehouse'])
            f = open(old_customer_file_name, "w+b")
            f.write(OLD_MAGIC_TEXT.replace('\r\n', '\n') % (
                c['warehouse'], repo_id, current_time_str, current_time_str,
                file_name_element_value, urls, redirects_block))
            f.close()

        if (split_whs != ""):
            dci_file_path = os.path.join(
                directory, "%s.dat" % customer_file_name)
            logging.info(
                "Creating new dci file %s for warehouse %s", dci_file_path,
                c['warehouse'])
            # parent_checksum_for_now = "234567"
            ef = open(dci_file_path, "w+b")
            ef.write(NEW_DCI_MAGIC_TEXT.replace('\r\n',
                                                '\n') % (repo_id,
                                                         current_time_str,
                                                         current_time_str,
                                                         file_name_element_value,
                                                         urls, wh_block,
                                                         redirects_block))
            ef.close()
            sign(dci_file_path)


def process_command_line(argv):
    """
    Return a 2-tuple: (settings object, args list).

    `argv` is a list of arguments, or `None` for ``sys.argv[1:]``.
    """
    if argv is None:
        argv = sys.argv[1:]

    # initialize the parser object:
    parser = optparse.OptionParser(usage=USAGE,
                                   formatter=optparse.TitledHelpFormatter(
                                       width=78),
                                   add_help_option=None)

    # define options here:
    parser.add_option('-v', '--verbose', action="store_true", dest='verbose')
    parser.add_option(
        "-c", "--codeline", dest="codeline",
        help="Codeline identifier for this build")
    parser.add_option(
        "-b", "--buildid", dest="buildid", help="Build identifier")
    parser.add_option(      # customized description; put --help last
        '-h', '--help', action='help',
        help='Show this help message and exit.')

    options, args = parser.parse_args(argv)

    # check number of arguments, verify values, etc.:
    if len(args) < 1:
        parser.error('input file not specified')
    if options.codeline is None:
        parser.error('codeline must be specified')
    if options.buildid is None:
        parser.error('buildid must be specified')

    return options, args


def main(argv=None):
    """Generates customer files."""
    logging.basicConfig(format="%(levelname)s:%(asctime)s:%(message)s",
                        stream=sys.stdout)

    options, args = process_command_line(argv)

    if options.verbose:
        logging.getLogger('').setLevel(logging.DEBUG)

    create(args[0], options)

    return 0        # success

if __name__ == '__main__':
    status = main()
    sys.exit(status)
