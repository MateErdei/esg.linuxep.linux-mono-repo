import os
import xml.etree.ElementTree as etree
import re


class ImportFile:

    def __init__(self, import_file):
        self._file = import_file
        self._tree = etree.ElementTree(None, import_file)

    def fixup_version(self, version_string):
        version_element = self._tree.find("Component/Version")
        version_element.text = version_string
        eml_element = self._tree.find("Component/EMLVersion")
        if eml_element is not None:
            eml_element.text = version_string
        token_element = self._tree.find("Dictionary/Name/Label/Token")
        match = re.match("(.+)#(.+)", token_element.text)
        token_element.text = "%s#%s" % (match.group(1), version_string)
        short_element = self._tree.find(
            "Dictionary/Name/Label/Language/ShortDesc")
        short_element.text = version_string
        long_element = self._tree.find(
            "Dictionary/Name/Label/Language/LongDesc")
        match = re.match("(.+) v\d+.\d+.\d+", long_element.text)
        long_element.text = "%s v%s" % (match.group(1), version_string)

    def remove_eps_role(self):
        role_element = self._tree.find("Component/Roles")
        roles_elements = self._tree.findall("Component/Roles/Role")
        for r in roles_elements:
            if r.get("Name") == "EPS":
                role_element.remove(r)

    def write(self):
        self._tree.write(self._file, "utf-8")

    def get_name(self):
        name_element = self._tree.find("Component/Name")
        return name_element.text

    def get_version(self):
        version_element = self._tree.find("Component/Version")
        return version_element.text

    def file_list(self):
        list = self._tree.find("Component/FileList")
        return FileList(list)


class FileItem:

    def __init__(self, file_element):
        self._element = file_element

    def get_name(self):
        return self._element.get("Name")

    def get_element(self):
        return self._element


class FileList:

    def __init__(self, element):
        self._file_list_element = element

    def __iter__(self):
        return (FileItem(f) for f in self._file_list_element.findall("File"))

    def remove(self, f):
        self._file_list_element.remove(f.get_element())
