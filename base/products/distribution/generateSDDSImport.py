#!/usr/bin/env python

from __future__ import absolute_import, print_function, division, unicode_literals

import re
import os
import xml.dom.minidom

TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<ComponentData>
  <Component>
    <Name></Name>
    <RigidName></RigidName>
    <Version></Version>
    <Build>1</Build>
    <ProductType>Component</ProductType>
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
    fileNode.setAttribute("Name", fileobj.basename())
    fileNode.setAttribute("Size", str(fileobj.m_length))
    directory = fileobj.dirname()
    if directory != "":
        fileNode.setAttribute("Offset", directory)

    filelist.appendChild(fileNode)

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


def readVersionIniFile():
    scriptPath = os.path.dirname(os.path.realpath(__file__))
    BASE = os.environ.get("BASE", None)
    version = None

    autoVersionFile = os.path.join(scriptPath, "include", "AutoVersioningHeaders", "AutoVersion.ini")
    if BASE is not None and not os.path.isfile(autoVersionFile):
        autoVersionFile = os.path.join(BASE,"products","distribution","include", "AutoVersioningHeaders", "AutoVersion.ini")

    if os.path.isfile(autoVersionFile):
        print ("Reading version from {}".format(autoVersionFile))
        with open(autoVersionFile,"r") as f:
            for line in f.readlines():
                if "ComponentAutoVersion=" in line:
                    version = line.strip().split("=")[1]
    else:
        print("Failed to get AutoVersion from {}".format(autoVersionFile))

    if version == "1.0.999":
        print("Ignoring template version {} from {}".format(version, autoVersionFile))
        version = None

    return version


def readVersionFromJenkinsFile():
    scriptPath = os.path.dirname(os.path.realpath(__file__))
    BASE = os.environ.get("BASE", None)
    ## Try reading from Jenkinsfile
    productsDir = os.path.dirname(scriptPath)
    srcdir = os.path.dirname(productsDir)
    jenkinsfile = os.path.join(srcdir, "Jenkinsfile")
    if not os.path.isfile(jenkinsfile):
        ## if we are in a plugin then the script is in a sub-directory 1 deeper.
        srcdir = os.path.dirname(srcdir)
        jenkinsfile = os.path.join(srcdir, "Jenkinsfile")
    if BASE is not None and not os.path.isfile(jenkinsfile):
        ## Try base
        jenkinsfile = os.path.join(BASE, "Jenkinsfile")


    if os.path.isfile(jenkinsfile):
        lines = open(jenkinsfile).readlines()
        print("Reading version from {}".format(jenkinsfile))
        LINE_RE=re.compile(r"version_key = '([\d.]+)'$")
        for line in lines:
            line = line.strip()
            mo = LINE_RE.match(line)
            if mo:
                return mo.group(1)
    else:
        print("Failed to find Jenkinsfile {} from {}".format(jenkinsfile, scriptPath))

    return None


def defaultVersion():
    defaultValue = "0.5.1"
    print ("Using default {}".format(defaultValue))
    return defaultValue


def readVersion():
    """
    10 possible use cases:
    (Base or Plugin) *
    (
        manual build using cmake directly - e.g. CLion
        local build.sh
        Local Jenkins
        ESG-CI (Artisan-CI)
        Production Artisan
    )

    manual build has to find version without $BASE being set
        Base finds it relative to the script dir
        Plugins find it up a directory from the script dir
    other builds should have BASE set correctly.

    :return:
    """
    version = readVersionIniFile() or readVersionFromJenkinsFile() or defaultVersion()
    print("Using version {}".format(version))
    return version


def getVariable(environmentVariable, fileName, variableName, defaultValue):
    temp = os.environ.get(environmentVariable, None)
    if temp is not None:
        return temp

    try:
        return open(fileName).read().strip()
    except EnvironmentError as e:
        print("Failed to read", variableName, "from", fileName, "in", os.getcwd(), ":", e, ", using default:",
              defaultValue)
        pass

    return defaultValue

def getProductName():
    temp = os.environ.get("PLUGIN_NAME", None)
    if temp is not None:
        return temp

    return getVariable("PRODUCT_NAME", "PRODUCT_NAME", "Product/Plugin Name", "Sophos Server Protection Linux - Base")

def getRigidName():
    return getVariable("PRODUCT_LINE_ID", "RIGID_NAME", "Rigid name", "ServerProtectionLinux-Base")

def generate_sdds_import(dist, file_objects):
    sdds_import_path = os.path.join(dist, b"SDDS-Import.xml")
    doc = xml.dom.minidom.parseString(TEMPLATE)
    tidyXml(doc)

    productName = getProductName()
    fullVersion = readVersion()
    rigidName = getRigidName()
    defaultHomeFolder = getVariable("DEFAULT_HOME_FOLDER", "DEFAULT_HOME_FOLDER", "defaultHomeFolder", "sspl-base")

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

    setTextInTag(doc, "RigidName", rigidName)
    setTextInTag(doc, "Version", fullVersion)
    setTextInTag(doc, "Name", productName)
    setTextInTag(doc, "DefaultHomeFolder", defaultHomeFolder)

    token = rigidName+"#"+fullVersion
    setTextInTag(doc, "Token", token)

    shortDescription = fullVersion
    setTextInTag(doc, "ShortDesc", shortDescription)

    longDescription = "Sophos Server Protection for Linux v%s" % fullVersion
    setTextInTag(doc, "LongDesc", longDescription)


    f = open(sdds_import_path, "wb")
    doc.writexml(f, encoding="UTF-8")
    f.close()
    doc.unlink()
