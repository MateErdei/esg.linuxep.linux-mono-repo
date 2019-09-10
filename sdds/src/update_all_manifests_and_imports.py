from xmlrpclib import ServerProxy
import os.path
import socket
from StringIO import StringIO
import hashlib
from xml.etree import ElementTree as ET
import csv
import os
import json
import sys


csv.register_dialect('manifest', delimiter=' ')

server = ServerProxy("http://buildsign-m:8000", allow_none=True)


def path_of_node(package, node):
    if "Offset" in node.attrib:
        return os.path.join(package, node.attrib["Offset"],
                            node.attrib["Name"])
    else:
        return os.path.join(package, node.attrib["Name"])


def update_ca(sdds, package):
    sav_marker = [f for f in sdds.findall(".//Role")
                  if f.attrib["Name"] == "SAV"]
    sau_marker = [f for f in sdds.findall(".//Role")
                  if f.attrib["Name"] == "SAU"]

    try:
        product_version = [int(x) for x in sdds.find(".//Version").text.split(".")]
    except ValueError:
        product_version = []
    empty_vdl = False
    if len(sav_marker) > 0 and not empty_vdl:
        # Note if we want this to work with dev and production it will not work
        print "Skipping certificate update on SAVXP, because its for VDL"
        return
    root_cas = [f for f in sdds.findall(".//File")
                if f.attrib["Name"] == "ps_rootca.crt"]
    root_crls = [f for f in sdds.findall(".//File")
                 if f.attrib["Name"] == "ps.crl"]
    for root_ca in root_cas:
        file_path = path_of_node(package, root_ca)
        with open(file_path, "wb") as f:
            f.write(server.get_cert("rootca", None))
        update_file_checksum_and_size(root_ca, file_path)
    for root_crl in root_crls:
        file_path = path_of_node(package, root_crl)
        with open(file_path, "wb") as f:
            crldata = ""
            if sau_marker and product_version < [4, 4]:
                print "WARNING! Forcing a NULL root crl for SAU%s because its CRL"\
                      " handling is broken!" % product_version
            else:
                crldata = server.get_cert("rootcrl", None)

            f.write(crldata)
        update_file_checksum_and_size(root_crl, file_path)


def update_file_checksum_and_size(node, file_path):
    with open(file_path, "rb") as f:
        md5 = hashlib.md5(f.read()).hexdigest()
        size = str(os.stat(file_path).st_size)
        if node.attrib["MD5"] != md5 or node.attrib["Size"] != size:
            print "Updating %s MD5 %s->%s size %s->%s" % (file_path,
                                                          node.attrib["MD5"],
                                                          md5,
                                                          node.attrib["Size"],
                                                          size)
        node.attrib["MD5"] = md5
        node.attrib["Size"] = size


def update_manifest_dat_node(sdds, manifest):
    if not os.path.isfile(manifest):
        return
    manifestnode = [f for f in sdds.findall(".//File")
                    if f.attrib["Name"] == "manifest.dat"][0]
    update_file_checksum_and_size(manifestnode, manifest)


def get_file_size_and_checksum(rootpath, filepath, checksum_type):
    path = os.path.join(rootpath, filepath)
    with open(path, "rb") as f:
        if checksum_type == "sha1":
            checksum = hashlib.sha1(f.read()).hexdigest()
        elif checksum_type == "sha256":
            checksum = hashlib.sha256(f.read()).hexdigest()
        elif checksum_type == "sha384":
            checksum = hashlib.sha384(f.read()).hexdigest()
        else:
            checksum = None
        size = str(os.stat(path).st_size)
        return size, checksum


def update_manifest_dat_file(manifest, rootpath):
    if not os.path.isfile(manifest):
        return
    print "Updating manifest", manifest
    manifestdata = StringIO()
    with open(manifest, "rb") as m:
        for line in m.readlines():
            if line == "-----BEGIN SIGNATURE-----\n":
                break
            parts = line.split()
            if parts[0] == "#sig":
                break
            if parts[0] == "#sha384":
                checksum_type = "sha384"
                # Filename and size are retained from the previous line
                checksum = parts[1]
            elif parts[0] == "#sha256":
                checksum_type = "sha256"
                # Filename and size are retained from the previous line
                checksum = parts[1]
            else:
                checksum_type = "sha1"
                sep = list(csv.reader([line], dialect="manifest"))[0]
                filename, size, checksum = sep[0], sep[1], sep[2]

            new_size, new_checksum = get_file_size_and_checksum(rootpath, filename, checksum_type)
            if new_size != size or new_checksum != checksum:
                print "Updating %s  %s->%s %s->%s" % (checksum_type, size, new_size,
                                                      checksum, new_checksum)
            if checksum_type == "sha1":
                manifestdata.writelines(
                    ['"%s" %s %s\n' % (filename, new_size, new_checksum)])
            elif checksum_type == "sha256":
                manifestdata.writelines(
                    ['#sha256 %s\n' % new_checksum])
                # Invalidate for next sha1:sha256 row pair
                filename, size, checksum = None, None, None
            elif checksum_type == "sha384":
                manifestdata.writelines(
                    ['#sha384 %s\n' % new_checksum])
                # Invalidate for next sha1:sha384 row pair
                filename, size, checksum = None, None, None

    manifestdata.pos = 0
    file_data = manifestdata.read()

    with open(manifest, "wb") as m_file:

        m_file.write(file_data)

        try:
            m_file.write(server.sign_file(file_data, manifest, None))
        except socket.error, e:
            print "ERROR - Signing Failed: %s" % e
            raise
        for cert in ("pub", "ca"):
            try:
                m_file.write(server.get_cert(cert, None))
            except socket.error, e:
                print "ERROR - Failed to get cert %s: %s" % (cert, e)
                raise


def get_full_subdirs(directory):
    return [fd for fd in [os.path.join(directory, d)
                          for d in os.listdir(directory)]
            if os.path.isdir(fd)]


def update_component_suite_version(sdds, path_to_match,
                                   component_suite_lable_updates):
    if path_to_match in component_suite_lable_updates:
        item = component_suite_lable_updates[path_to_match]
        print "Updating component suite version", path_to_match, "to", item
        sdds.find(".//Version").text = item["version"]
        sdds.find(".//ShortDesc").text = item["description"]
        sdds.find(".//LongDesc").text = item["description"]
        sdds.find(".//Token").text = "%s#%s" % \
            (sdds.find(".//RigidName").text, item["version"])


def main(root_path):
    with open("component_suite_version_labels.json") as csl:
        component_suite_lable_updates = json.load(csl)
    collatedirs = get_full_subdirs(os.path.join(root_path, "inputs"))
    for collatedir in collatedirs:
        sdds_ready_dir = os.path.join(root_path, collatedir, "SDDS-Ready-Packages")
        print "Searching", sdds_ready_dir
        if os.path.isdir(sdds_ready_dir):
            inputs = get_full_subdirs(sdds_ready_dir)
            for package in inputs:
                print "Searching", package
                manifest = os.path.join(root_path, package, "manifest.dat")
                sddsimport = os.path.join(root_path, package, "SDDS-Import.xml")

                sdds = ET.parse(sddsimport)

                for f in sdds.findall(".//Role"):
                    if f.attrib["Name"] != "SEC":
                        update_ca(sdds, package)

                        update_manifest_dat_file(manifest, package)

                        update_manifest_dat_node(sdds, manifest)

                        update_component_suite_version(
                            sdds, package, component_suite_lable_updates)

                        sdds.write(sddsimport)

if __name__ == '__main__':
    main(sys.argv[1])
