#!/usr/bin/env python

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import xml.dom.minidom

TEMPLATE = """<?xml version="1.0" encoding="utf-8"?>
<ComponentData>
  <Component>
    <Name>Sophos Server Protection Linux - Base</Name>
    <RigidName></RigidName>
    <Version></Version>
    <Build>1</Build>
    <ProductType>Component</ProductType>
    <DefaultHomeFolder>sspl-base</DefaultHomeFolder>
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

def generate_sdds_import(dist, file_objects):
    sdds_import_path = os.path.join(dist, b"SDDS-Import.xml")
    doc = xml.dom.minidom.parseString(TEMPLATE)
    tidyXml(doc)

    fullVersion = os.environ.get("FULL_VERSION", "0.5.0.0")
    rigidName = os.environ.get("RIGID_NAME", "ServerProtectionLinux-Base")

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

    setTextInTag(doc, "Version", fullVersion)
    setTextInTag(doc, "RigidName", rigidName)

    token = rigidName+"#"+fullVersion
    setTextInTag(doc, "Token", token)

    shortDescription = fullVersion
    setTextInTag(doc, "ShortDesc", shortDescription)

    longDescription = "Sophos Server Protection for Linux v%s"%fullVersion
    setTextInTag(doc, "LongDesc", longDescription)


    f = open(sdds_import_path, "wb")
    doc.writexml(f, encoding="UTF-8")
    f.close()
    doc.unlink()
