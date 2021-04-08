#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import sys
import xml.dom.minidom

"""
    <line id="ServerProtectionLinux-Base">
      <component id="ServerProtectionLinux-Base-0.5.1.792" 
                 version="0.5.1.792" 
                 fileset="\\uk-filer5.prod.sophos\prodro\bfr\216\216202\sspl-base\output\SDDS-COMPONENT" 
                 longdic="Sophos Server Protection for Linux v0.5.1" 
                 shortdic="0.5.1"/>
    </line>
    
    
      <line id="ServerProtectionLinux-Base">
        <component importreference="ServerProtectionLinux-Base-0.5.1.792">
            <releasetag baseversion="" tag="RECOMMENDED"/>
        </component>
      </line>
  
"""

def addOutputIfRequired(o):
    output = os.path.join(o,"output")
    if os.path.isdir(output):
        return output
    return o
    
def fileset(p):
    if p.startswith("/uk-filer5/"):
        return "\\" + p.replace("/","\\").replace("uk-filer5","uk-filer5.prod.sophos")
        
def readFile(*p):
    p = os.path.join(*p)
    return open(p).read().strip()
    
def shortVersion(v):
    parts = v.split(".")
    return ".".join(parts[:3])

class Component(object):
    pass
    
def getTextFromNode(node):
    """Get the text out of an XML node.
    """
    text = ""
    for n in node.childNodes:
        text += n.data
    return text

def getTextFromElement(dom, tagname):
    return getTextFromNode(dom.getElementsByTagName(tagname)[0])

def readSDDSImport(*sdds):
    import_xml = os.path.join(os.path.join(*sdds), "SDDS-Import.xml")
    assert os.path.isfile(import_xml), import_xml+" doesn't exist"
    dom = xml.dom.minidom.parse(open(import_xml))
    
    c = Component()
    c.version = getTextFromElement(dom, "Version")
    c.rigidName = getTextFromElement(dom, "RigidName")
    c.sdds = sdds
    return c

PRODUCT_NAME="Sophos Server Protection for Linux"
SHORT_PRODUCT_NAME="SSPL"
        
def component(output, sdds, longdic):
    sdds = os.path.join(output, sdds)
    component = readSDDSImport(sdds)
    version = component.version
    rigidName = component.rigidName
    short = shortVersion(version)
    component.short = short
    component.longdic = "{} v{}".format(longdic, component.short)
    component.importreference = "{}-{}".format(rigidName, version)
    print('    <line id="{}">'.format(rigidName))
    print('      <component ')
    print('        id="{}"'.format(component.importreference))
    print('        version="{}"'.format(version))
    print('        fileset="{}"'.format(fileset(sdds)))
    print('        longdic="{}" shortdic="{}" />'.format(component.longdic, short))
    print('    </line>')
    return component
    
def importspec(output):
    """
    <line id="ServerProtectionLinux-Base">
      <component id="ServerProtectionLinux-Base-0.5.1.792" 
                 version="0.5.1.792" 
                 fileset="\\uk-filer5.prod.sophos\prodro\bfr\216\216202\sspl-base\output\SDDS-COMPONENT" 
                 longdic="Sophos Server Protection for Linux v0.5.1" 
                 shortdic="0.5.1"/>
    </line>
    """
    print("IMPORT SPEC:")
    global PRODUCT_NAME
    c = component(output, "SDDS-COMPONENT", PRODUCT_NAME)
    return c
    
def pubspec_warehouse(output, suite):
    """
    
    
      <line id="ServerProtectionLinux-Base">
        <component importreference="ServerProtectionLinux-Base-0.5.1.792">
            <releasetag baseversion="" tag="RECOMMENDED"/>
        </component>
      </line>
  
    """
    print('      <line id="{}">'.format(suite.rigidName))
    print('        <component importreference="{}">'.format(suite.importreference))
    print('          <releasetag baseversion="" tag="RECOMMENDED"/>')
    print('        </component>')
    print('      </line>')
    
    
def pubspec(output, c):
    """
    
      <line id="ServerProtectionLinux-Base">
        <component importreference="ServerProtectionLinux-Base-0.5.1.792">
            <releasetag baseversion="" tag="RECOMMENDED"/>
        </component>
      </line>
  
  """
    print("PUB SPEC:")
    pubspec_warehouse(output, c)
    return 0

def main(argv):
    OUTPUT=argv[1]
    assert os.path.isdir(OUTPUT)
    OUTPUT = addOutputIfRequired(OUTPUT)
    
    c = importspec(OUTPUT)
    return pubspec(OUTPUT, c)
    
    

if __name__ == '__main__':
    sys.exit(main(sys.argv))
