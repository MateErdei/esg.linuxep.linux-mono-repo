import capnp
from builtins import print
import sys
sys.path.append('/home/ed/g/sspl-tools/sspl-plugin-anti-virus/redist/capnproto/include/')
sys.path.append('/home/ed/g/sspl-tools/sspl-plugin-anti-virus/tapvenv/lib/python3.7/site-packages/')
sys.path.append('/home/ed/g/sspl-tools/sspl-plugin-anti-virus/modules/scan_messages/')
print(sys.path)

import NamedScan_capnp
NamedScan_capnp = capnp.load('/home/ed/g/sspl-tools/sspl-plugin-anti-virus/modules/scan_messages/NamedScan.capnp',
                              imports=['/home/ed/g/sspl-tools/sspl-plugin-anti-virus/tapvenv/lib/python3.7/site-packages','/tmp'])

scan = NamedScan_capnp.NamedScan.new_message(name="named_scan")
print(scan)


address_schema = capnp.load('addressbook.capnp')

print(address_schema.qux)

address_book = address_schema.AddressBook.new_message()

print(address_book)
