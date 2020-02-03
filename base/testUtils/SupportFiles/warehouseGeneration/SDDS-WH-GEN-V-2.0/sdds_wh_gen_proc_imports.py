from xml.dom import minidom
import xml.dom

import sys
# Create package objects based on the component suite\component suite

def getText(node):
    text = ""
    for n in node.childNodes:
       text += n.data
    return text

class ProcessSDDSImports(object):

    def __init__( self, li_product_instances, logger ):
        self.logger = logger
        self.error_state = 0
        self.li_product_instances = li_product_instances
        self.li_child_objects = []

    def _load( self, comp_import_path ):
        file = None
        XML_document = None

        self.logger.statusMessage( "Opening SDDS import file from: %s" % ( comp_import_path ) )

        try:
            file = open( comp_import_path, "rb" )
        except IOError, Error:
            self.logger.errorMessage( "Failed to open SDDS import file", Error )
            raise
        assert file is not None

        self.logger.statusMessage( "Parsing data from SDDS import file from: %s" % ( comp_import_path ) )

        try:
            XML_document = minidom.parse( file )
        except xml.parsers.expat.ExpatError, AttributeError:
            self.logger.errorMessage( "Failed to parse XML document", AttributeError )
            print >>sys.stderr,"Failed to read",comp_import_path
            data = open( comp_import_path, "rb" ).read()
            print >>sys.stderr,"File contents: ###",data,"###"
            raise

        self.logger.statusMessage( "Sucessfully opened and parsed SDDS import file from: %s" % ( comp_import_path ) )


        assert XML_document is not None
        return XML_document

    # Cycle through list of product instance descriptions, create 'PackageData' object for parent and children packages
    # add releade tags, suppress features/platforms

    def processImports( self ):
        packageID = 0

        self.logger.statusMessage( "Generating data objects based on data parsed from warehouse description file" )

        alreadyProcessedImportPaths = {}

        for product_instance in self.li_product_instances:
            comp_import_path = product_instance.component_data.component_import_path
            product_instance.parent_object = alreadyProcessedImportPaths.get(comp_import_path,None)
            if product_instance.parent_object is not None:
                self.logger.statusMessage( "Not processing duplicate instance with component suite import file: '%s'" % ( comp_import_path ) )
                continue

            assert product_instance.parent_object is None
            self.logger.statusMessage( "Processing product instance with component suite import file: '%s'" % ( comp_import_path ) )

            XML_document = self._load( comp_import_path )
            assert XML_document is not None

            # Add the current 'PackageData' object in to the 'ProductData' object
            parent_object = PackageData(product_instance)
            product_instance.parent_object = self.extractDataFromImport( XML_document, parent_object )
            assert product_instance.parent_object is not None

            alreadyProcessedImportPaths[comp_import_path] = product_instance.parent_object

            if product_instance.parent_object.product_type != "Component Suite":
                self.logger.statusMessage( "warning: component suite SDDS-import file isn't a component suite" )

            # List of features comprised of the parent and the child packages
            li_compound_features = product_instance.parent_object.features

            # List of platforms comprised of the parent and the child packages
            li_compound_platforms = product_instance.parent_object.platforms

            # Cycle through list of child import paths and create 'PackageData' objects based on the import files
            for child_component_data in product_instance.child_component_data:

                create_new_package = True
                existing_child_package_ID = None

                # Before adding the 'PackageData' object to the list and attaching it to a parent object, check if it's not a duplicate
                for ch_object in self.li_child_objects:
                    if child_component_data.component_import_path == ch_object.SDDS_import_path:
                        create_new_package = False
                        existing_child_package_ID = ch_object.object_ID

                if create_new_package:
                    self.logger.statusMessage( "New SDDS-import file found. Creating object ID: '%s'" % ( str( packageID ) ) )

                    child_object = PackageData(component_data=child_component_data)
                    XML_document = self._load( child_component_data.component_import_path )
                    assert XML_document is not None

                    child_object.object_ID = packageID

                    child_object = self.extractDataFromImport( XML_document, child_object )
                    assert child_object is not None
                    alreadyProcessedImportPaths[child_component_data.component_import_path] = child_object ## Don't process in it's own right

                    if child_object.product_type != "Component":
                        self.logger.errorMessage( "Component SDDS-import file isn't a component" )
                        self.error_state = 1
                    else:
                        child_object.supplements = child_component_data.supplements
                        child_object.resubscriptions = child_component_data.resubscriptions

                        self.li_child_objects.append( child_object )

                        # Add 'PackageData' ID to parent product instance object
                        self.logger.statusMessage( "Linking product instance: '%s' to package ID: '%s'" % (
                            product_instance.parent_object.en_long_desc, str( packageID ) ) )
                        product_instance.child_object_IDs.append( packageID )

                        packageID += 1

                        # Add the child components features/platforms to the compound lists
                        li_compound_features = self.addToList( li_compound_features, child_object.features )
                        li_compound_platforms = self.addToList( li_compound_platforms, child_object.platforms )

                else:
                    self.logger.statusMessage( "Package already exists. Ignoring and using package ID: '%s'" % ( str( existing_child_package_ID ) ) )
                    self.logger.statusMessage( "Linking product instance: '%s' to package ID: '%s'" % (
                        product_instance.parent_object.en_long_desc, str( existing_child_package_ID ) ) )
                    product_instance.child_object_IDs.append( existing_child_package_ID )

            # Save the updated lists back the parent
            product_instance.parent_object.features = li_compound_features
            product_instance.parent_object.platforms = li_compound_platforms

            # Suppress platfoms and features as per WH_desr
            product_instance.parent_object.features = self.removeFromList( product_instance.parent_object.features,
                                                                           product_instance.suppressed_features )
            product_instance.parent_object.platforms = self.removeFromList( product_instance.parent_object.platforms,
                                                                            product_instance.suppressed_platforms )

            # Transfer to release tags list
            product_instance.parent_object.release_tags = product_instance.release_tags

        for pi in self.li_product_instances:
            parent_object = pi.parent_object
            self.logger.statusMessage("*** Got Product instance with rigid_name=%s, version=%s, parent_object=%s"%
                (
                parent_object.rigid_name,
                parent_object.version,
                str(parent_object)
                )
            )

    # Extract data from each indivual SDDS import file
    def extractDataFromImport( self, XML_document, instance_object ):
        self.logger.statusMessage( "Extracting data from DOM object" )

        self.logger.statusMessage( "Looking for 'Component' node" )
        assert XML_document is not None
        component_nodes = XML_document.getElementsByTagName( "Component" )

        if component_nodes.length == 1:
            self.logger.statusMessage( "Found single 'Component' node" )

            # Loop through all children of 'Component' node
            for component_child_node in component_nodes[0].childNodes:
                if component_child_node.nodeType != 1:
                    continue

                if component_child_node.tagName == "Name":
                    self.logger.statusMessage( "Found 'Name' node" )
                    instance_object.name = getText(component_child_node)

                elif component_child_node.tagName == "RigidName":
                    self.logger.statusMessage( "Found 'RigidName' node" )
                    instance_object.setRigidName(getText(component_child_node))

                elif component_child_node.tagName == "Version":
                    self.logger.statusMessage( "Found 'Version' node" )
                    instance_object.version = getText(component_child_node)
                    if instance_object.version == "":
                        self.logger.errorMessage("Empty version element found in SDDS-Import.xml!")
                        assert instance_object.version != "","Got an empty version node in SDDS-Import.xml"

                elif component_child_node.tagName == "Build":
                    self.logger.statusMessage( "Found 'Build' node" )
                    instance_object.build = getText(component_child_node)

                elif component_child_node.tagName == "ProductType":
                    self.logger.statusMessage( "Found 'ProductType' node" )
                    instance_object.product_type = getText(component_child_node)

                elif component_child_node.tagName == "DefaultHomeFolder":
                    self.logger.statusMessage( "Found 'DefaultHomeFolder' node" )
                    instance_object.default_home_folder = getText(component_child_node)

            object_incomplete = False
            if not instance_object.name or not  instance_object.rigid_name or not instance_object.version:
                object_incomplete = True
            if not instance_object.build or not instance_object.product_type or not instance_object.default_home_folder:
                object_incomplete = True

            if not object_incomplete:
                self.logger.statusMessage( "Creating package object" )
                self.logger.statusMessage( "Name: '%s'" % ( instance_object.name ) )
                self.logger.statusMessage( "Rigid Name: '%s'" % ( instance_object.rigid_name ) )
                self.logger.statusMessage( "Version: '%s'" % ( instance_object.version ) )
                self.logger.statusMessage( "Build: '%s'" % ( instance_object.build ) )
                self.logger.statusMessage( "Product Type: '%s'" % ( instance_object.product_type ) )
                self.logger.statusMessage( "Default Home Folder: '%s'" % ( instance_object.default_home_folder ) )

                for i in range( 0, len( component_nodes[0].childNodes ) ):
                    component_child_node = component_nodes[0].childNodes[i]
                    if component_child_node.nodeType != 1:
                        continue

                    if component_child_node.tagName == "TargetTypes":
                        self.logger.statusMessage( "Found 'TargetType' node" )

                        # Loop through all children of 'TargetType' node and pull out data
                        for k in range( 0, len( component_child_node.childNodes ) ):
                            if component_child_node.childNodes[k].nodeType == 1:
                                target_type = component_child_node.childNodes[k].attributes["Name"].value
                                instance_object.target_types.append( target_type )
                                self.logger.statusMessage( "Adding target type '%s'" % ( target_type ) )

                    elif component_child_node.tagName == "Roles":
                        self.logger.statusMessage( "Found 'Roles' node" )

                        # Loop through all children of 'Roles' node and pull out data
                        for k in range( 0, len( component_child_node.childNodes ) ):
                            if component_child_node.childNodes[k].nodeType == 1:
                                role = component_child_node.childNodes[k].attributes["Name"].value
                                instance_object.roles.append( role )
                                self.logger.statusMessage( "Adding role '%s'" % ( role ) )

                    elif component_child_node.tagName == "Platforms":
                        self.logger.statusMessage( "Found 'Platforms' node" )

                        # Loop through all children of 'Platforms' node and pull out data
                        for k in range( 0, len( component_child_node.childNodes ) ):
                            if component_child_node.childNodes[k].nodeType == 1:
                                platform = component_child_node.childNodes[k].attributes["Name"].value
                                instance_object.platforms.append( platform )
                                self.logger.statusMessage( "Adding platform '%s'" % ( platform ) )

                    elif component_child_node.tagName == "Features":
                        self.logger.statusMessage( "Found 'Features' node" )

                        # Loop through all children of 'Features' node and pull out data
                        for k in range( 0, len( component_child_node.childNodes ) ):
                            if component_child_node.childNodes[k].nodeType == 1:
                                feature = component_child_node.childNodes[k].attributes["Name"].value
                                instance_object.features.append( feature )
                                self.logger.statusMessage( "Adding platform '%s'" % ( feature ) )

                    elif component_child_node.tagName == "SAVLine":
                        self.logger.statusMessage( "Found 'SAVLine' node" )
                        instance_object.SAV_line = component_child_node.firstChild.data
                        self.logger.statusMessage( "SAV Line: '%s'" % ( instance_object.SAV_line ) )

                    elif component_child_node.tagName == "SAVVersion":
                        self.logger.statusMessage( "Found 'SAVVersion' node" )
                        instance_object.SAV_version = component_child_node.firstChild.data
                        self.logger.statusMessage( "SAV Version: '%s'" % ( instance_object.SAV_version ) )

                    elif component_child_node.tagName == "VirusDataVersion":
                        self.logger.statusMessage( "Found 'VirusDataVersion' node" )
                        instance_object.virus_data_version = component_child_node.firstChild.data
                        self.logger.statusMessage( "Virus Data Version: '%s'" % ( instance_object.virus_data_version ) )

                    elif component_child_node.tagName == "VirusEngineVersion":
                        self.logger.statusMessage( "Found 'VirusEngineVersion' node" )
                        instance_object.virus_engine_version = component_child_node.firstChild.data
                        self.logger.statusMessage( "Virus Engine Version: '%s'" % ( instance_object.virus_engine_version ) )

                    elif component_child_node.tagName == "FileList":
                        self.logger.statusMessage( "Found 'FileList' node" )

                        # Loop through all children of 'FileList' node and pull out data
                        for filenode in component_child_node.childNodes:
                            if filenode.nodeType != 1:
                                continue

                            # Create 'File' object
                            file_data = File()

                            file_data.MD5 = filenode.attributes["MD5"].value
                            file_data.name = filenode.attributes["Name"].value
                            if "Offset" in filenode.attributes.keys():
                                file_data.offset = filenode.attributes["Offset"].value
                            file_data.size = filenode.attributes["Size"].value

                            instance_object.file_data.append( file_data )

                            self.logger.statusMessage( "Adding file '%s' MD5=%s" % ( file_data.name , file_data.MD5 ) )

                        if len( component_child_node.childNodes ) == 0:
                            self.logger.errorMessage( "SDDS-import file contains 0 files" )
                            self.error_state = 1
            else:
                 self.logger.errorMessage( "SDDS-import file is missing mandatory data elements" )
                 self.error_state = 1

        else:
            self.logger.errorMessage( "Either none or too many 'Component' nodes exist" )
            self.error_state = 1

        label_nodes = XML_document.getElementsByTagName( "Label" )

        if label_nodes.length > 0:
            self.logger.statusMessage( "Found single 'Label' node" )

            # Loop through all children of 'Label' node
            for i in range( 0, len( label_nodes[0].childNodes ) ):
                if label_nodes[0].childNodes[i].nodeType == 1:
                    label_child_node = label_nodes[0].childNodes[i]

                    if label_child_node.tagName == "Token":
                        self.logger.statusMessage( "Found 'Token' node" )
                        instance_object.dictionary_label_token = label_child_node.firstChild.data
                        self.logger.statusMessage( "Token: '%s'" % ( instance_object.dictionary_label_token ) )

                    if label_child_node.tagName == "Language":
                        self.logger.statusMessage( "Found 'Language' node" )
                        instance_object.en_short_desc = getText(label_child_node.childNodes[0])
                        self.logger.statusMessage( "Short Description: '%s'" % ( instance_object.en_short_desc ) )
                        instance_object.en_long_desc = label_child_node.childNodes[1].firstChild.data
                        self.logger.statusMessage( "Long Description: '%s'" % ( instance_object.en_long_desc ) )

        else:
            self.logger.errorMessage( "There are no 'Label' nodes" )
            self.error_state = 1

        # return the object
        if self.error_state == 0:
            return instance_object
        else:
            return None

    def addToList( self, original_list, new_list ):
        for new_element in new_list:
            if not new_element in original_list:
                original_list.append( new_element )

        return original_list

    def removeFromList( self, original_list, new_list ):
        for new_element in new_list :
            if new_element in original_list:
                original_list.remove( new_element )

        return original_list

# Container for individual products parses from their sdds-import file

class PackageData(object):

    def __init__(self, product_instance=None, component_data=None):
        self.object_ID = None
        self.package_meta_MD5 = None
        self.__m_package_meta_sha384 = None
        self.__m_package_meta_size = None

        self.attributes_meta_MD5 = None
        self.__m_attributes_meta_sha384 = None
        self.__m_attributes_meta_size = None

        # Attributes of package
        self.name = None
        self.rigid_name = None
        self.version = None
        self.build = None
        self.product_type = None
        self.default_home_folder = None
        self.target_types = []
        self.roles = []
        self.platforms = []
        self.features = []
        self.SAV_line = None
        self.SAV_version = None
        self.virus_data_version = None
        self.virus_engine_version = None
        self.file_data = []
        self.dictionary_label_token = None
        self.en_short_desc = None
        self.en_long_desc = None

        self.release_tags = []

        if component_data is None:
            component_data = product_instance.component_data

        self.__m_lifestage = component_data.getLifeStage()
        self.last_modified = component_data.last_modified
        self.major_roll_out = component_data.major_roll_out
        self.minor_roll_out = component_data.minor_roll_out
        self.supplements = component_data.supplements
        self.resubscriptions = component_data.resubscriptions
        self.SDDS_import_path = component_data.component_import_path
        self.decode_path = component_data.decode_path

    def getLifeStage(self):
        return self.__m_lifestage

    def setRigidName(self, rigidName):
        self.rigid_name = rigidName
        for resub in self.resubscriptions:
            resub.setParentRigidName(rigidName)

    def getSHA384(self):
        return self.__m_package_meta_sha384

    def packageMetaSha384(self, v=None):
        if v is not None:
            assert(isinstance(v,basestring))
            self.__m_package_meta_sha384 = v
        return self.__m_package_meta_sha384

    def getSize(self):
        return  self.__m_package_meta_size

    def packageMetaSize(self, v=None):
        if v is not None:
            assert(isinstance(v,basestring))
            self.__m_package_meta_size = v
        return self.__m_package_meta_size

    def attributesMetaSha384(self, v=None):
        if v is not None:
            assert(isinstance(v,basestring))
            self.__m_attributes_meta_sha384 = v

        return self.__m_attributes_meta_sha384

    def attributesMetaSize(self, v=None):
        if v is not None:
            assert(isinstance(v,basestring))
            self.__m_attributes_meta_size = v
        return self.__m_attributes_meta_size

# Container for file data

class File(object):
    def __init__( self ):
        self.MD5 = None
        self.name = None
        self.offset = None
        self.size = None
        self.last_modified = None
        self.__m_sha384 = "NOT_YET"

    def getSHA384(self):
        return self.__m_sha384

    def setSHA384(self, value):
        self.__m_sha384 = value
