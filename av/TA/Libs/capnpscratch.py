import capnp
from builtins import print
import sys
sys.path.append('/home/hamza/git/sspl-tools/sspl-plugin-anti-virus/redist/capnproto/include/capnp')
sys.path.append('/home/hamza/git/sspl-tools/sspl-plugin-anti-virus/tapvenv/lib/python3.7/site-packages/capnp')
sys.path.append('/home/hamza/git/sspl-tools/sspl-plugin-anti-virus/modules/scan_messages/')
print(sys.path)

#import NamedScan_capnp
# named_scan_capnp = capnp.load('/home/hamza/git/sspl-tools/sspl-plugin-anti-virus/modules/scan_messages/NamedScan.capnp',
#                               imports=['/home/hamza/git/sspl-tools/sspl-plugin-anti-virus/tapvenv/lib/python3.7/site-packages','/tmp'])

scan = NamedScan_capnp.NamedScan.new_message(name="named_scan")
print(scan)


address_schema = capnp.load('addressbook.capnp')

print(address_schema.qux)

address_book = address_schema.AddressBook.new_message()

print(address_book)
