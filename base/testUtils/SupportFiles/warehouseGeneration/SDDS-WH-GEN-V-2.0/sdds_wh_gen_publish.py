import os
import hashlib
import shutil
import time
import stat

import sdds_wh_gen_xml_funcs as xml_funcs
from sdds_wh_gen_sign import Sign, sdds_platform

# Main control class for publishing warehouses

class DeliberateErrors(object):
    def __init__(self, deliberate_errors, logger):
        self.logger = logger
        self.m_deliberate_errors = deliberate_errors

    def getSHA384_errors(self, key, sha384):
        value = self.m_deliberate_errors.get(key,None)
        if value is None:
            return sha384

        if value == '"':
            value = None

        ## If we want permutations then introduce special values
        self.logger.statusMessage(" Replacing sha384 %s with %s"%(sha384,str(value)))

        return value

    def add_SHA384_errors(self, xmlobj, key):
        """
        Add SHA-384 errors to a subclass of XMLPrototype
        """
        xmlobj.setSHA384(self.getSHA384_errors(key,xmlobj.getSHA384()))

GL_DELIBERATE_ERRORS = None

class Publish(object):

    def __init__( self, warehouse_description, logger ):
        self.logger = logger
        self.m_warehouse_description = warehouse_description
        self.wh_path = warehouse_description.wh_path ## path to publish warehouse to
        self.wh_TLC = warehouse_description.wh_TLC ## Top level catalogue name
        self.dictionary_set = warehouse_description.dictionary_set
        self.li_product_instances = warehouse_description.li_product_instances
        self.li_child_objects = warehouse_description.li_child_objects
        self.error_state = 0
        self.m_deliberate_errors = DeliberateErrors(warehouse_description.m_deliberate_errors,logger)
        global GL_DELIBERATE_ERRORS
        GL_DELIBERATE_ERRORS = self.m_deliberate_errors

    def getSHA384_errors(self, key, sha384):
        return self.m_deliberate_errors.getSHA384_errors(key,sha384)

    def beginPublishing( self ):
        self.logger.statusMessage( "Beginning publish of warehouse: '%s' at location: '%s'" % ( self.wh_TLC, self.wh_path ) )

        # Create directory structure for warehouse

        warehouse_dirs = [self.wh_path,
                          os.path.join( self.wh_path, "catalogue" ),
                          os.path.join( self.wh_path, "history" )]

        for i in range( 0, 3 ):
            warehouse_dir = warehouse_dirs[i]
            if not os.path.exists(warehouse_dir):
                self.logger.statusMessage( "Directory: '%s' does not exist - creating" % (warehouse_dir) )
                try:
                    os.makedirs(warehouse_dir)
                except OSError, Error:
                    self.logger.errorMessage( "Failed to create directory", Error )
            else:
                self.logger.statusMessage( "Directory: '%s' already exists" % (warehouse_dir) )

        # Create a dictionary GUID-[list_of_product_objects] for the purpose of creating the rigid name catalogue

        dic_rigid_object = {}

        # First publish all child products as we need their metadata MD5s

        for child_object in self.li_child_objects:
            self.logger.statusMessage( "Publishing package: '%s'" % ( child_object.en_long_desc ) )

            # Copy files across to warehouse location and verify
            assert os.path.exists(child_object.SDDS_import_path)
            child_object.file_data = self.copyFiles( child_object.SDDS_import_path, child_object.file_data )

            package_instance_conts_ob = PackageInstanceContents( child_object, self.wh_path, self.logger )

            # Assume in this version of the SDDS warehouse gen that atomic packages won't have sub-packages, therefore save
            pack_object = package_instance_conts_ob.package_object
            pack_object.save()

            rigid_name = child_object.rigid_name

            # Save the 'badger:package' MD5 to component package object

            child_object.package_meta_MD5 = pack_object.getMD5()
            child_object.packageMetaSha384(
                self.getSHA384_errors(
                    rigid_name+"_CONTENTS",
                    pack_object.getSHA384()
                    )
                    )
            child_object.packageMetaSize(pack_object.getSize())

            # Create the attributes metadata and save MD5 to package data object

            self.logger.statusMessage( "Creating 'badger:attributes' XML document at: '%s'" % ( self.wh_path ) )
            attributes_ob = MetaAttributes(  child_object, self.wh_path, self.logger , subcomponent=True)
            child_object.attributes_meta_MD5 = attributes_ob.getMD5()
            child_object.attributesMetaSha384(
                self.getSHA384_errors(rigid_name+"_ATTRIBUTES",
                attributes_ob.getSHA384()
                )
                )
            child_object.attributesMetaSize(attributes_ob.getSize())

            dic_rigid_object = self.addRigidobjectToDictionary( dic_rigid_object, child_object  )

        # Then publish all product instances


        for product_instance_object in self.li_product_instances:

            self.logger.statusMessage( "Publishing package for product instance: '%s'" % ( product_instance_object.parent_object.en_long_desc ) )

            # Copy files across to warehouse location and verify
            assert os.path.exists(product_instance_object.component_data.component_import_path)
            product_instance_object.parent_object.file_data = self.copyFiles( product_instance_object.component_data.component_import_path,
                                                                              product_instance_object.parent_object.file_data )



            package_instance_conts_ob = PackageInstanceContents( product_instance_object.parent_object, self.wh_path, self.logger )

            # Cycle through list of child package IDs and find corresponding package objects so their metadata MD5s can be utilised

            self.logger.statusMessage( "Matching already published packages against product instance dependency" )
            for child_object_ID in product_instance_object.child_object_IDs:
                matched_child_object = self.matchPackageObjectToID( child_object_ID )
                if matched_child_object:
                    decode_path = matched_child_object.decode_path
                    if decode_path is None:
                        decode_path = matched_child_object.default_home_folder
                        self.logger.errorMessage("Using subpackage default home folder: %s"%decode_path)

                    self.logger.statusMessage("Adding package: '%s' to subpackage list decode_path=%s" % ( matched_child_object.en_long_desc,decode_path ))
                    matched_child_object.packageMetaSha384(self.getSHA384_errors(decode_path+"_SUBPACKAGE", matched_child_object.getSHA384()))
                    package_instance_conts_ob.package_object.addSubpackageData(decode_path, matched_child_object)

                else:
                    self.logger.errorMessage( "Could not find already published package against product instance's ID: '%s'" % ( child_object_ID ) )
                    self.error_state = 1

            # Save the package metadata file now sub-package dependencies have been attached

            package_instance_conts_ob.package_object.save()

            # Save the 'badger:package' MD5 to component package object

            product_instance_object.parent_object.package_meta_MD5 = package_instance_conts_ob.package_object.getMD5()
            product_instance_object.parent_object.packageMetaSha384(
                self.getSHA384_errors(
                    product_instance_object.parent_object.rigid_name+"_CONTENTS", ## Possibly need to differentiate more on this one?
                    package_instance_conts_ob.package_object.getSHA384() ## Level 5/6 file list going into Level 4 Rigid name catalogue
                    )
                )

            product_instance_object.parent_object.packageMetaSize(package_instance_conts_ob.package_object.getSize())

            # Create the attributes metadata and save MD5 to package data object

            self.logger.statusMessage( "Creating 'badger:attributes' XML document at: '%s'" % ( self.wh_path ) )
            attributes_ob = MetaAttributes(  product_instance_object.parent_object, self.wh_path, self.logger, subcomponent=False )
            product_instance_object.parent_object.attributes_meta_MD5 = attributes_ob.getMD5()
            product_instance_object.parent_object.attributesMetaSha384(
                self.getSHA384_errors(
                    product_instance_object.parent_object.rigid_name+"_ATTRIBUTES", ## Level 5/6 attributes going into Level 4 Rigid name catalogue
                    attributes_ob.getSHA384() ## Possibly need to differentiate more on this one?
                    )
                )
            product_instance_object.parent_object.attributesMetaSize(attributes_ob.getSize())

            assert(product_instance_object.parent_object.attributesMetaSha384() is not None)

            dic_rigid_object = self.addRigidobjectToDictionary( dic_rigid_object, product_instance_object.parent_object  )

        # Cycle through dictionary of rigid objects and prompt them to create a file

        for k, v in dic_rigid_object.iteritems():
            v.save( k )
            ## Replace the sha384 for Rigid objects
            self.add_SHA384_errors(v,k) ## Level 4 MetaDataProductLineRigid going to Level 3 Ultimate catalogue

        # Create a full list of product lines for the dictionary

        li_package_instances = []
        for product_instance_object in self.li_product_instances:
            li_package_instances.append( product_instance_object.parent_object )
        for child_object in self.li_child_objects:
            li_package_instances.append( child_object )

        # Create the dictionary file

        dictionary_ob = MetaDictionary( self.wh_path, self.dictionary_set, li_package_instances, self.logger )

        # Create the dictionary catalogue file

        dictionary_cata_ob = MetaDictionaryCatalogue( self.wh_path, dictionary_ob, self.logger )

        # Create the product lines catalogue

        self.logger.statusMessage( "Creating 'badger:ultimate' XML document at: '%s'" % ( self.wh_path ) )
        ultimate_ob = MetaDataUltimate( self.wh_path, dic_rigid_object, dictionary_cata_ob, self.logger )
        self.add_SHA384_errors(ultimate_ob,"ULTIMATE_XML") ## Level 3 Ultimate item going into Level 2 TLC

        # create the signature data for the warehouse
        signature, public_cert, inter_cert = self.signFile( ultimate_ob.getPath(), self.logger )

        # Create the signature file

        meta_ob =  MetaSignature( self.wh_path, signature, public_cert, inter_cert, self.logger )
        self.add_SHA384_errors(meta_ob,"ULTIMATE_XML_SIGNATURE") ## Level 3 Ultimate item going into Level 2 TLC

        # Create the top level catalogue

        self.logger.statusMessage( "Creating 'badger:catalogue' XML document at: '%s'" % ( self.wh_path ) )
        catalog_ob = MetaCatalogue( self.wh_path, self.wh_TLC, ultimate_ob, meta_ob, self.logger )

    def add_SHA384_errors(self, xmlobj, key):
        xmlobj.setSHA384(self.getSHA384_errors(key,xmlobj.getSHA384()))

    def addRigidobjectToDictionary( self, dic_rigid_object, package_object ):
        # Check if GUID has been entered in to list already, which depends on how data is added to dictionary

        if package_object.rigid_name not in dic_rigid_object:
            self.logger.statusMessage( "Creating 'badger:rigid' XML document for: '%s' at: '%s'" % ( package_object.rigid_name, self.wh_path ) )

            # Create GUID directory within warehouse stucture

            GUID_dir = os.path.join( self.wh_path, package_object.rigid_name )
            if not os.path.exists( GUID_dir ):
                self.logger.statusMessage( "Directory: '%s' does not exist - creating" % ( GUID_dir ) )
                try:
                    os.makedirs( GUID_dir )
                except OSError, Error:
                    self.logger.errorMessage( "Failed to create directory", Error )
            else:
                self.logger.statusMessage( "Directory: '%s' already exists" % ( GUID_dir ) )

            # Create a rigid name metadata object

            rigid_object = MetaDataProductLineRigid( self.wh_path, self.logger )

            # Create new dictionary key and attached list of rigid objects

            dic_rigid_object[package_object.rigid_name] = rigid_object

        self.logger.statusMessage( "Adding version: '%s' to 'badger:rigid' XML document for: '%s'" % ( package_object.version,
                                                                                                       package_object.rigid_name ) )
        dic_rigid_object[package_object.rigid_name].addProductInstance( package_object )

        return dic_rigid_object

    def matchPackageObjectToID( self, ID_to_find ):
        for child_object in self.li_child_objects:
            if child_object.object_ID == ID_to_find:
                self.logger.statusMessage( "Matched object ID: '%s' to package: '%s'" % ( ID_to_find, child_object.en_long_desc ) )
                return child_object

        return None

    def signFile( self, file_name, logger ):
        self.logger.statusMessage( "Signing: '%s' with: '%s'" % ( file_name, sdds_platform.MANIFEST_DAT ) )
        self.logger.statusMessage( "Using private key: '%s'" % ( sdds_platform.PRIVATEKEY_PEM ) )
        self.logger.statusMessage( "Using public certificate: '%s'" % ( sdds_platform.PUBLICCERT_CRT ) )
        self.logger.statusMessage( "Using intermediate certificate: '%s'" % ( sdds_platform.INTERCERT_CRT ) )
        self.logger.statusMessage( "Using OpenSSL at location: '%s'" % ( sdds_platform.OPENSSL ) )
        self.logger.statusMessage( "Using manifest.dat at location: '%s'" % ( sdds_platform.MANIFEST_DAT ) )

        final_sig, pub_cert, inter_cert = Sign( logger ).signManifest( file_name,
                                                                       sdds_platform.PRIVATEKEY_PEM,
                                                                       sdds_platform.PUBLICCERT_CRT,
                                                                       sdds_platform.MANIFEST_DAT,
                                                                       sdds_platform.OPENSSL,
                                                                       sdds_platform.INTERCERT_CRT )

        final_pub_cert = pub_cert.replace( sdds_platform.MANIFEST_DAT, '' )
        final_inter_cert = inter_cert.replace( sdds_platform.MANIFEST_DAT, '' )

        return final_sig, str( final_pub_cert ).strip(), str( final_inter_cert ).strip()

    def copyFiles( self, import_path, li_file_data ):


        #self.logger.errorMessage( li_file_data)

        assert self.error_state == 0, "Errors before getting files for warehouse"
        # First get the directory the from filename path

        package_path = os.path.split(import_path)[0]
        assert os.path.exists(package_path), "%s doesn't exist - can't possibly import from it" % package_path

        # Cycle through list of file data from SDDS-import file
        for file_data in li_file_data:

            current_file_path = None

            # Construct the path
            # self.logger.errorMessage("----------")
            # self.logger.errorMessage("------package path = " + package_path)
            # self.logger.errorMessage("------offset = " + file_data.offset)
            # self.logger.errorMessage("------name = " + file_data.name)



            if file_data.offset:
                current_file_path = os.path.join( package_path, file_data.offset, file_data.name )
            else:
                current_file_path = os.path.join( package_path, file_data.name )

            # self.logger.errorMessage("---------full path = " + current_file_path)

            # If the file exists in the location on disk
            if os.path.exists( current_file_path ):
                # Check if the size of the file on disk matches the size specified in the import
                file_MD5 = None
                try:
                    file = open( current_file_path, "rb" )
                except IOError, Error:
                    self.logger.errorMessage( "Failed to open package source file", Error )
                    self.error_state = 1
                else:
                    ## AIX can't handle md5 all in one go
                    md5calc = hashlib.md5()
                    sha384calc = hashlib.sha384()
                    while True:
                        data = file.read(10240)
                        if len(data) == 0:
                            break
                        md5calc.update(data)
                        sha384calc.update(data)
                    file.close()
                    file_MD5 = md5calc.hexdigest()
                    file_SHA384 = sha384calc.hexdigest()

                # Check if the checksum of the file contents matches that specified in the import
                if file_MD5 != file_data.MD5:
                    self.logger.errorMessage( "Checksum of file: '%s' differs to that in SDDS-import file" % ( current_file_path ) )
                    self.error_state = 1
                else:
                    file_data.setSHA384(self.getSHA384_errors(file_MD5,file_SHA384))



                # Check if the size of the file matches
                file_size = None
                try:
                    file_size = os.stat( current_file_path )[stat.ST_SIZE]
                except IOError, Error:
                    self.logger.errorMessage( "Failed to get size of file", Error )
                    self.error_state = 1
                print "--------filesize",file_size
                print "--------filedata",file_data
                print "--------filedata size",file_data.size
                if int( file_size ) != int( file_data.size ):
                    self.logger.errorMessage( "Size of file: '%s' differs to that in SDDS-import file" % ( current_file_path ) )
                    self.error_state = 1

                # Copy file to warehouse location using MD5 as new filename
                if self.error_state == 0:
                    target_path = os.path.join( self.wh_path, file_data.MD5 + "x000.dat")
                    self.logger.statusMessage( "Copying source file: '%s' to target: '%s' " % ( current_file_path, target_path  ) )

                    try:
                        #self.logger.errorMessage("------copy " + current_file_path + " to " + target_path)
                        shutil.copyfile( current_file_path, target_path )
                    except IOError, Error:
                        self.logger.errorMessage( "Failed to copy file", Error )
                        self.error_state = 1

                # Get the last modified
                if self.error_state == 0:
                    try:
                        mod_date = os.stat( current_file_path )[stat.ST_MTIME]
                    except IOError, (errno, strerror):
                        self.logger.errorMessage( "Failed to get last modified attribute of file", Error )
                        self.error_state = 1
                    else:
                        file_data.last_modified = time.strftime( "%Y-%m-%dT%H:%M:%S", time.gmtime( mod_date ) )

            else:
                self.logger.errorMessage( package_path)
                self.logger.errorMessage(file_data.name,file_data.offset)

                self.logger.errorMessage( "File: '%s' doesn't exist in source" % ( current_file_path ) )
                ## We can't procceed without the file, as otherwise file_data.last_modified will be None,
                ## which would put a None into the XML (now asserts earlier), which would break XML generation
                self.error_state = 1

        assert self.error_state == 0,"Errors during getting files for warehouse"
        return li_file_data

# Class to create a package instance

class PackageInstanceContents(object):
    BADGER_PACKAGE_CONTENTS_COUNT = 0

    def __init__( self, package_object, wh_path, logger ):
        # Create package contents XML data for a  product instance

        logger.statusMessage( "Creating 'badger:contents' XML document for: '%s' at: '%s'" % ( package_object.dictionary_label_token, wh_path ) )
        self.contents_object = MetaDataContents( package_object, wh_path, logger )

        # Create package definition XML data for a product instance
        key = "badger_package_contents_%d"%PackageInstanceContents.BADGER_PACKAGE_CONTENTS_COUNT
        PackageInstanceContents.BADGER_PACKAGE_CONTENTS_COUNT += 1
        global GL_DELIBERATE_ERRORS
        GL_DELIBERATE_ERRORS.add_SHA384_errors(self.contents_object,key)

        logger.statusMessage( "Creating 'badger:package' XML document for: '%s' at: '%s'" % ( package_object.dictionary_label_token, wh_path ) )
        self.package_object = MetaDataPackage( package_object, wh_path, self.contents_object, logger )

# Class to create package contents meta elements, derived from 'XMLPrototype'

class XMLPrototype(object):

    def __init__( self, logger ):
        self.logger = logger
        self.XML_object = xml_funcs.XmlProcessing()
        self.MD5 = None
        self.__m_sha384 = None
        self.__m_size = None

    def saveXMLDocument(self, wh_path, dir = ""):
        XML_path = os.path.join( wh_path, "%s.xml" % dir )

        self.logger.statusMessage( "Creating XML meta data file: '%s'" % XML_path )

        self.XML_object.svXMLDoc( XML_path )

    def saveXMLDocumentAsMD5(self, wh_path, dir = ""):
        temp_XML_path = os.path.join( wh_path, "temp.xml" )

        self.XML_object.svXMLDoc( temp_XML_path )

        if not os.path.exists( temp_XML_path ):
            self.logger.errorMessage( "Failed to create temporary XML file")
            raise Exception("Failed to create temporary file %s"%temp_XML_path)

        try:
            temp_XML_file = open( temp_XML_path, "rb")
        except IOError, Error:
            self.logger.errorMessage( "Failed to open temporary XML file", Error )
            raise
        else:
            data = temp_XML_file.read()
            self.MD5 = hashlib.md5(data).hexdigest()
            self.__m_sha384 = hashlib.sha384(data).hexdigest()
            self.__m_size = str(len(data))
            temp_XML_file.close()
            os.unlink(temp_XML_path)

        MD5_XML_path = os.path.join( wh_path, dir, self.MD5 + "x000.xml" )

        self.logger.statusMessage( "Creating XML meta data file: '%s'" % MD5_XML_path )
        open(MD5_XML_path,"wb").write(data)

    def getMD5(self):
        return self.MD5

    def getSHA384(self):
        return self.__m_sha384

    def setSHA384(self, v):
        """
        Allow error values for XML files
        """
        self.__m_sha384 = v

    def getSize(self):
        return self.__m_size

# Class to create content metadata XML files

class MetaDataContents( XMLPrototype ):

    def __init__( self, package_object, wh_path, logger ):
        XMLPrototype.__init__( self, logger )

        li_used_paths = []

        self.XML_object.crParElem( "contents", "xmlns", "badger:contents" )
        self.XML_object.crChldElem( "parent", "directories", "directories" )

        self.XML_object.crChldElem( "directories", "top_level", "directory")

        for file_data in package_object.file_data:

            file_components = []

            if not file_data.offset:
                self.addDir( file_data, "top_level" )
            else :
                file_components = os.path.normpath( file_data.offset ).split( os.sep )

            if file_components:
                if file_data.offset not in li_used_paths:

                    last_node = "top_level"
                    current_node = ""

                    for file_component in file_components:
                        current_node = os.path.join( current_node, file_component )
                        current_node = current_node.replace( "\\", "/" )

                        if current_node not in li_used_paths:
                            assert file_component is not None
                            self.XML_object.crChldElem(last_node, current_node, "directory", ["path"], [file_component] )

                        last_node = current_node
                        li_used_paths.append( current_node )

                self.addDir( file_data, file_data.offset )

        self.saveXMLDocumentAsMD5( wh_path )

    def addDir( self, file_data, parent_node ):
        name = file_data.name
        assert name is not None
        size = file_data.size
        assert size is not None
        date = file_data.last_modified
        assert date is not None
        MD5 = file_data.MD5

        attributes = {
            "name":name,
            "size":size,
            "date":date,
        }
        parent_node=parent_node.strip('/')
        self.XML_object.crChldElem( parent_node, "file", "file", attributes )

        attributes = {
            "extent":"x000",
            "sha384":file_data.getSHA384(),
            "size":size,
        }

        self.XML_object.crChldElem( "file", "md5", "md5", attributes )
        self.XML_object.chlTxtNode( "md5", MD5 )

# Class to create package metadata XML files

class MetaDataPackage( XMLPrototype ):

    def __init__( self, package_object, wh_path, package_content_meta, logger ):
        XMLPrototype.__init__( self, logger )

        self.wh_path = wh_path

        product_name = package_object.en_long_desc
        product_name = product_name.replace( " v" + package_object.en_short_desc, "" )

        self.XML_object.crParElem( "package", "name", str.join( '', (
                                  "_pkg_", product_name,
                                  " ", package_object.version ) ), "xmlns","badger:package" )

        # Package reference
        self.XML_object.crChldElem( "parent", "node1", "contents" )
        self.XML_object.crChldElem( "node1", "node2", "md5", {
            "extent":"x000",
            "sha384":package_content_meta.getSHA384(),
            "size":package_content_meta.getSize(),
                } )
        self.XML_object.chlTxtNode( "node2", package_content_meta.getMD5() )

        self.XML_object.crChldElem( "parent", "node4", "subpackages" )

    def addSubpackageData( self, package_content_meta_path, package_content_meta ):
        package_content_meta_MD5 = package_content_meta.package_meta_MD5
        # Sub-package reference
        self.XML_object.crChldElem( "node4", "node5", "subpackage", ["path"], [package_content_meta_path] )
        self.XML_object.crChldElem( "node5", "node6", "md5", {
            "extent":"x000",
            "sha384":package_content_meta.getSHA384(),
            "size":package_content_meta.getSize(),
                } )
        self.XML_object.chlTxtNode( "node6", package_content_meta_MD5 )

    def save(self):
        self.saveXMLDocumentAsMD5( self.wh_path )

# Class to create product line metadata

class MetaDataProductLineRigid( XMLPrototype ):

    def __init__( self, wh_path, logger ):
        XMLPrototype.__init__( self, logger )

        self.wh_path = wh_path

        # Create the initial part of product line catalogue
        self.XML_object.crParElem( "rigidName", "xmlns", "badger:rigidName" )

        self.XML_object.crChldElem( "parent", "node1", "versions" )

    def addProductInstance( self, package_object ):
        self.XML_object.crChldElem( "node1", "node2", "version" )

        self.XML_object.crChldElem( "node2", "node8", "attributes" )

        self.XML_object.crChldElem( "node8", "node9", "md5", {
            "extent":"x000",
            "sha384":package_object.attributesMetaSha384(),
            "size":package_object.attributesMetaSize(),
            } )
        self.XML_object.chlTxtNode( "node9", package_object.attributes_meta_MD5 )

        major_rollout = None
        if package_object.major_roll_out:
            major_rollout = package_object.major_roll_out
        else:
            major_rollout = "2"

        minor_rollout = None
        if package_object.minor_roll_out:
            minor_rollout = package_object.minor_roll_out
        else:
            minor_rollout = "1"

        last_modified = None
        if package_object.last_modified:
            last_modified = package_object.last_modified
        else:
            last_modified = time.strftime( "%Y-%m-%dT%H:%M:%S", time.gmtime() )

        self.XML_object.crChldElem( "node2", "node5", "rollOut", {
            "majorRollOut":major_rollout,
            "minorRollOut":minor_rollout,
            "version-id":package_object.version,
            "updated":last_modified,
            } )

        assert(isinstance(package_object.packageMetaSha384(),basestring)),"Wrong type for packageMetaSha384: %s - %s - %s"%(
            type(package_object.packageMetaSha384()),str(package_object.packageMetaSha384()),repr(package_object.packageMetaSha384()))
        assert(isinstance(package_object.packageMetaSize(),basestring))

        self.XML_object.crChldElem( "node2", "node6", "md5", {
            "extent":"x000",
            "sha384":package_object.packageMetaSha384(),
            "size":package_object.packageMetaSize(),
            } )
        self.XML_object.chlTxtNode( "node6", package_object.package_meta_MD5 )

    def save( self, product_line_GUID ):
        self.saveXMLDocumentAsMD5( self.wh_path, product_line_GUID )

# Class to create ultimate metadata

class MetaDataUltimate( XMLPrototype ):

    def __init__( self, wh_path, dic_rigid_object, dictionary, logger ):
        XMLPrototype.__init__( self, logger )

        self.wh_path = wh_path

        self.XML_object.crParElem( "ultimate", "xmlns", "badger:ultimate" )
        self.XML_object.crChldElem( "parent", "node1", "rigidNames" )

        for k, v in dic_rigid_object.iteritems():
            attributes = {
                "rigidName":k,
                }
            self.XML_object.crChldElem( "node1", "node2", "rigidName", attributes )

            attributes = {
                "extent":"x000",
                "sha384":v.getSHA384(),
                "size":v.getSize(),
                }
            self.XML_object.crChldElem( "node2", "node3", "md5", attributes )

            self.XML_object.chlTxtNode( "node3", v.getMD5() )

        self.XML_object.crChldElem( "parent", "node4", "dictionaries" )

        attributes = {
            "extent":"x000",
            "sha384":dictionary.getSHA384(),
            "size":dictionary.getSize(),
            }
        self.XML_object.crChldElem( "node4", "node5", "md5",  attributes )
        self.XML_object.chlTxtNode( "node5", dictionary.getMD5() )

        self.XML_object.crChldElem( "parent", "node6", "lastModified" )
        self.XML_object.chlTxtNode( "node6", time.strftime( "%Y-%m-%dT%H:%M:%S", time.gmtime() ) )

        self.saveXMLDocumentAsMD5( wh_path )

    def getPath( self ):
        return os.path.join( self.wh_path, "%s.xml" % ( self.getMD5() + "x000" ) )

# Class to create top level catalogue

class MetaCatalogue( XMLPrototype ):

    def __init__( self, wh_path, wh_TLC, ultimate, signature, logger ):

        XMLPrototype.__init__(self, logger )

        self.XML_object.crParElem( "catalogue", "xmlns", "badger:catalogue" )


        attributes = {
            "extent":"x000",
            "sha384":ultimate.getSHA384(),
            "size":ultimate.getSize(),
                }

        self.XML_object.crChldElem( "parent", "node1", "file" )
        self.XML_object.crChldElem( "node1", "node2", "md5", attributes )
        self.XML_object.chlTxtNode( "node2", ultimate.getMD5() )

        attributes = {
            "extent":"x000",
            "sha384":signature.getSHA384(),
            "size":signature.getSize(),
                }

        self.XML_object.crChldElem("parent", "node6", "sig")
        self.XML_object.crChldElem("node6", "node7", "md5", attributes )
        self.XML_object.chlTxtNode("node7", signature.getMD5())

        self.saveXMLDocument( wh_path, os.path.join( "catalogue", wh_TLC ) )

# Class to create attributes file

class MetaAttributes( XMLPrototype ):

    def __init__(self, package_object, wh_path, logger, subcomponent=False):
        XMLPrototype.__init__( self, logger )

        self.XML_object.crParElem( "ReleaseAttributes", "xmlns", "badger:ReleaseAttributes" )

        # Default home folder
        if package_object.default_home_folder:
            keys = ["name"]
            values = ["DefaultHomeFolder"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "Str1024" )
            self.XML_object.chlTxtNode( "node3", package_object.default_home_folder )

        # Features
        if package_object.features:
            values = ["Features"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )

            for feature in package_object.features:
                self.XML_object.crChldElem( "node2", "node3", "Feature" )
                self.XML_object.chlTxtNode( "node3", feature )

        values = ["Lifestage"]
        self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
        self.XML_object.crChldElem( "node2", "node3", "Lifestage" )
        self.XML_object.chlTxtNode( "node3", package_object.getLifeStage() )

        # Name
        if package_object.name:
            values = ["Name"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "Name" )
            self.XML_object.chlTxtNode( "node3", package_object.name )

        # Platforms
        if package_object.platforms:
            values = ["Platforms"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )

            for platform in package_object.platforms:
                  self.XML_object.crChldElem( "node2", "node3", "Platform" )
                  self.XML_object.chlTxtNode( "node3", platform )

        # Release tags
        if package_object.release_tags:

            values = ["ReleaseTags"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )

            for release_tag in package_object.release_tags:
                self.XML_object.crChldElem( "node2", "node3", "ReleaseTag" )
                self.XML_object.crChldElem("node3", "node4", "BaseVersion")

                base = None

                if release_tag.base:
                    base = release_tag.base
                    self.XML_object.chlTxtNode( "node4", base  )
                else:
                    base = "Null"

                self.XML_object.crChldElem( "node3", "node4", "Tag" )
                self.XML_object.chlTxtNode( "node4", release_tag.tag )

                self.XML_object.crChldElem( "node3", "node4", "Label" )
                if ReleaseTagEnumerations.release_tag_enums.has_key( release_tag.tag ):
                    self.XML_object.chlTxtNode( "node4", ReleaseTagEnumerations.release_tag_enums[release_tag.tag][base] )
                else:
                    logger.errorMessage( "Release tag: '%s' is invalid" % ( release_tag.tag ) )
        else:
            if not subcomponent:
                logger.errorMessage("NO RELEASE TAGS SPECIFIED")

        # Resubscriptions
        if package_object.resubscriptions:
            values = ["Resubscriptions"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )

            for resubscription in package_object.resubscriptions:
                self.XML_object.crChldElem( "node2", "node3", "Resubscription" )
                self.XML_object.crChldElem( "node3", "node4", "ProductLine" )
                self.XML_object.chlTxtNode( "node4", resubscription.line )
                assert resubscription.line != "SELF"
                self.XML_object.crChldElem("node3", "node4", "BaseVersion")
                self.XML_object.chlTxtNode( "node4", resubscription.baseversion )
                self.XML_object.crChldElem( "node3", "node4", "FullVersion" )
                self.XML_object.chlTxtNode( "node4", resubscription.version )

        # Roles
        if package_object.roles:
            values = ["Roles"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )

            for role in package_object.roles:
                self.XML_object.crChldElem( "node2", "node3", "Role" )
                self.XML_object.chlTxtNode( "node3", role )

        # Supplements
        if package_object.supplements:
            values = ["Supplements"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )

            for supplement in package_object.supplements:
                self.XML_object.crChldElem( "node2", "node3", "Supplement" )

                for repository_path in supplement.repositories:
                    self.XML_object.crChldElem( "node3", "node4", "Repository" )
                    self.XML_object.chlTxtNode( "node4", repository_path )

                self.XML_object.crChldElem( "node3", "node4", "Warehouse" )
                self.XML_object.chlTxtNode( "node4", supplement.catalogue )

                self.XML_object.crChldElem( "node3", "node4", "ProductRelease" )

                self.XML_object.crChldElem( "node4", "node5", "ProductLine" )
                self.XML_object.chlTxtNode( "node5", supplement.product_line )

                if supplement.version_number:
                    self.XML_object.crChldElem( "node4", "node5", "VersionSpec", ["type"], ["absolute"] )
                    self.XML_object.crChldElem( "node5", "node6", "VersionNumber" )
                    self.XML_object.chlTxtNode( "node6", supplement.version_number )
                else:
                    self.XML_object.crChldElem( "node4", "node5", "VersionSpec", ["type"], ["generic"] )
                    self.XML_object.crChldElem( "node5", "node6", "ReleaseTagId" )
                    self.XML_object.crChldElem( "node6", "node7", "BaseVersion" )
                    if supplement.base_version:
                        self.XML_object.chlTxtNode( "node7", supplement.base_version )
                    self.XML_object.crChldElem( "node6", "node7", "Tag" )
                    self.XML_object.chlTxtNode( "node7", supplement.release_tag )

                self.XML_object.crChldElem( "node3", "node4", "DecodePath" )
                self.XML_object.chlTxtNode( "node4", supplement.decode_path )

        # Target type
        if package_object.target_types:
            values = ["TargetTypes"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "TargetType" )
            self.XML_object.chlTxtNode( "node3", "ENDPOINT" )

        if package_object.SAV_line:
            values = ["SAVLine"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "SAVLine" )
            self.XML_object.chlTxtNode( "node3", package_object.SAV_line )

        if package_object.SAV_version:
            values = ["SAVVersion"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "Str1024" )
            self.XML_object.chlTxtNode( "node3", package_object.SAV_version )

        if package_object.virus_engine_version:
            values = ["VirusEngineVersion"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "Str1024" )
            self.XML_object.chlTxtNode( "node3", package_object.virus_engine_version )

        if package_object.virus_data_version:
            values = ["VirusDataVersion"]
            self.XML_object.crChldElem( "parent", "node2", "Attribute", keys, values )
            self.XML_object.crChldElem( "node2", "node3", "Str1024" )
            self.XML_object.chlTxtNode( "node3", package_object.virus_data_version )

        self.saveXMLDocumentAsMD5( wh_path )

class MetaSignature( XMLPrototype ):

    def __init__( self, wh_path, signature, public_cert, inter_cert, logger ):

        XMLPrototype.__init__( self, logger )

        self.XML_object.crParElem( "signatureFile", "xmlns", "badger:signature" )
        self.XML_object.crChldElem( "parent", "node1", "signature" )
        self.XML_object.chlTxtNode( "node1", signature )
        self.XML_object.crChldElem( "parent", "node2", "signing_cert" )
        self.XML_object.chlTxtNode( "node2", public_cert )
        self.XML_object.crChldElem( "parent", "node3", "intermediate_cert" )
        self.XML_object.chlTxtNode( "node3", inter_cert )

        self.saveXMLDocumentAsMD5( wh_path )

class MetaDictionary( XMLPrototype ):

    def __init__(self, wh_path, dictionary_set, li_package_instances, logger ):

        XMLPrototype.__init__( self, logger )

        self.XML_object.crParElem( "Dictionary", "xmlns", "badger:dictionary" )
        self.XML_object.crChldElem( "parent", "node1", "Lang" )
        self.XML_object.chlTxtNode( "node1", "en" )
        self.XML_object.crChldElem( "parent", "node1", "Labels" )

        self.XML_object.crChldElem( "node1", "node2", "Line" )

        for translation_ob in dictionary_set.li_product_lines:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "Name" )

        for package in li_package_instances:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [package.name] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", package.en_short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", package.en_long_desc )

        self.XML_object.crChldElem( "node1", "node2", "Features" )

        for translation_ob in dictionary_set.li_features:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "Platforms" )

        for translation_ob in dictionary_set.li_platforms:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "ReleaseTags" )

        for translation_ob in dictionary_set.li_release_tags:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "Lifestage" )

        for translation_ob in dictionary_set.li_lifestage:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "Roles" )

        for translation_ob in dictionary_set.li_roles:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "SAVLine" )

        for translation_ob in dictionary_set.li_sav_lines:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.XML_object.crChldElem( "node1", "node2", "TargetTypes" )

        for translation_ob in dictionary_set.li_target_types:
            self.XML_object.crChldElem( "node2", "node3", "Label", ["token"], [translation_ob.token] )

            self.XML_object.crChldElem( "node3", "node4", "Short" )
            self.XML_object.chlTxtNode( "node4", translation_ob.short_desc )

            self.XML_object.crChldElem( "node3", "node4", "Long" )
            self.XML_object.chlTxtNode( "node4", translation_ob.long_desc )

        self.saveXMLDocumentAsMD5( wh_path )

class MetaDictionaryCatalogue( XMLPrototype ):

    def __init__( self, wh_path, dictionary, logger ):

        XMLPrototype.__init__( self, logger )

        self.XML_object.crParElem( "DictionariesCatalog", "xmlns", "badger:dictionariescatalog" )
        self.XML_object.crChldElem( "parent", "node1", "DefaultDictionary" )
        self.XML_object.chlTxtNode( "node1", "en" )
        self.XML_object.crChldElem( "parent", "node1", "Dictionaries" )

        self.XML_object.crChldElem( "node1", "node2", "Dictionary", ["lang"], ["en"] )

        attributes = {
            "extent":"x000",
            "sha384":dictionary.getSHA384(),
            "size":dictionary.getSize(),
                }

        self.XML_object.crChldElem( "node2", "node3", "md5", attributes)
        self.XML_object.chlTxtNode( "node3", dictionary.getMD5() )

        self.saveXMLDocumentAsMD5( wh_path )

class ReleaseTagEnumerations(object):
    release_tag_enums = {
       "LATEST":{
            "0":("7E249D36-4577-472d-89DA-7F15BA9B3BE2"),
            "1":("FCC0A783-197B-49a2-86AE-C6DD528AB2D8"),
            "2":("B3E4B456-2796-4a6e-966C-D9E7B390C3FB"),
            "3":("9D7C5DC5-1582-4353-B596-06C629D37244"),
            "4":("609E9902-8BF4-48d5-BD31-5BFCE48BA6EA"),
            "5":("47DADAF0-0886-4472-A755-BDAC1EEEDD45"),
            "6":("24BE44DC-7759-4dd3-91EE-5183F716B569"),
            "7":("2EB1CFCE-EFA6-4712-8728-F5A419742A87"),
            "8":("8CD9A8D1-F8A3-44df-9F4E-60D955ACC20E"),
            "9":("C08A0BEC-67F1-4c58-8818-41011B0B44FC"),
            "10":("DA5A16AE-B0F8-4090-BFDE-F1174D726F1F"),
            "11":("DA5A16AE-B0F8-4090-BFDE-F1174D726F1F"),
            "Null":("B6344492-F52E-4c32-B96D-2001D1B753B9"),
        },
        "OLDEST":{
            "0":("B5F9DB05-0401-4eca-A945-07C7FFD3B655"),
            "1":("0B8BA7DF-D875-4cad-85A5-A96AE9576E6C"),
            "2":("ED47CA7D-06DA-43f6-871F-2E4E2F3B41BF"),
            "3":("F10D1059-3BDD-4566-BCDD-F7519318A657"),
            "4":("DDC86CAE-2FAE-4558-8F24-C45A6E4F5F35"),
            "5":("A33E9051-BDD0-42ae-BBE5-5E3C6B5EE2EE"),
            "6":("A1F17914-DB16-4cb6-9AFE-C7DC066A1F84"),
            "7":("5593107F-B11B-4e26-B701-4BFB6710B379"),
            "8":("71419D1C-4811-4465-AC82-DC9C9D8E06D7"),
            "9":("31AE543B-BB00-4af7-934F-AEBAC8D36B54"),
            "10":("ED22C3BF-3223-4bd3-8851-3616FACBFDE5"),
            "11":("ED22C3BF-3223-4bd3-8851-3616FACBFDE5"),
            "Null":("6B140872-CEC8-43f6-8CCB-C92E709DF9BE"),
        },
        "RECOMMENDED":{
            "0":("60E0CBE4-9EC7-42ba-9EA4-6E8D247C0B4B"),
            "1":("0C609ABC-6C26-4b50-BEAA-E11D6AC9ADA3"),
            "2":("BF43F837-AAB1-4272-BB20-0DAD17ACF694"),
            "3":("E5342D27-1F33-4efa-ACB7-B68BE04DAE37"),
            "4":("61C6B9F0-E4E7-400c-9811-E3877494A3CE"),
            "5":("748A247A-7F48-4940-B7EA-397CFC92273A"),
            "6":("97CA9D9A-D0B0-4830-A193-890B58546BE4"),
            "7":("64330BC8-9CEF-4071-90A3-83E1332D3C38"),
            "8":("BB05E859-107D-4691-AF7B-CBE4736DBA5C"),
            "9":("A76E1754-D345-4d53-B7CA-65954CD6F6E2"),
            "10":("BB7AC5A0-5734-40e3-8988-D944CBCA8E7C"),
            "11":("BB7AC5A0-5734-40e3-8988-D944CBCA8E7C"),
            "Null":("D0BE8E2D-027A-470d-90A5-7F053F9B7DE8"),
            },
        "PREVIOUS":{
            "0":("7931C192-3F43-4828-8B1F-016111F56BCE"),
            "1":("1A642F14-4A46-4cea-AFEA-2D02687F2D5E"),
            "2":("96124590-7207-4e0b-89EB-39CE8D0B5816"),
            "3":("99B500B8-19D4-44ec-BBA7-E086B6F82275"),
            "4":("05B50E90-366E-45b2-AA93-A77768F710F4"),
            "5":("654C51FD-1E90-458e-9BA1-C8EFDD89C4D0"),
            "6":("AAD336F8-70C6-4238-8718-A6E4E8BB4425"),
            "7":("5624FCC9-5875-4db9-ADED-799BC1514A9F"),
            "8":("B7E77169-FEEF-4309-AB6B-0B32ADED9BC2"),
            "9":("5A2B97D4-AF8A-4a05-A8C2-F92787939F6C"),
            "10":("F2CA814F-E39B-4d4e-A7B2-77EE100CB5AE"),
            "11":("F2CA814F-E39B-4d4e-A7B2-77EE100CB5AE"),
            "Null":("D7F5EC02-E345-4832-924C-3BCF469AC754"),
        }
    }
