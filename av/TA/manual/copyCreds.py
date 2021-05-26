#!/bin/env python3
# Copy SDDS credentials from external to internal
# Copies the warehouses in an external DCI into an internal one.

# Allows upgrade/downgrade testing using internal creds, without having to write complicated URLs

import hashlib
import sys
import urllib.request
import xml.dom.minidom

def is_md5(s):
    if len(s) != 32:
        return False
    return True

def h(external_creds):
    # pass md5 directly back
    if is_md5(external_creds):
        return external_creds
    elif ":" not in external_creds:
        external_creds += ":54m5ung"
    m = hashlib.md5(external_creds.encode("UTF-8"))
    return m.hexdigest()

def remove_blanks(node):
    for x in node.childNodes:
        if x.nodeType == xml.dom.Node.TEXT_NODE:
            if x.nodeValue:
                x.nodeValue = x.nodeValue.strip()
        elif x.nodeType == xml.dom.Node.ELEMENT_NODE:
            remove_blanks(x)

def getXmlText(node):
    """Get the text out of an XML node.
    """
    text = ""
    for n in node.childNodes:
        text += n.data
    return text

def main(argv):
    external_creds = argv[1].strip()
    hash = h(external_creds)
    url = "https://dci.sophosupd.com/update/"+hash[0]+"/"+hash[1:3]+"/"+hash+".dat"
    print(url)
    contents = urllib.request.urlopen(url).read().decode("UTF-8")
    contents = contents.split("-----BEGIN SIGNATURE-----", 1)[0]
    dom = xml.dom.minidom.parseString(contents)
    remove_blanks(dom)
    warehouses = dom.getElementsByTagName("Warehouses")[0]
    names = warehouses.getElementsByTagName("Name")
    text = [ getXmlText(n) for n in names ]
    print(text)

    warehouses = [ "fw="+n for n in text ]


    # http://dcigen.green.sophos/create/dcigen.cgi?u=JKWtest1&p=password&w=sdds.ufw_epa_s_WIN2014-2.1&fw=sdds.fw_sde_561&fw=sdds.sepa_s_retired&fw=sdds.fw_osx_epa_s_MAC2014-1&fw=sdds.fw_unix_s_LIN2014-2&fw=sdds.fw_linux_s_LIN2014-2&fw=sdds.fw_classic_unix_s_LIN2014-2&fw=sdds.fw_vshield_1110_VS2014-3&fw=sdds.epa_s_WIN2015-4.2&fw=sdds.fw_sum_s_epa_155_SUM2015-1.1&m=doit
    username = argv[2].strip()
    password = argv[3].strip()
    url = "http://dcigen.green.sophos/create/dcigen.cgi?u=" + username + "&p=" + password + "&" + \
          "&".join(warehouses) + \
          "&m=doit"

    contents = urllib.request.urlopen(url).read().decode("UTF-8")
    print(contents)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

