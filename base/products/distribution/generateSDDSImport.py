#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.



import fileInfo
import readVersion
import os
import sys
import xml.dom.minidom

TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<ComponentData>
  <Component>
    <Name></Name>
    <RigidName></RigidName>
    <Version></Version>
    <Build>1</Build>
    <ProductType></ProductType>
    <DefaultHomeFolder></DefaultHomeFolder>
    <TargetTypes>
      <TargetType Name="ENDPOINT"/>
    </TargetTypes>
    <Roles>
      <Role Name="SAU"/>
    </Roles>
    <Platforms>
      <Platform Name="LINUX_INTEL_LIBC6"/>
    </Platforms>
    <Features>
    </Features>
    <FileList>
    </FileList>
  </Component>
  <Dictionary>
    <Name>
      <Label>
        <Token></Token>
        <Language lang="en">
          <ShortDesc></ShortDesc>
          <LongDesc></LongDesc>
        </Language>
      </Label>
    </Name>
  </Dictionary>
</ComponentData>
"""

TELEMSUPP_TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<ComponentData>
  <Component>
    <Name></Name>
    <RigidName></RigidName>
    <Version></Version>
    <Build>1</Build>
    <ProductType></ProductType>
    <DefaultHomeFolder></DefaultHomeFolder>
    <TargetTypes/>
    <Roles/>
    <Platforms/>
    <Features/>
    <FileList>
    </FileList>
  </Component>
  <Dictionary>
    <Name>
      <Label>
        <Token></Token>
        <Language lang="en">
          <ShortDesc></ShortDesc>
          <LongDesc></LongDesc>
        </Language>
      </Label>
    </Name>
  </Dictionary>
</ComponentData>
"""


PY3 = sys.version_info[0] == 3


unicode_str = str
byte_str = bytes


def ensure_binary_string(s):
    if isinstance(s, unicode_str):
        return s.encode("UTF-8")
    return s

def ensure_unicode_string(s):
    if isinstance(s, byte_str):
        return s.decode("UTF-8")
    return s


def addFile(doc, filelist, fileobj):
    """
      <File MD5="62d286b6c9aa3243610ed22dbc3b85f8" Name="savfeedback.key" Offset="sav-linux/common/etc" Size="450"/>

    :param doc:
    :param filelist:
    :param fileobj:
    :return:
    """
    fileNode = doc.createElement("File")
    fileNode.setAttribute("MD5", fileobj.m_md5)
    fileNode.setAttribute("SHA384", fileobj.m_sha384)
    fileNode.setAttribute("Name", ensure_unicode_string(fileobj.basename()))
    fileNode.setAttribute("Size", str(fileobj.m_length))
    directory = fileobj.dirname()
    if directory != "":
        fileNode.setAttribute("Offset", ensure_unicode_string(directory))

    filelist.appendChild(fileNode)

def addFeature(doc, featureList, feature):
    """
    <Feature id="AV"/>
    """
    featureNode = doc.createElement("Feature")
    featureNode.setAttribute("Name", feature)
    featureList.appendChild(featureNode)

def remove_blanks(node):
    for x in node.childNodes:
        if x.nodeType == xml.dom.Node.TEXT_NODE:
            if x.nodeValue:
                x.nodeValue = x.nodeValue.strip()
        elif x.nodeType == xml.dom.Node.ELEMENT_NODE:
            remove_blanks(x)

def tidyXml(doc):
    remove_blanks(doc)
    doc.normalize()

def setTextInNode(doc, node, text):
    text_node = doc.createTextNode(text)
    node.appendChild(text_node)

def setTextInTag(doc, tag, text):
    return setTextInNode(doc, doc.getElementsByTagName(tag)[0], text)

def getXmlText(node):
    """Get the text out of an XML node.
    """
    text = ""
    for n in node.childNodes:
        text += n.data
    return text


def getVariable(environmentVariable, fileName, variableName, defaultValue, reportAbsentFile=True):
    temp = os.environ.get(environmentVariable, None)
    if temp is not None:
        return temp

    try:
        return open(fileName).read().strip()
    except EnvironmentError as e:
        if reportAbsentFile:
            print("Failed to read", variableName, "from", fileName, "in", os.getcwd(), ":", e, ", using default:",
                  defaultValue)

    return defaultValue

def getProductName():
    temp = os.environ.get("PLUGIN_NAME", None)
    if temp is not None:
        return temp

    return getVariable("PRODUCT_NAME", "PRODUCT_NAME", "Product/Plugin Name", "Sophos Server Protection Linux - Base")

def getFeatureList():
    #   Read csv list of features of the form: feature1, feature2, feature3
    features_string = getVariable("FEATURE_LIST", "FEATURE_LIST", "Feature List", "")
    if features_string == "":
        return []
    return features_string.split(", ")


def getRigidName():
    return getVariable("PRODUCT_LINE_ID", "RIGID_NAME", "Rigid name", "ServerProtectionLinux-Base-component")

def get_sdds_import_tempate():

    template = os.environ.get("SDDSTEMPLATE", "TEMPLATE")

    if template == "TEMPLATE":
        return TEMPLATE
    elif template == "TELEMSUPP_TEMPLATE":
        return TELEMSUPP_TEMPLATE
    else:
        raise Exception("Unknown SDDS template given: {}".format(template))


def generate_sdds_import(dist, file_objects, BASE=None):
    sdds_import_path = os.path.join(dist, "SDDS-Import.xml")

    doc = xml.dom.minidom.parseString(get_sdds_import_tempate())
    tidyXml(doc)

    productName = getProductName()
    fullVersion = readVersion.readVersion(BASE)
    rigidName = getRigidName()
    defaultHomeFolder = getVariable("DEFAULT_HOME_FOLDER", "DEFAULT_HOME_FOLDER", "defaultHomeFolder", "sspl-base")
    featureList = getFeatureList()
    productType = getVariable("PRODUCT_TYPE", "PRODUCT_TYPE", "productType", "Component", False)

    filelistNode = doc.getElementsByTagName("FileList")[0]
    for f in file_objects:
        addFile(doc, filelistNode, f)
        base = f.basename()
        if base == "fullVersion":
            fullVersion = f.contents().strip()
        elif base == "sddsVersion":
            fullVersion = f.contents().strip()
        elif base == "rigidName":
            rigidName = f.contents().strip()

    featureListNode = doc.getElementsByTagName("Features")[0]
    for feature in featureList:
        addFeature(doc, featureListNode, feature)

    setTextInTag(doc, "RigidName", rigidName)
    setTextInTag(doc, "Version", fullVersion)
    setTextInTag(doc, "Name", productName)
    setTextInTag(doc, "DefaultHomeFolder", defaultHomeFolder)
    setTextInTag(doc, "ProductType", productType)

    token = rigidName+"#"+fullVersion
    setTextInTag(doc, "Token", token)

    shortDescription = getVariable("SHORT_DESCRIPTION", "SHORT_DESCRIPTION", "shortDescription", fullVersion, reportAbsentFile=False)
    setTextInTag(doc, "ShortDesc", shortDescription)

    longDescription = "Sophos Linux Protection Base Component v%s" % fullVersion
    longDescription = getVariable("LONG_DESCRIPTION", "LONG_DESCRIPTION", "longDescription", longDescription, reportAbsentFile=False)
    setTextInTag(doc, "LongDesc", longDescription)

    xmlstr = doc.toxml('UTF-8')
    doc.unlink()

    f = open(sdds_import_path, "wb")
    f.write(xmlstr)
    f.close()

def main(argv):
    dist = argv[1]
    if len(argv) > 2:
        distribution_list = argv[2]
    else:
        distribution_list = None
    if len(argv) > 3:
        BASE = argv[3]
    else:
        BASE = os.environ.get("BASE", None)

    ## Don't exclude manifest from SDDS-Import.xml direct generation
    file_objects = fileInfo.load_file_info(dist, distribution_list, excludeManifest=False)
    generate_sdds_import(dist, file_objects, BASE)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
