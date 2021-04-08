#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import sys
import xml.dom.minidom

"""

  <imports>
    <line id="ServerProtectionLinux-Plugin-MDR">
      <componentsuite 
        fileset="\\uk-filer5.prod.sophos\prodro\bfr\214\214962\sspl-mdr-componentsuite\output\SDDS-SSPL-MDR-COMPONENT-SUITE"
        id="ServerProtectionLinux-Plugin-MDR-0.5.2.23.120.28.2332.28"
        longdic="Sophos Protection for linux MDR v0.5.2" shortdic="0.5.2"
        marketingversion="SSPL 0.5.2 MDR" version="0.5.2.23.120.28.2332.28"/>
    </line>
    <line id="ServerProtectionLinux-MDR-Control-Component">
      <component id="ServerProtectionLinux-MDR-Control-Component-0.5.2.23" version="0.5.2.23" fileset="\\uk-filer5.prod.sophos\prodro\bfr\214\214962\sspl-mdr-componentsuite\output\SDDS-SSPL-MDR-COMPONENT" longdic="Sophos Protection for linux MDR Control Component v0.5.2" shortdic="0.5.2"/>
    </line>
    <line id="ServerProtectionLinux-MDR-DBOS-Component">
      <component id="ServerProtectionLinux-MDR-DBOS-Component-2.0.28" version="2.0.28" fileset="\\uk-filer5.prod.sophos\prodro\bfr\214\214962\sspl-mdr-componentsuite\output\SDDS-SSPL-DBOS-COMPONENT" longdic="Sophos Protection for linux MDR DBOS Component v2.0" shortdic="2.0"/>
    </line>
    <line id="ServerProtectionLinux-MDR-osquery-Component">
      <component id="ServerProtectionLinux-MDR-osquery-Component-3.3.2.28" version="3.3.2.28" fileset="\\uk-filer5.prod.sophos\prodro\bfr\214\214962\sspl-mdr-componentsuite\output\SDDS-SSPL-OSQUERY-COMPONENT" longdic="Sophos Protection for linux MDR osquery Component v3.3.2" shortdic="3.3.2"/>
    </line>
  </imports>
  
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
        
def componentsuite(output):
    global PRODUCT_NAME
    sdds = os.path.join(output, "SDDS-SSPL-MDR-COMPONENT-SUITE")
    c = readSDDSImport(sdds)
    version = c.version
    c.importreference = "{}-{}".format(c.rigidName, version)
    c.short = shortVersion(version)
    c.longdic = PRODUCT_NAME+" MDR v{}".format(c.short)
    c.marketingversion = "{} {} MDR".format(SHORT_PRODUCT_NAME, c.short)
    print('    <line id="{}">'.format(c.rigidName))
    print('      <componentsuite ')
    print('        id="{}"'.format(c.importreference))
    print('        version="{}"'.format(version))
    print('        fileset="{}"'.format(fileset(sdds)))
    print('        longdic="{}" shortdic="{}"'.format(c.longdic, c.short))
    print('        marketingversion="{}" />'.format(c.marketingversion))
    print('    </line>')
    return c

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
    print("IMPORT SPEC:")
    global PRODUCT_NAME
    suite = componentsuite(output)
    suite.components = []
    c = component(output, "SDDS-SSPL-MDR-COMPONENT", PRODUCT_NAME+" MDR Control Component")
    suite.components.append(c)
    c = component(output, "SDDS-SSPL-DBOS-COMPONENT", PRODUCT_NAME+" MDR DBOS Component")
    suite.components.append(c)
    c = component(output, "SDDS-SSPL-OSQUERY-COMPONENT", PRODUCT_NAME+" MDR osquery Component")
    suite.components.append(c)
    
    return suite
    
def pubspec_componentsuite(output, suite):
    """
    <line id="ServerProtectionLinux-Plugin-MDR">
      <componentsuite importreference="ServerProtectionLinux-Plugin-MDR-0.5.2.23.120.28.2332.28">
        <component importreference="ServerProtectionLinux-MDR-Control-Component-0.5.2.23" line="ServerProtectionLinux-MDR-Control-Component" mountpoint="."/>
        <component importreference="ServerProtectionLinux-MDR-DBOS-Component-2.0.28" line="ServerProtectionLinux-MDR-DBOS-Component" mountpoint="."/>
        <component importreference="ServerProtectionLinux-MDR-osquery-Component-3.3.2.28" line="ServerProtectionLinux-MDR-osquery-Component" mountpoint="."/>
      </componentsuite>
    """
    print('    <line id="{}">'.format(suite.rigidName))
    print('      <componentsuite importreference="{}">'.format(suite.importreference))
    for c in suite.components:
        print('        <component importreference="{}" line="{}" mountpoint="."/>'.format(c.importreference, c.rigidName))
    print('      </componentsuite>')
    print('    </line>')
    
def pubspec_warehouse(output, suite):
    """
    
  <warehouses>
    <warehouse furcatedentitlement="CLOUD_SSPL_MDR" id="ssplmdr">
      <line id="ServerProtectionLinux-Plugin-MDR">
        <component importreference="ServerProtectionLinux-Plugin-MDR-0.5.2.23.120.28.2332.28">
            <releasetag baseversion="" tag="RECOMMENDED"/>
        </component>
      </line>
      <line id="ServerProtectionLinux-MDR-Control-Component">
        <component importreference="ServerProtectionLinux-MDR-Control-Component-0.5.2.23"/>
      </line>
      <line id="ServerProtectionLinux-MDR-DBOS-Component">
        <component importreference="ServerProtectionLinux-MDR-DBOS-Component-2.0.28"/>
      </line>
      <line id="ServerProtectionLinux-MDR-osquery-Component">
        <component importreference="ServerProtectionLinux-MDR-osquery-Component-3.3.2.28"/>
      </line>
    </warehouse>
  </warehouses>
    """
    print('    <warehouse furcatedentitlement="CLOUD_SSPL_MDR" id="ssplmdr">')
    print('      <line id="{}">'.format(suite.rigidName))
    print('        <component importreference="{}">'.format(suite.importreference))
    print('          <releasetag baseversion="" tag="RECOMMENDED"/>')
    print('        </component>')
    print('      </line>')
    for c in suite.components:
        print('      <line id="{}">'.format(c.rigidName))
        print('        <component importreference="{}"/>'.format(c.importreference))
        print('      </line>')
    print('    </warehouse>')
    
    
def pubspec(output, suite):
    """
  <componentsuites>
    <line id="ServerProtectionLinux-Plugin-MDR">
      <componentsuite importreference="ServerProtectionLinux-Plugin-MDR-0.5.2.23.120.28.2332.28">
        <component importreference="ServerProtectionLinux-MDR-Control-Component-0.5.2.23" line="ServerProtectionLinux-MDR-Control-Component" mountpoint="."/>
        <component importreference="ServerProtectionLinux-MDR-DBOS-Component-2.0.28" line="ServerProtectionLinux-MDR-DBOS-Component" mountpoint="."/>
        <component importreference="ServerProtectionLinux-MDR-osquery-Component-3.3.2.28" line="ServerProtectionLinux-MDR-osquery-Component" mountpoint="."/>
      </componentsuite>
    </line>
  </componentsuites>
  <warehouses>
    <warehouse furcatedentitlement="CLOUD_SSPL_MDR" id="ssplmdr">
      <line id="ServerProtectionLinux-Plugin-MDR">
        <component importreference="ServerProtectionLinux-Plugin-MDR-0.5.2.23.120.28.2332.28">
            <releasetag baseversion="" tag="RECOMMENDED"/>
        </component>
      </line>
      <line id="ServerProtectionLinux-MDR-Control-Component">
        <component importreference="ServerProtectionLinux-MDR-Control-Component-0.5.2.23"/>
      </line>
      <line id="ServerProtectionLinux-MDR-DBOS-Component">
        <component importreference="ServerProtectionLinux-MDR-DBOS-Component-2.0.28"/>
      </line>
      <line id="ServerProtectionLinux-MDR-osquery-Component">
        <component importreference="ServerProtectionLinux-MDR-osquery-Component-3.3.2.28"/>
      </line>
    </warehouse>
  </warehouses>
  """
    print("PUB SPEC:")
    pubspec_componentsuite(output, suite)
    pubspec_warehouse(output, suite)
    return 0

def main(argv):
    OUTPUT=argv[1]
    assert os.path.isdir(OUTPUT)
    OUTPUT = addOutputIfRequired(OUTPUT)
    
    suite = importspec(OUTPUT)
    return pubspec(OUTPUT, suite)
    
    

if __name__ == '__main__':
    sys.exit(main(sys.argv))
