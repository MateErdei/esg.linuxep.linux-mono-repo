"""
This is the components module for the warehouse definition maker.

"""
import xml.etree.ElementTree as etree
from glob import glob
import os
import os.path
import logging
import tempfile
import shutil
import sys

import filer


def mkdir_p(tgt):
    print('mkdir {0}'.format(tgt))
    if not os.path.exists(tgt):
        try:
            os.makedirs(tgt)
        except Exception as exception:
            print("mkdir_p ERROR: Making directory failed.")
            print(exception.message)
            raise
    else:
        print("mkdir_p WARNING: Folder already exists.")


def makeparent(dst):
    dirname = os.path.dirname(dst)
    if not os.path.isdir(dirname):
        mkdir_p(False, dirname)


def fetch_files(src_dir, dest_dir):
    # for when dest_dir exists, cos shutil.copytree needs it to not
    items = glob(os.path.join(src_dir, "*"))
    for src_item in items:
        dest_item = src_item.replace(src_dir, dest_dir)
        if not os.path.isfile(src_item):
            shutil.copytree(src_item, dest_item)
        else:
            shutil.copy2(src_item, dest_item)


class ComponentBuild:

    def __init__(self, attribs):
        if "name" not in attribs:
            # This is the only mandatory item
            raise RuntimeError("You must specify a name in the build information", attribs)

        def str_or_type(key, default=""):
            ret = attribs.get(key, None)
            return str(ret) if ret is not None else default

        self.name = str_or_type('name')
        self.version = str_or_type('version', "")
        self.build_id = str_or_type('build_id', "")
        output = str_or_type('output', "")
        self._path = filer.locate_package_from_clues(self.name, self.version, self.build_id, output)

    @property
    def path(self):
        return self._path


class Component:

    def __init__(self, instance_name, line_id, canonical_name, fileset=None, build_spec=None):
        assert (not fileset and build_spec) or (fileset and not build_spec)
        self._name = instance_name
        self._line_id = line_id
        self._canonical_name = canonical_name
        self._fileset = fileset
        self._build_spec = build_spec
        self._version = None
        self._mountpoint = None
        self._tlc = None
        self._subcomponents = []
        self._resolved_subcomponents = []
        self._features = []
        self._platforms = []
        if self._build_spec:
            self.fetch_component()

    def fetch_component(self):
        assert self._build_spec and not self._fileset
        remote_root_path = self._build_spec.path
        logging.info("searching for component SDDS-Import.xml at: {}".format(remote_root_path))
        if not os.path.isfile(os.path.join(remote_root_path, "SDDS-Import.xml")):
            raise Exception("The build for {} is not a valid SDDS import fileset.".format(self._name))
        local_root_path = os.path.join(r'..\inputs\filer5', self._name)
        if os.path.exists(local_root_path):
            logging.warning(f'Skipping fetch from {remote_root_path} to {local_root_path} - already exists')
        else:
            os.makedirs(local_root_path)
            fetch_files(remote_root_path, local_root_path)
        logging.info("Copied to {}".format(local_root_path))
        self._fileset = local_root_path

    def set_mountpoint(self, mountpoint):
        self._mountpoint = mountpoint

    def set_tlc(self, tlc):
        self._tlc = tlc

    def set_subcomponents(self, subcomp):
        self._subcomponents = subcomp

    def set_features(self, features):
        self._features = features

    def set_platforms(self, platforms):
        self._platforms = platforms

    @property
    def version(self):
        if self._version is None:
            with open(os.path.join(self._fileset, "SDDS-Import.xml")) as f:
                t = etree.ElementTree(None, f)
            self._version = t.find("*/Version").text.strip()
        return self._version

    @property
    def line_id(self):
        return self._line_id

    @property
    def canonical_name(self):
        return self._canonical_name

    @property
    def name(self):
        return self._name

    @property
    def mountpoint(self):
        return self._mountpoint

    @property
    def path(self):
        return self._fileset

    def resolve_subcomponents(self, index):
        logging.debug("Resolving subcomponents for {}".format(self._name))
        for s in self._subcomponents:
            try:
                item = index[s]
            except KeyError as e:
                logging.error(
                    "Component {} has subcomponent {} which was not found in the warehouse def file.".format(self._name,
                                                                                                             s))
                raise e
            logging.debug("Found {}".format(item))
            self._resolved_subcomponents.append(item)

    def to_element_tree(self):
        # core
        instance = etree.Element("Instance", fileset=self._fileset,
                                 version=self.version)
        if self._tlc is not None:
            instance.attrib['tlcname'] = self._tlc
        # subcomponents if there are any
        logging.debug("Adding {} subcomponents".format(len(self._resolved_subcomponents)))
        if len(self._resolved_subcomponents) > 0:
            components = etree.SubElement(instance, "Components")
            for s in self._resolved_subcomponents:
                line, ver, mount = s
                logging.debug("Adding {} {}".format(line, ver))
                c = etree.SubElement(components, "Component",
                                     line=line, version=ver)
                if mount is not None:
                    c.set("mountpoint", mount)
        # features if there are any
        if len(self._features) > 0:
            features = etree.SubElement(instance, "Features")
            for f in self._features:
                etree.SubElement(features, "Feature", id=f)
        # platforms if there are any
        if len(self._platforms) > 0:
            features = etree.SubElement(instance, "Platforms")
            for p in self._platforms:
                etree.SubElement(features, "Platform", id=p)
        return instance


if __name__ == "__main__":
    logging.basicConfig()
