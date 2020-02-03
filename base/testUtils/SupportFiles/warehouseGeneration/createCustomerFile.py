import md5
import getopt
import os
import sys
import datetime
import argparse
from xml.dom.minidom import Document

## https://confluence.green.sophos/display/OPM/DCI+file+-+existing+%28old%29+format
## https://confluence.green.sophos/display/OPM/DCI+file+-+new+format

class Options(object):
    pass

def createCustomerFile( args ):

    parser = argparse.ArgumentParser()

    parser.add_argument('--destination', default=None, help='destination of created customer file')
    parser.add_argument('--username', default=None, help='Username used for updating')
    parser.add_argument('--password', default=None, help='Password used for updating')
    parser.add_argument('--address', action='append', default=None, help='Url updating address Address')
    parser.add_argument('--catalogue', default=None, help='Catalogue name')
    parser.add_argument('--warehouse', action='append', default=None, help='warehouse name')
    parser.add_argument('--timestamp', default=None, help='timestamp for last assigned and last modified values')
    parser.add_argument('--redirect', default=None, help='redirection address list, used for update cache')
    parser.add_argument('--malformed-redirect', default=None, help='malformed redirect address list, used for update cache')
    parser.add_argument('--redirect-keyfile', default=None, help='redirect key file, not used?')
    parser.add_argument('--signing-keyfile', default=None, help='keyfile used for signing customer file')
    parser.add_argument('--signing-passphrase', default=None, help='passphrase used for signing customer file')
    parser.add_argument('--signing-certificate', default=None, help='certificate used for signing customer file')
    parser.add_argument('--verbose', default=False, help='Log verbose output')
    parser.add_argument('--check', default=False, help='Perform basic file checks during creation of customer file')

    subargs = args[1:len(args)]

    argoptions = parser.parse_args(subargs)

    options = Options()

    options.destination = argoptions.destination
    options.username = argoptions.username
    options.password = argoptions.password
    options.addresses = argoptions.address
    options.catalogue = argoptions.catalogue
    options.warehouses = argoptions.warehouse
    options.timestamp = argoptions.timestamp
    options.signingkey = argoptions.signing_keyfile
    options.signingpass = argoptions.signing_passphrase
    options.signingcert = argoptions.signing_certificate
    options.redirect = argoptions.redirect
    options.malformed_redirect = argoptions.malformed_redirect
    options.verbose = argoptions.verbose
    options.check = argoptions.check

    assert options.destination is not None
    assert options.username is not None
    assert options.password is not None

    filename = md5.new(options.username + ":" + options.password).hexdigest()
    dirpath = options.destination + "/" + filename[0] + "/" + filename[1:3]
    if options.signingkey is None:
        filepath = dirpath + "/" + filename + ".xml"
    else:
        filepath = dirpath + "/" + filename + ".dat"

    if options.check:
        if os.path.isfile(filepath):
            return 0
        return 1

    assert len(options.addresses) > 0

    if options.signingkey is None:
        # old style (.xml) customer file
        assert options.catalogue is not None
        assert options.signingpass is None
        assert options.signingcert is None
        assert options.warehouses is None
    else:
        # new style (.dat) customer file
        assert options.catalogue is None
        assert options.signingpass is not None
        assert options.signingcert is not None
        assert len(options.warehouses) > 0

    filename = md5.new(options.username + ":" + options.password).hexdigest()

    if options.verbose:
        print("Writing customer file %s for '%s:%s'"%(filename,options.username,options.password))

    description = "SAV Linux Regression Suite"

    if not os.path.exists(dirpath):
        os.makedirs(dirpath)

    if options.timestamp is not None:
        timestamp = options.timestamp
    else:
        # 2010-10-06T12:51:40Z
        timestamp = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")

    doc = Document()

    Warehouse = doc.createElement("Warehouse")

    if options.catalogue is not None:
        WarehouseName = doc.createElement("WarehouseName")
        WarehouseName.appendChild(doc.createTextNode(options.catalogue))
        Warehouse.appendChild(WarehouseName)

    LicenseDescription = doc.createElement("LicenseDescription")
    LicenseDescription.appendChild(doc.createTextNode(description))
    Warehouse.appendChild(LicenseDescription)

    for address in options.addresses:
        URL = doc.createElement("URL")
        URL.appendChild(doc.createTextNode(address))
        Warehouse.appendChild(URL)

    LastAssigned = doc.createElement("LastAssigned")
    LastAssigned.appendChild(doc.createTextNode(timestamp))
    Warehouse.appendChild(LastAssigned)

    LastModified = doc.createElement("LastModified")
    LastModified.appendChild(doc.createTextNode(timestamp))
    Warehouse.appendChild(LastModified)

    FileName = doc.createElement("FileName")
    FileName.appendChild(doc.createTextNode(filename))
    Warehouse.appendChild(FileName)

    if options.warehouses is not None and len(options.warehouses) > 0:
        Warehouses = doc.createElement("Warehouses")
        for warehouse in options.warehouses:
            WarehouseEntry = doc.createElement("WarehouseEntry")
            Name = doc.createElement("Name")
            if not "sdds." in warehouse:
                warehouse = "sdds." + warehouse
            Name.appendChild(doc.createTextNode(warehouse))
            WarehouseEntry.appendChild(Name)
            Warehouses.appendChild(WarehouseEntry)
        Warehouse.appendChild(Warehouses)

    if options.redirect is not None and options.redirect != "" and "=" in options.redirect:
        redirects = options.redirect.split(";")
        RedirectsNode = doc.createElement("Redirects")
        Warehouse.appendChild(RedirectsNode)
        for redirect in redirects:
            assert "=" in redirect,"%s doesn't contain ="%redirect
            source,replacement = redirect.split("=",1)
            RedirectNode = doc.createElement("Redirect")
            RedirectNode.setAttribute("Pattern",source)
            RedirectNode.setAttribute("Substitution",replacement)
            RedirectsNode.appendChild(RedirectNode)
    elif options.malformed_redirect == "MISSING-PATTERN":
        RedirectsNode = doc.createElement("Redirects")
        Warehouse.appendChild(RedirectsNode)
        RedirectNode = doc.createElement("Redirect")
        RedirectsNode.appendChild(RedirectNode)
        RedirectNode.setAttribute("Substitution","FOOBAR")
    elif options.malformed_redirect == "BAD-XML":
        RedirectsNode = doc.createElement("Redirects")
        Warehouse.appendChild(RedirectsNode)
        RedirectNode = doc.createElement("Redirect")
        RedirectsNode.appendChild(RedirectNode)
        RedirectNode.setAttribute("Substitution","REPLACETARGET")

    doc.appendChild(Warehouse)

    text = doc.toprettyxml(indent=" ",newl="\n",encoding="UTF-8")

    if options.malformed_redirect == "BAD-XML":
        text = text.replace("REPLACETARGET",'"><<<<')

    try:
        xmlFile = open(filepath, "wb")
    except IOError:
        print "XML file failed to save correctly."
        return None
    else:
        xmlFile.write(text)
        xmlFile.close()

    print "Signing key"
    print options.signingkey

    if options.signingkey is not None:
        import Signer

        signer = Signer.Signer()
        cwd = os.getcwd()
        signer.m_options.key = options.signingkey
        signer.password = options.signingpass
        certs = open( os.path.join(cwd,options.signingcert)).read()

        ## Get signature
        signature = signer.encodedSignatureForFile(filepath)

        try:
            d = open(filepath,"a")
        except IOError:
            print "XML file failed to save correctly."
            return None
        else:
            d.write(signature)

            ## And put the key certificates in the manifest as well
            d.write(certs)

            d.close()

            print "Wrote signed customer file to %s"%filepath

    return 0


if __name__ == "__main__":
    sys.exit(createCustomerFile( sys.argv ))
