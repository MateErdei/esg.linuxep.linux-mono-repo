# Copyright 2023 Sophos Limited. All rights reserved.

import argparse
import xml.dom.minidom

TEMPLATE = """<?xml version="1.0" encoding="utf-8" ?>
<SDDSData>
    <ProductInstance>
        <!-- Product Instance Identity (Primary) -->
        <ProductLineId></ProductLineId>
        <ProductVersionId>@ComponentAutoVersion@</ProductVersionId>
        <!-- Product Instance Identity (Secondary) -->
        <ProductLineCanonicalName></ProductLineCanonicalName>
        <ProductVersionLabels>
            <Label></Label>
        </ProductVersionLabels>
        <ProductInstanceNames>
            <Name></Name>
        </ProductInstanceNames>
        <!-- Product Instance Form -->
        <Form>simple</Form>
        <Managed>1</Managed>
        <SignedManifest>1</SignedManifest>
        <!-- Product Instance Details -->
        <DefaultHomeFolder>sspl-base</DefaultHomeFolder>
        <Features>
        </Features>
        <Platforms>
        </Platforms>
        <Roles>
        </Roles>
        <TargetTypes>
        </TargetTypes>
    </ProductInstance>
</SDDSData>
"""


def remove_blanks(node):
    for x in node.childNodes:
        if x.nodeType == xml.dom.Node.TEXT_NODE:
            if x.nodeValue:
                x.nodeValue = x.nodeValue.strip()
        elif x.nodeType == xml.dom.Node.ELEMENT_NODE:
            remove_blanks(x)


def tidy_xml(doc):
    remove_blanks(doc)
    doc.normalize()


def set_text_in_tag(doc, tag, text):
    text_node = doc.createTextNode(text)
    doc.getElementsByTagName(tag)[0].appendChild(text_node)


def set_list(doc, list_tag, entry_tag, entries):
    list_node = doc.getElementsByTagName(list_tag)[0]
    for entry in entries:
        node = doc.createElement(entry_tag)
        text_node = doc.createTextNode(entry)
        node.appendChild(text_node)
        list_node.appendChild(node)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True, type=argparse.FileType("w"))
    parser.add_argument("--product_line_id", required=True)
    parser.add_argument("--product_line_canonical_name", required=True)
    parser.add_argument("--features", nargs="+", default=[])
    parser.add_argument("--platforms", nargs="+", default=[])
    parser.add_argument("--roles", nargs="+", default=[])
    parser.add_argument("--target_types", nargs="+", default=[])

    args = parser.parse_args()

    doc = xml.dom.minidom.parseString(TEMPLATE)
    tidy_xml(doc)

    set_text_in_tag(doc, "ProductLineId", args.product_line_id)
    set_text_in_tag(doc, "ProductLineCanonicalName", args.product_line_canonical_name)
    set_text_in_tag(doc, "Label", args.product_line_canonical_name)
    set_text_in_tag(doc, "Name", args.product_line_canonical_name)
    set_list(doc, "Features", "Feature", args.features)
    set_list(doc, "Platforms", "Platform", args.platforms)
    set_list(doc, "Roles", "Role", args.roles)
    set_list(doc, "TargetTypes", "TargetType", args.target_types)

    xmlstr = doc.toprettyxml()

    args.output.write(xmlstr)


if __name__ == "__main__":
    main()
