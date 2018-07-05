#!/usr/bin/env python

from __future__ import absolute_import,print_function,division,unicode_literals

import os

def generate_manifest(dist, file_objects):
    manifest_path = os.path.join(dist, b"manifest.dat")
    output = open(manifest_path, "wb")
    for f in file_objects:
        display_path = b".\\" + f.m_path.replace(b"/", b"\\")
        output.write(b'"%s" %d %s\n' % (display_path, f.m_length, f.m_sha1))
        output.write(b'#sha256 %s\n' % f.m_sha256)
        output.write(b'#sha384 %s\n' % f.m_sha384)

    output.write(b"We need to add signatures here\n")
