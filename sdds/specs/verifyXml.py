#!/usr/bin/env python3

import os
import subprocess
import sys
import xml.dom.minidom

for x in sys.argv[1:]:
    d = xml.dom.minidom.parse(open(x))
    d.unlink()
    
    if x.endswith("pub_spec.xml"):
        subprocess.check_call(
            ['xmllint', '--noout', '--schema', "PubSpec.xsd", x])
    elif x.endswith("import_spec.xml"):
        subprocess.check_call(
            ['xmllint', '--noout', '--schema', "ImportSpec.xsd", x])
