#!/usr/bin/env python3

from __future__ import print_function,division,unicode_literals


import argparse
import hashlib
import os

from . import fileutil
from .dev_digest import LocalDigestConfig
from .digest import Digest
from .manifest import Manifest


def _get_manifest_data_for_file(folder, file, args):
    data = fileutil.read_file(os.path.join(folder, file))
    sha1 = hashlib.sha1(data).hexdigest()
    sha256 = hashlib.sha256(data).hexdigest()

    digest = '"%s" %s %s\n' % (file, len(data), sha1)
    if not args.no_sha256:
        if args.bad_sha256:
            sha256 = sha256.replace("a", "b")
        digest += '#sha256 %s\n' % sha256

    if args.extra_comment:
        digest += "#A"*200+"\n"

    return digest


def _generate_manifest_data(input_folder, exclusions, args):
    """
    Find all files in the folder, generate the manifest and then sign it.
    """
    all_files = fileutil.get_all_files(input_folder)
    all_files.sort()

    manifest_data = ''
    for file in all_files:
        if file[2:] in exclusions:
            continue

        manifest_data += _get_manifest_data_for_file(input_folder, file, args)

    return manifest_data


def _local_sign(input_folder, output_filename, exclusions, args):
    """
    Sign the file locally.  This invokes the same code that is used on the server,
    but with dev certs instead of production certs.
    """
    manifest = Manifest(Digest(config=LocalDigestConfig()))
    unsigned_manifest_data = _generate_manifest_data(input_folder, exclusions, args)
    signed_manifest_data = manifest.sign(manifest_data=unsigned_manifest_data,
                                         include_sha1=args.legacy,
                                         include_sha2=args.sha2_signature,
                                         signing_key=args.signing_key)
    fileutil.write_file(output_filename, signed_manifest_data.encode('utf-8'))


def main():
    parser = argparse.ArgumentParser(description="Manifest signing script")
    parser.add_argument("--folder", "-f", "--directory", "-d", required=True,
                        help="Folder to generate manifest for")
    parser.add_argument("--output", "-o", required=True,
                        help="Filename to write signed manifest to")
    parser.add_argument("--exclusions", "-x", default='',
                        help="Comma separated list of files in folder to exclude from manifest")
    parser.add_argument("--legacy", "-l", "--sha1-signature", default=False, help="Include the SHA1 signature to the manifest.",
                        action='store_true')
    parser.add_argument("--no-sha256", default=False, action="store_true")
    parser.add_argument("--bad-sha256", default=False, action="store_true")
    parser.add_argument("--no-sha2-signature", dest="sha2_signature", default=True, action="store_false")
    parser.add_argument("--extra-comment", default=False, action="store_true")
    parser.add_argument("--signing_key", default=None)
    args = parser.parse_args()

    exclusions = args.exclusions.split(',')

    _local_sign(input_folder=args.folder, output_filename=args.output, exclusions=exclusions, args=args)
    return 0


if __name__ == '__main__':
    main()
