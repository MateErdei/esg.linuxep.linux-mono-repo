from xml.dom import minidom
from xml.dom.minidom import Document
import xml.dom

import sys


class ProcessDictionaries(object):
    """
    Create dictionary objects containing
    """

    def __init__( self, li_dictionary_paths, logger ):
        self.logger = logger
        self.error_state = 0
        self.li_dictionary_paths = li_dictionary_paths

        self.dictionary_set = DictionarySet()
  
    def _load( self, dictionary_import_path ):
        file = None
        XML_document = None
        
        self.logger.statusMessage( "Opening dictionary file from: %s" % ( dictionary_import_path ) ) 
        
        try:
            file = open( dictionary_import_path, "rb" )       
        except IOError as Error:
            self.logger.errorMessage( "Failed to open dictionary file", Error )

        self.logger.statusMessage( "Parsing data from dictionary file from: %s" % ( dictionary_import_path ) )    

        try:     
            XML_document = minidom.parse( file )
        except xml.parsers.expat.ExpatError as AttributeError:
            self.logger.errorMessage( "Failed to parse XML document", AttributeError )

        self.logger.statusMessage( "Successfully opened and parsed dictionary file from: %s" % ( dictionary_import_path ) )

        return XML_document

    def processDics( self ):
        for dictionary_path in self.li_dictionary_paths:
            XML_document = self._load( dictionary_path )
            assert XML_document is not None
                
            self.logger.statusMessage( "Extracting data from DOM object" )

            self.logger.statusMessage( "Looking for acceptable dictionary node" )
            dic_type_nodes = XML_document.getElementsByTagName( "Dictionary" )
        
            if len( dic_type_nodes ) > 1 or len( dic_type_nodes ) == 0:
                self.logger.errorMessage( "Too many or too few 'Dictionary' nodes" )
                self.error_state = 1
            else:
                dic_type_node = dic_type_nodes[0]
                for child_node in dic_type_node.childNodes:
                    if child_node.nodeType == 1:
                        label_nodes = child_node.childNodes

                        for label_node in label_nodes:
                            if label_node.nodeType == 1 and label_node.tagName == "Label":
                                dictionary_entry = DictionaryEntry()

                                for lable_child_node in label_node.childNodes:
                                    if lable_child_node.nodeType == 1:
                                        if lable_child_node.tagName == "Token":
                                            dictionary_entry.token = lable_child_node.firstChild.data
                                        if lable_child_node.tagName == "Language":
                                            for lang_child_node in lable_child_node.childNodes:
                                                if lang_child_node.nodeType == 1:
                                                    if lang_child_node.tagName == "LongDesc":
                                                        dictionary_entry.long_desc = lang_child_node.firstChild.data
                                                    if lang_child_node.tagName == "ShortDesc":
                                                        dictionary_entry.short_desc = lang_child_node.firstChild.data

                                self.logger.traceMessage( "Found translation:" )
                                self.logger.traceMessage( "Token: '%s'" % ( dictionary_entry.token ) )
                                self.logger.traceMessage( "Long description: '%s'" % ( dictionary_entry.long_desc ) )
                                self.logger.traceMessage( "Short description: '%s'" % ( dictionary_entry.short_desc ) )
                                  
                                if child_node.tagName == "Features":
                                    self.logger.traceMessage( "Adding translation to 'Features'" )
                                    self.dictionary_set.li_features.append( dictionary_entry )

                                if child_node.tagName == "Identity":
                                    self.logger.traceMessage( "Adding translation to 'ProductLines'" )
                                    self.dictionary_set.li_product_lines.append( dictionary_entry )

                                if child_node.tagName == "Lifestage":
                                    self.logger.traceMessage( "Adding translation to 'Lifestage'" )
                                    self.dictionary_set.li_lifestage.append( dictionary_entry )

                                if child_node.tagName == "ReleaseTags":
                                    self.logger.traceMessage( "Adding translation to 'ReleaseTags'" )
                                    self.dictionary_set.li_release_tags.append( dictionary_entry )

                                if child_node.tagName == "Platforms":
                                    self.logger.traceMessage( "Adding translation to 'Platforms'" )
                                    self.dictionary_set.li_platforms.append( dictionary_entry )

                                if child_node.tagName == "Roles":
                                    self.logger.traceMessage( "Adding translation to 'Roles'" )
                                    self.dictionary_set.li_roles.append( dictionary_entry )

                                if child_node.tagName == "SAVLine":
                                    self.logger.traceMessage( "Adding translation to 'SAVLine'" )
                                    self.dictionary_set.li_sav_lines.append( dictionary_entry )

                                if child_node.tagName == "TargetTypes":
                                    self.logger.traceMessage( "Adding translation to 'TargetTypes'" )
                                    self.dictionary_set.li_target_types.append( dictionary_entry )


class DictionarySet(object):
    """
     Container for all dictionary entries in order
    """

    def __init__( self ):
        self.li_features = []
        self.li_lifestage = []
        self.li_platforms = []
        self.li_product_lines = []
        self.li_release_tags = []
        self.li_roles = []
        self.li_sav_lines = []
        self.li_target_types = []


class DictionaryEntry(object):
    """
    Containing for individual translation data
    """
    def __init__( self ):
        self.token = None
        self.language = None
        self.long_desc = None
        self.short_desc = None
