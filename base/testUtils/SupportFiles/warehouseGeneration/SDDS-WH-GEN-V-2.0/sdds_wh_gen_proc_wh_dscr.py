from xml.dom import minidom
from xml.dom.minidom import Document
import xml.dom
import sys

# Process the import file that depicts the warehouse

class BadWarehouseDescriptionException(Exception):
    pass

class BadReleaseTag(BadWarehouseDescriptionException):
    pass


def getText(node):
    text = ""
    for n in node.childNodes:
       text += n.data
    return text

class ProcessWarehouseDescription(object):

    def __init__( self, wh_dscr_path, logger ):
        self.logger = logger
        self.error_state = 0
        self.wh_dscr_path = wh_dscr_path
        self.XML_document = None

        # List of dictionary paths
        self.li_dictionary_paths = []

        # List of objects
        self.li_product_instances = []

        self.m_deliberate_errors = {}

    def _load( self ):
        file = None

        self.logger.statusMessage( "Opening warehouse description file from: %s" % ( self.wh_dscr_path ) )

        try:
            file = open( self.wh_dscr_path, "rb" )
        except IOError, Error:
            self.logger.errorMessage( "Failed to open warehouse description file", Error )
            self.error_state = 1

        self.logger.statusMessage( "Parsing data from warehouse description file from : %s" % ( self.wh_dscr_path ) )

        try:
            self.XML_document = minidom.parse( file )
        except xml.parsers.expat.ExpatError, AttributeError:
            self.logger.errorMessage( "Failed to parse XML document", AttributeError )
            self.error_state = 1

        self.logger.statusMessage( "Sucessfully opened and parsed warehouse description file from: %s" % ( self.wh_dscr_path ) )

    def getParentProductInstanceNodes(self):
        xmldoc = self.XML_document
        sdds_data_node = self.XML_document.getElementsByTagName( "SDDSData" )[0]
        nodes = []
        for node in sdds_data_node.childNodes:
            if node.nodeType != xml.dom.Node.ELEMENT_NODE:
                continue
            if node.tagName != "ProductInstance":
                continue
            nodes.append(node)
        return nodes

    def parse( self ):
        self.logger.statusMessage( "Extracting data from DOM object and creating objects" )

        self.logger.statusMessage( "Looking for 'DictionaryFiles' node" )
        assert self.XML_document is not None
        dictionary_file_nodes = self.XML_document.getElementsByTagName( "DictionaryFiles" )

        if len(dictionary_file_nodes) == 1:
            for dictionaryNode in dictionary_file_nodes[0].childNodes:
                if dictionaryNode.nodeType == xml.dom.Node.ELEMENT_NODE:
                    if dictionaryNode.tagName == "DictionaryFile":
                        dictionary_path = getText(dictionaryNode)
                        self.logger.statusMessage( "Adding dictionary path: '%s'" % ( dictionary_path ) )
                        self.li_dictionary_paths.append( dictionary_path )

        elif dictionary_file_nodes.length == 0:
            self.logger.errorMessage( "Could not find a 'DictionaryFiles' node" )
            self.error_state = 1
        else:
            self.logger.errorMessage( "Too many 'DictionaryFiles' nodes" )
            self.error_state = 1

        self.logger.statusMessage( "Looking for 'ProductInstance' nodes" )

        product_instance_nodes = self.getParentProductInstanceNodes()

        if len(product_instance_nodes) > 0:
            self.logger.statusMessage( "Found %d 'ProductInstance' node(s)" % len(product_instance_nodes) )

            for (i,prod_inst_node) in enumerate(product_instance_nodes):
                self.logger.statusMessage( "Looking for child nodes in 'ProductInstance' node %d" % ( i + 1 ) )

                # Create ProductData object
                prod_inst_object = ProductData(self.logger)

                # Loop through all children of 'ProductInstance' node
                for prod_inst_child_node in prod_inst_node.childNodes:
                    if prod_inst_child_node.nodeType != xml.dom.Node.ELEMENT_NODE:
                        continue

                    instance_child_node = prod_inst_child_node

                    # If the child nodes is 'ComponentSuiteImport' get the file target
                    if instance_child_node.tagName == "ComponentSuite":
                        self.logger.statusMessage( "Found 'ComponentSuite' node" )

                        component_data = ComponentData()
                        component_data = self.getComponentData( instance_child_node, component_data )

                        prod_inst_object.component_data = component_data

                    # If the child node is 'ReleaseTag' find get child nodes
                    elif instance_child_node.tagName == "ReleaseTags":

                        # Loop through all children of 'release_tags' node and pull out data
                        for release_tag_node in prod_inst_child_node.childNodes:
                            if release_tag_node.nodeType != 1:
                                continue

                            if release_tag_node.tagName == "ReleaseTag":
                                self.logger.statusMessage( "Found 'ReleaseTag' node" )

                                release_tag = ReleaseTag()

                                for release_tag_child_node in release_tag_node.childNodes:
                                    if release_tag_child_node.nodeType == 1:

                                        if release_tag_child_node.tagName == "Tag":
                                            release_tag.tag = getText(release_tag_child_node)
                                            self.logger.statusMessage( "Adding release tag '%s'" % ( release_tag.tag ) )

                                        elif release_tag_child_node.tagName == "Base":
                                            release_tag.base = getText(release_tag_child_node)
                                            self.logger.statusMessage( "Adding base version '%s'" % ( release_tag.base ) )

                                else:
                                    prod_inst_object.addReleaseTag(release_tag)

                    # If the child node is 'FeatureSuppression' find get child nodes
                    elif instance_child_node.tagName == "FeatureSuppression":
                        self.logger.statusMessage( "Found 'FeatureSuppression' node" )

                        feature_suppression_node = prod_inst_child_node

                        # Loop through all children of 'FeatureSuppression' node and pull out data
                        for k in range( 0, len( feature_suppression_node.childNodes ) ):
                            if feature_suppression_node.childNodes[k].nodeType == 1:
                                feature = getText(feature_suppression_node.childNodes[k])

                                self.logger.statusMessage( "Adding suppression of feature '%s'" % ( feature ) )

                                prod_inst_object.suppressed_features.append( feature )

                    # If the child node is 'PlatformSuppression' find get child nodes
                    elif instance_child_node.tagName == "PlatformSuppression":
                        self.logger.statusMessage( "Found 'PlatformSuppression' node" )

                        platform_suppression_node = prod_inst_child_node

                        # Loop through all children of 'PlatformSuppression' node and pull out data
                        for k in range( 0, len( platform_suppression_node.childNodes ) ):
                            if platform_suppression_node.childNodes[k].nodeType == 1:
                                platform = getText(platform_suppression_node.childNodes[k])

                                self.logger.statusMessage( "Adding suppression of platform '%s'" % ( platform ) )

                                prod_inst_object.suppressed_platforms.append( platform )

                    # If the child node is 'ComponentImports' find get child nodes
                    elif instance_child_node.tagName == "Components":
                        self.logger.statusMessage( "Found 'Components' node" )

                        component_imports_node = prod_inst_child_node
                        # Loop through all children of 'ComponentImports' node and pull out data
                        for component_imports_node_child_node in component_imports_node.childNodes:
                            if component_imports_node_child_node.nodeType != xml.dom.Node.ELEMENT_NODE:
                                continue

                            if component_imports_node_child_node.tagName == "ProductInstance":
                                ## Go down another level
                                for c in component_imports_node_child_node.childNodes:
                                    if c.nodeType != xml.dom.Node.ELEMENT_NODE:
                                        continue
                                    if c.tagName != "ComponentSuite":
                                        continue
                                    component_imports_node_child_node = c
                                    break
                                assert component_imports_node_child_node.nodeType == xml.dom.Node.ELEMENT_NODE
                                assert component_imports_node_child_node.tagName == "ComponentSuite"

                            component_data = ComponentData()
                            component_data = self.getComponentData( component_imports_node_child_node, component_data )

                            if component_data.component_import_path:
                                prod_inst_object.child_component_data.append( component_data )
                            else:
                                self.logger.errorMessage( "'Component' lacks a component suite import path" )
                                self.error_state = 1

                if prod_inst_object.component_data.component_import_path is None:
                    self.logger.errorMessage( "'ProductInstance' lacks a component suite import path" )
                    self.error_state = 1
                else:
                    self.logger.statusMessage( "Adding object with 'ComponentSuiteImport' '%s' to publishable list"
                                               % prod_inst_object.component_data.component_import_path )
                    self.li_product_instances.append( prod_inst_object )
        else:
            self.logger.errorMessage( "Could not find any 'ProductInstance' nodes" )
            self.error_state = 1



        error_nodes = self.XML_document.getElementsByTagName( "DeliberateError" )

        for error_node in error_nodes:
            if error_node.nodeType != xml.dom.Node.ELEMENT_NODE:
                continue
            text = getText(error_node)
            error_type = error_node.getAttribute("type")
            if error_type == "file":
                key = error_node.getAttribute("md5")
            else:
                key = error_type
            self.m_deliberate_errors[key] = text

    def getComponentData( self, XML_node, component_data ):
        # Loop through all children of the current node and pull out data
        for childNode in XML_node.childNodes:
            if childNode.nodeType != 1:
                continue

            if childNode.tagName == "ComponentImport":
                component_data.component_import_path = getText(childNode)

                self.logger.statusMessage( "Adding component import path '%s'" % ( component_data.component_import_path ) )

            elif childNode.tagName == "DecodePath":
                self.logger.statusMessage( "Found 'DecodePath' node" )
                decodePath = getText(childNode)
                if decodePath is not None:
                    decodePath = decodePath.strip()
                    if decodePath != "":
                        component_data.decode_path = decodePath

            elif childNode.tagName == "MajorRelease":
                component_data.major_roll_out = getText(childNode)

                self.logger.statusMessage( "Adding component major roll out number: '%s'" % ( component_data.major_roll_out ) )

            elif childNode.tagName == "MinorRelease":
                component_data.minor_roll_out = getText(childNode)

                self.logger.statusMessage( "Adding component minor roll out number: '%s'" % ( component_data.minor_roll_out ) )

            elif childNode.tagName == "TimeStamp":
                component_data.last_modified = getText(childNode)

                self.logger.statusMessage( "Adding component last modified timestamp: '%s'" % ( component_data.last_modified ) )

            elif childNode.tagName == "LifeStage":
                component_data.setLifeStage(getText(childNode))
                self.logger.statusMessage( "Adding component life stage: '%s'" % ( component_data.getLifeStage() ) )

            elif childNode.tagName == "Supplements":

                for supplementnode in childNode.childNodes:
                    if supplementnode.nodeType != xml.dom.Node.ELEMENT_NODE:
                        continue

                    if supplementnode.tagName != "Supplement":
                        continue

                    self.logger.statusMessage( "Found 'Supplement' node" )

                    supplement = Supplement()

                    for supplementdetailsnode in supplementnode.childNodes:
                        if supplementdetailsnode.nodeType != 1:
                            continue

                        if supplementdetailsnode.tagName == "Repositories":

                            repository_nodes = supplementdetailsnode.childNodes

                            for repository_node in repository_nodes:
                                if repository_node.nodeType != 1:
                                    continue

                                if repository_node.tagName == "Repository":
                                    self.logger.statusMessage( "Found 'Repository' node" )
                                    supplement.repositories.append( getText(repository_node) )
                                    self.logger.statusMessage(
                                        "Adding supplement repository: '%s'" % ( getText(repository_node) ) )

                        elif supplementdetailsnode.tagName == "Catalogue":
                            supplement.catalogue = getText(supplementdetailsnode)
                            self.logger.statusMessage( "Adding supplement catalogue: '%s'" % ( supplement.catalogue ) )

                        elif supplementdetailsnode.tagName == "ProductLine":
                            supplement.product_line = getText(supplementdetailsnode)
                            self.logger.statusMessage( "Adding supplement line: '%s'" % ( supplement.product_line ) )

                        elif supplementdetailsnode.tagName == "BaseVersion":
                            supplement.base_version = getText(supplementdetailsnode)
                            self.logger.statusMessage( "Adding supplement base version: '%s'" % ( supplement.base_version ) )

                        elif supplementdetailsnode.tagName == "ReleaseTag":
                            supplement.release_tag = getText(supplementdetailsnode)
                            self.logger.statusMessage( "Adding supplement release tag: '%s'" % ( supplement.release_tag ) )

                        elif supplementdetailsnode.tagName == "VersionNumber":
                            supplement.version_number = getText(supplementdetailsnode)
                            self.logger.statusMessage( "Adding supplement version number: '%s'" % ( supplement.version_number ) )

                        elif supplementdetailsnode.tagName == "DecodePath":
                            supplement.decode_path = getText(supplementdetailsnode)
                            self.logger.statusMessage( "Adding supplement decode path: '%s'" % ( supplement.decode_path ) )

                    if not supplement.catalogue:
                        self.logger.errorMessage( "Incomplete supplement specification - catalogue not set" )
                        self.error_state = 1
                    if not supplement.product_line:
                        self.logger.errorMessage( "Incomplete supplement specification - product line not set" )
                        self.error_state = 1
                    if not supplement.base_version and not supplement.release_tag:
                        if not supplement.version_number:
                            self.logger.errorMessage( "Incomplete supplement specification - base version\release tag OR version number not set" )
                            self.error_state = 1
                    if not supplement.decode_path:
                        self.logger.errorMessage( "Incomplete supplement specification - decode path not set" )
                        self.error_state = 1

                    if self.error_state == 0:
                        component_data.supplements.append( supplement )

            elif childNode.tagName == "Resubscriptions":

                for resubscriptionNode in childNode.childNodes:
                    if resubscriptionNode.nodeType != 1:
                        continue

                    if resubscriptionNode.tagName != "Resubscription":
                        continue

                    self.logger.statusMessage( "Found 'Resubscription' node" )

                    resubscription = Resubscription()

                    for detailNode in resubscriptionNode.childNodes:
                        if detailNode.nodeType != 1:
                            continue

                        if detailNode.tagName == "Line":
                            resubscription.line = getText(detailNode)
                            self.logger.statusMessage( "Adding resubscription line: '%s'" % ( resubscription.line ) )

                        elif detailNode.tagName == "Version":
                            resubscription.version = getText(detailNode)
                            self.logger.statusMessage( "Adding resubscription version: '%s'" % ( resubscription.version ) )

                        elif detailNode.tagName == "BaseVersion":
                            resubscription.baseversion = getText(detailNode)
                            self.logger.statusMessage( "Adding resubscription baseversion: '%s'" % ( resubscription.baseversion ) )

                    if not resubscription.line or not resubscription.version or not resubscription.baseversion:

                        self.logger.errorMessage( "Incomplete resubscription specification" )
                        self.error_state = 1
                    else:
                        component_data.addResubscription(resubscription)

        return component_data

    def getErrorState( self ):
        return self.error_state

# Container for product data parsed from the SDDS import manifest example

class ProductData(object):

    def __init__( self, logger ):
        self.logger = logger
        self.component_data = None

        self.release_tags = []
        self.suppressed_features = []
        self.suppressed_platforms = []

        self.child_component_data = []

        # Storage of 'PackageData' object
        self.parent_object = None

        # IDs relating to child components
        self.child_object_IDs = []

    def addReleaseTag(self, tag):
        if not tag.tag:
            self.logger.errorMessage( "'ReleaseTag' lacks 'Tag' element" )
            raise BadReleaseTag("'ReleaseTag' lacks 'Tag' element")

        self.logger.statusMessage("*** RELEASE TAG: %s,%s ***"%(tag.tag, str(tag.base)))
        if not tag.base:
            self.logger.statusMessage( "ReleaseTag "+tag.tag+" lacks 'Base' element" )
        self.release_tags.append(tag)

# Container for data pertaining to indivual components

class ComponentData(object):

    def __init__( self ):
        self.component_import_path = None
        self.decode_path = None

        self.supplements = []
        self.resubscriptions = []
        self.last_modified = None
        self.major_roll_out = None
        self.minor_roll_out = None
        self.m_lifestage = "APPROVED"

    def getLifeStage(self):
        return self.m_lifestage

    def setLifeStage(self, stage):
        self.m_lifestage = stage
        return self

    def addResubscription(self, resub):
        self.resubscriptions.append(resub)

# Container for supplemental data

class Supplement(object):

    def __init__( self ):
        self.repositories = []
        self.catalogue = None
        self.product_line = None
        self.base_version = None
        self.release_tag = None
        self.version_number = None
        self.decode_path = None

# Container for resubscription

class Resubscription(object):

    def __init__( self ):
        self.line = None
        self.version = None
        self.baseversion = None

    def setParentRigidName(self, parentRigidName):
        if self.line == "SELF":
            self.line = parentRigidName

class ReleaseTag(object):

    def __init__( self ):
        self.tag = None
        self.base = None
