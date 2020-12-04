"""
This is the warehouses module for the warehouse definition maker.

"""
import xml.etree.ElementTree as etree
import sys
import os
import logging
import re
from glob import glob


class Supplement:
    def __init__(self, warehouse, id, tag):
        self._id = id
        self._warehouse = warehouse
        self._tag = tag
        self._base_version = None
        self._decode = None

    def set_decode_path(self, path):
        self._decode = path

    def set_base_version(self, base_version):
        self._base_version = str(base_version)

    def to_element_tree(self):
        root = etree.Element("Supplement")
        etree.SubElement(root, "Warehouse", id=self._warehouse)
        inst = etree.SubElement(root, "Instance",
                                {'line': self._id,
                                 'versiontype': "generic",
                                 'tag': self._tag})
        if self._base_version != None:
            inst.set('baseversion', self._base_version)
        decode = etree.SubElement(root, "DecodePath")
        if self._decode != None:
            decode.text = self._decode
        return root


class Warehouse:
    def __init__(self, id, type):
        self._id = id
        self._type = type
        self._items = {}

    @property
    def id(self):
        return self._id

    def add_item(self, item):
        self._items.setdefault(item.id, [])
        self._items[item.id].append(item)

    def to_element_tree(self):
        root = etree.Element("Warehouse", type=self._type, id=self._id, snapshot="true")
        lines = etree.SubElement(root, "Lines")
        for line_id, items in self._items.items():
            line = etree.SubElement(lines, "Line", id=line_id)
            instances = etree.SubElement(line, "Instances")
            for i in items:
                instances.append(i.to_element_tree())
        return root


class WarehouseItem(object):
    def __init__(self, line_id, version):
        self._id = line_id
        self._version = version
        self._promotional_state = None
        self._release_tags = []
        self._resubscription = None
        self._supplements = []

    @property
    def id(self):
        return self._id

    @property
    def version(self):
        return self._version

    def set_promotional_state(self, state):
        self._promotional_state = str(state)

    def set_release_tags(self, tags):
        self._release_tags = tags

    def set_resubscription(self, line_id, baseversion, version):
        self._resubscription = (line_id, str(baseversion), str(version))

    def add_supplement(self, supplement):
        self._supplements.append(supplement)

    def to_element_tree(self):
        instance = etree.Element("Instance", version=self._version)
        if self._promotional_state is not None:
            instance.set("promotionalstate", self._promotional_state)
        if len(self._supplements) != 0:
            supps = etree.SubElement(instance, "Supplements")
            for s in self._supplements:
                supps.append(s.to_element_tree())
        if self._release_tags:
            tags = etree.SubElement(instance, "ReleaseTags")
            for tagentry in self._release_tags:
                tag = tagentry['tag']
                supp = etree.SubElement(tags, "ReleaseTag", {'tag': tag})
                if 'base-version' in tagentry:
                    supp.set('baseversion', str(tagentry['base-version']))

        if self._resubscription is not None:
            resubs = etree.SubElement(instance, "Resubscriptions")
            (id, bv, v) = self._resubscription
            etree.SubElement(resubs, "Resubscription", line=id, baseversion=bv, version=v)
        return instance


if __name__ == "__main__":
    logging.basicConfig()
