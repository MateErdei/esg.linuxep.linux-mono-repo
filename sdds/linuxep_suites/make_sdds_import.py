##############################################################################
#WAREHOUSE VALIDATOR: MAIN
# Python 3.x
##############################################################################

import hashlib
import os
import sys

from optparse import OptionParser
from time import gmtime, strftime
from xml.dom import minidom
#from xml.dom.minidom import getDOMImplementation


class Ex_General(Exception):
    """Class for raising exception"""
    #(msg)

#-----------------------------------------------------------------------------
#Debug
#-----------------------------------------------------------------------------
    
def output_deliverables_info():
    """Prints-out content of deliverables-information-list. For debug."""
    global g_dlvrbls_info
    print("deliverables-info (%d):"%(len(g_dlvrbls_info)))
    for record in g_dlvrbls_info:
        filename = record[0]
        filesize = record[1]
        filecsum = record[2]
        relfldr  = record[3]
        print(" Name='%s' Size='%s' MD5='%s' Offset='%s'"%(filename,filesize,filecsum,relfldr))

        
def output_product_description_dict():
    """Prints-out contents of product-description-dictionary. For debug."""
    global g_dscrptn_dict
    print("ProductLineId            [%s]"%(g_dscrptn_dict["ProductLineId"]))
    print("ProductVersionId         [%s]"%(g_dscrptn_dict["ProductVersionId"]))
    print("ProductLineCanonicalName [%s]"%(g_dscrptn_dict["ProductLineCanonicalName"]))
    print("ProductLineVersionLabels(%d):"%(len(g_dscrptn_dict["ProductVersionLabels"])))
    for language_code in g_dscrptn_dict["ProductVersionLabels"].keys():
        print(" [%s][%s]"%(language_code,g_dscrptn_dict["ProductVersionLabels"][language_code]))
    print("ProductLineInstanceNames(%d):"%(len(g_dscrptn_dict["ProductInstanceNames"])))
    for language_code in g_dscrptn_dict["ProductInstanceNames"].keys():
        print(" [%s][%s]"%(language_code,g_dscrptn_dict["ProductInstanceNames"][language_code]))
    print("EMLVersion               [%s]"%(g_dscrptn_dict["EMLVersion"]))
    print("Comment                  [%s]"%(g_dscrptn_dict["Comment"]))
    print("Form                     [%s]"%(g_dscrptn_dict["Form"]))
    print("Managed                  [%d]"%(g_dscrptn_dict["Managed"]))
    print("SignedManifest           [%s]"%(str(g_dscrptn_dict["SignedManifest"])))
    print("DefaultHomeFolder        [%s]"%(g_dscrptn_dict["DefaultHomeFolder"]))
    print("Features (%s):"%(len(g_dscrptn_dict["Features"])))
    for i in g_dscrptn_dict["Features"]:
        print(" [%s]"%(i))
    print("Platforms (%s):"%(len(g_dscrptn_dict["Features"])))
    for i in g_dscrptn_dict["Platforms"]:
        print(" [%s]"%(i))
    print("Roles (%s):"%(len(g_dscrptn_dict["Roles"])))
    for i in g_dscrptn_dict["Roles"]:
        print(" [%s]"%(i))
    print("TargetTypes (%s):"%(len(g_dscrptn_dict["TargetTypes"])))
    for i in g_dscrptn_dict["TargetTypes"]:
        print(" [%s]"%(i))
    print("SAVLine                  [%s]"%(g_dscrptn_dict["SAVLine"]))
    print("SAVVersion               [%s]"%(g_dscrptn_dict["SAVVersion"]))
    print("VirusDataVersion         [%s]"%(g_dscrptn_dict["VirusDataVersion"]))
    print("VirusEngineVersion       [%s]"%(g_dscrptn_dict["VirusEngineVersion"]))    


#-----------------------------------------------------------------------------
#LOG
#-----------------------------------------------------------------------------
    
def out_str(data,format=1):
    #Generates a representation of a string in which (by default)
    #non-printable characters are displayed as their hex ascii values.
    s = ""
    if format == 1:
        #Non-printable characters only shown as hex
        for ch in data:
            if (ord(ch) < 0x20) or (ord(ch) >= 0x7F):                
                s = s + "(%02x)"%(ord(ch))
            else:
                s = s + ch    
    elif format == 2:
        #All characters shown as hex
        for ch in data:
            s = s + "(%02x)"%(ord(ch))
    else:
        #Standard string
        s = data
    return s


def log(sev,domain,msg):
    """Log an event of a given severity"""
    global g_verbosity
    if sev <= g_verbosity:

        #Determine time for this event
        time_s = strftime("%Y-%m-%dT%H:%M:%SZ",gmtime())

        #Determine severity text
        sev_s = "INFO"
        if sev == g_log_CRITICAL:
            sev_s = "ERROR"            
        elif sev == g_log_ERROR:
            sev_s = "ERROR"
        elif sev == g_log_WARNING:
            sev_s = "WARNING"

        #Construct log-record and store it in log file
        if g_log_filepath:
            s = sev_s.ljust(8)
            log_record_s = "%s: %s: %s: %s\n"%(time_s,s,domain,out_str(msg))
            f = open(g_log_filepath,'a')
            f.write(log_record_s)
            f.close()

        #Construct output-message and display it
        if g_log_outstrm:
            out_s = "%s"%(out_str(msg))
            if sev < g_log_INFO:
                out_s = "%s: %s"%(sev_s,out_str(msg))
            print(out_s)
        

#-----------------------------------------------------------------------------
#Initialisation
#-----------------------------------------------------------------------------
def reset():
    """Sets global variables to initial values"""
    global g_prog_name
    global g_prog_ver
    global g_verbosity
    global g_dlvrbls_fldrpath
    global g_dscrptn_filepath
    global g_log_filepath
    global g_sdds_import_filepath
    global g_build_id
    global g_pretty_xml
    global g_line_name
    global g_default_language_code
    global g_dscrptn_dict
    global g_dlvrbls_info
    global g_log_CRITICAL
    global g_log_ERROR
    global g_log_WARNING
    global g_log_INFO
    global g_log_filepath
    global g_log_outstrm

    
    g_prog_name = "make SDDS import"
    g_prog_ver = "1.0"
    g_verbosity = 3

    g_dlvrbls_fldrpath = None
    g_dscrptn_filepath = None
    g_log_filepath = None
    g_sdds_import_filepath = None
    g_build_id = "0"
    g_pretty_xml = False
    g_line_name = None

    g_default_language_code = "en"

    g_dscrptn_dict = {
        "ProductLineId":None,                #string
        "ProductVersionId":None,            #string
        "ProductLineCanonicalName":None,    #string
        "ProductVersionLabels":{},            #{lang-code:string,...}
        "ProductInstanceNames":{},            #{lang-code:string,...}
        "EMLVersion":None,                    #string
        "Comment":None,                        #string
        "Form":None,                        #string
        "Managed":False,                    #bool
        "SignedManifest":0,                    #integer
        "DefaultHomeFolder":None,            #string
        "Features":[],                        #[string,...]
        "Platforms":[],                        #[string,...]
        "Roles":[],                            #[string,...]
        "TargetTypes":[],                    #[string,...]
        "SAVLine":None,                        #string
        "SAVVersion":None,                    #string
        "VirusDataVersion":None,            #string
        "VirusEngineVersion":None            #string
        }

    g_dlvrbls_info = []    #[(Name,Size,MD5,Offset),...]

    #For logging
    g_log_CRITICAL = 1
    g_log_ERROR = 2
    g_log_WARNING = 3
    g_log_INFO = 4
    g_log_filepath = None
    g_log_outstrm = 1


def read_command_line(argv):
    """Reads the command line"""
    parser = OptionParser(usage="%prog [options]",add_help_option=True)
    parser.add_option("-b", "--buildid", dest="build_id", default=None, help='Build identifier (default "0")')
    parser.add_option("-d", "--description", dest="dscrptn_filepath", default="./spv.xml", help="Path to file describing the deliverables (default ./spv.xml).")
    parser.add_option("-f", "--files", dest="dlvrbls_fldrpath", default=".", help="Path to folder containing the deliverables to be packaged (default .).")
    parser.add_option("-F", "--force", action="store_true", dest="force_flag", default=False, help="Allow description file to be with deliverables. Overwrite existing SDDS import file.")
    parser.add_option("-o", "--outfilename", dest="out_filename", default="SDDS-Import.xml", help="Name for the output file (default 'SDDS-Import.xml').")
    parser.add_option("-L", "--linename", dest="line_name", default=None, help="Include English line-name in dictionary element of SDDS import file (USE WITH CAUTION)")
    parser.add_option("-l", "--logfile", dest="log_filepath", default=None, help="Log file (default None).")
    parser.add_option("-p", "--pretty", action="store_true", dest="prettyxml_flag", default=False, help="Write SDDS import file with pretty format.")
    parser.add_option("-v", "--verbosity", dest="verbosity", default=2, help="0=None 1=CRITICAL ERRORS 2=ALL ERRORS 3=ERRORS+WARNINGS 4=ERRORS+WARNINGS+INFO (default 3).")
    #Add build-id
    (opts, args) = parser.parse_args(argv[1:])
    return opts    


def init(opts):
    """"Initialise the program"""
    global g_build_id
    global g_dlvrbls_fldrpath
    global g_dscrptn_filepath
    global g_line_name
    global g_log_filepath
    global g_pretty_xml
    global g_prog_name
    global g_prog_ver
    global g_sdds_import_filepath
    global g_verbosity

    #Set the verbosity
    g_verbosity = int(opts.verbosity)

    #Determine the specified deliverables folder
    g_dlvrbls_fldrpath = os.path.abspath(opts.dlvrbls_fldrpath)
    
    #Determine if it is OK to write the log file
    #(Set g_log_filepath to non-None if it is OK)
        # located in
        # deliverables  currently  OK to
        # folder?       exists?    write?
        # ---------------------------------------
        # NO            NO         YES
        # NO            YES        NO (may overwrite another file)
        # YES           NO         YES (to be ignored in listing)
        # YES           YES        NO (will overwrite a deliverable)
    g_log_filepath = None
    if opts.log_filepath:
        log_filepath = os.path.abspath(opts.log_filepath)
        if os.path.dirname(log_filepath) == g_dlvrbls_fldrpath:
            #Log file is specified as being in the deliverables folder
            if os.path.exists(log_filepath):
                #There exists a file in the deliverables folder with the same name as the
                #specified log file. This may very well be an actual deliverable. Therefore,
                #we cannot write the specified log file.
                raise Ex_General("Initialisation","Cannot write log file '%s'. File already exists (and is a deliverable)."%(log_filepath))
            else:
                #There is no item (file or folder) in the deliverables folder with the same
                #name as the specified log file. Therefore, it is OK to write this file. The
                #code that lists the deliverables folder shall ignore this file.
                g_log_filepath = log_filepath
        else:
            #Specified log file is not in the deliverables folder
            if os.path.exists(log_filepath):
                #A file already exists with the same path as the specified log file. This
                #might be an unrelated file. Therefore we cannot write the specified log file.
                raise Ex_General("Initialisation","Cannot write log file '%s'. File already exists (would be overwritten)."%(log_filepath))
            else:
                #There is no item (file or folder) with the same path as the specified log
                #file. It is safe to write the log file.
                g_log_filepath = log_filepath
                
    #Determine the program name and version
    g_prog_name = os.path.basename(sys.argv[0])
    log(g_log_INFO,"Initialisation","%s v%s"%(g_prog_name,g_prog_ver))

    #Report the log file being used (if any)
    log(g_log_INFO,"Initialisation","Log file = '%s'"%(g_log_filepath))
    
    #Determine build_id
    if not opts.build_id:
        log(g_log_WARNING,"Initialisation","No build id specified, using default ('%s')"%(g_build_id))
    else:
        g_build_id = opts.build_id
        log(g_log_INFO,"Initialisation","Build-id = '%s'"%(g_build_id))

    #Determine if the deliverables-folder exists
    if not os.path.isdir(g_dlvrbls_fldrpath):
        raise Ex_General("Initialisation","No such deliverables-folder '%s'"%(g_dlvrbls_fldrpath))
    log(g_log_INFO,"Initialisation","Deliverables-folder = '%s'"%(g_dlvrbls_fldrpath))

    #Determine if the description-file exists
    g_dscrptn_filepath = os.path.abspath(opts.dscrptn_filepath)
    if not os.path.isfile(g_dscrptn_filepath):
        raise Ex_General("Initialisation","No such description-file '%s'"%(g_dscrptn_filepath))
    if os.path.dirname(g_dscrptn_filepath) == g_dlvrbls_fldrpath:
        #The specified description file is in the deliverables folder
        if not opts.force_flag:
            raise Ex_General("Initialisation","Specified description file '%s' is in the deliverables folder"%(g_dscrptn_filepath))            
    log(g_log_INFO,"Initialisation","Description-file = '%s'"%(g_dscrptn_filepath))
    

    #Determine path to SDDS import file
    g_sdds_import_filepath = os.path.join(g_dlvrbls_fldrpath,opts.out_filename)
    if g_sdds_import_filepath == g_log_filepath:
        raise Ex_General("Initialisation","Output file and log file have same path '%s'"%(g_sdds_import_filepath))
    if os.path.exists(g_sdds_import_filepath):
        if not os.path.isfile(g_sdds_import_filepath):
            raise Ex_General("Initialisation","SDDS import file '%s' already exists and is not a file"%(g_sdds_import_filepath))
        if opts.force_flag:
            os.remove(g_sdds_import_filepath)
            log(g_log_INFO,"Initialisation","Forcibly removed SDDS import file '%s'"%(g_sdds_import_filepath))
        else:
            raise Ex_General("Initialisation","SDDS import file '%s' already exists"%(g_sdds_import_filepath))
    log(g_log_INFO,"Initialisation","SDDS import file will be written to '%s'"%(g_sdds_import_filepath))

    #Determine format for SDDS import file
    g_pretty_xml = opts.prettyxml_flag
    format_s = "standard"
    if g_pretty_xml:
        format_s = "pretty"
    log(g_log_INFO,"Initialisation","SDDS import file will be written in %s format"%(format_s))

    #Determine line-name, if any
    g_line_name = opts.line_name
    if g_line_name:
        log(g_log_INFO,"Initialisation","SDDS import file will include line-name '%s'"%(g_line_name))

#-----------------------------------------------------------------------------
#Product description file processing
#-----------------------------------------------------------------------------

def npjoin(base_np,child_tagname):
    return base_np + "/%s"%(child_tagname)


def get_attribute_value(nd,np,attribute_name,default_value=None):
    """Returns the value of of the specified attribute held by the specified node"""
    v = default_value

    found_value = False
    attrs = nd.attributes
    if len(attrs) > 0:
        if attribute_name in attrs:
            found_value = True
            v = attrs[attribute_name].value

    if (not found_value) and (default_value == None):
        raise Ex_General("Product description file","Missing attribute '%s' from node '%s'"%(attribute_name,np))

    return v    


def get_text_value(nd,np,default_value=None):
    """Returns the value of the text held by the specified element node"""
    v = default_value
    
    child_nds = nd.childNodes
    if len(child_nds) > 0:
        for child_nd in child_nds:
            if child_nd.nodeType == child_nd.TEXT_NODE:
                v = child_nd.data
                break

    return v    
    

def get_simple_element_values(parent_nd,parent_np,tagname,min=-1,max=-1):
    """Returns the values of the specified simple elements"""
    #NB: In this context, "simple element" means '<tagname>Text</tagname>'
    
    #Initialise return value
    vs = []

    #Find the simple elements, if any, and read their values
    child_nds = get_elements_by_tag_name(parent_nd,parent_np,tagname,min,max)
    count = 0
    for child_nd in child_nds:
        count = count + 1
        child_nd_name = child_nd.tagName
        v = get_text_value(child_nd,npjoin(parent_np,"%s#%d"%(child_nd_name,count)))
        if v:
            vs.append(v)

    #Return values
    return vs


def get_simple_list_value(parent_nd,parent_np,listname,itemname,min=-1):
    """Returns the value (a list) of the specified simple list element (if any)"""
    #NB: In this context:
    # simple-list-element := '<listname>ITEMS</listname>'
    # ITEMS := ITEM*
    # ITEM := '<itemname>Text</itemname>'

    #Initialise return value
    vs = []
    
    #Find the list element. (For a simple list, we assume that there is at most 1)
    list_nds = get_elements_by_tag_name(parent_nd,parent_np,listname,min,1)
    num = len(list_nds)
    if min >= 0:
        if num < min:
            raise Ex_General("Product description file","Too few list nodes '%s' in parent node '%s' (actual: %d; expected at least: %d)"%(listname,parent_np,num,min))
    if num > 1:
        raise Ex_General("Product description file","Too many list nodes '%s' in parent node '%s' (actual: %d; expected at most: %d)"%(listname,parent_np,num,1))

    #If the list exists, get the label values that it contains
    if num:
        list_nd = list_nds[0]
        list_np = npjoin(parent_np,list_nd.tagName)
        vs = get_simple_element_values(list_nd,list_np,itemname,1)

    #Return values
    return vs


def get_label_element_values(parent_nd,parent_np,tagname,min=-1,max=-1):
    """Returns the values of the specified label elements"""
    #NB: In this context, "label element" means '<tagname lang="Code">Text</tagname>'

    #Initialise return value
    vs = {}

    #Find the label elements, if any, and read their values
    child_nds = get_elements_by_tag_name(parent_nd,parent_np,tagname,min,max)
    count = 0
    for child_nd in child_nds:
        count = count + 1
        child_np = npjoin(parent_np,"%s#%d"%(child_nd.tagName,count))
        language_code = get_attribute_value(child_nd,child_np,"lang","en")        
        label_text = get_text_value(child_nd,child_np)
        if label_text:
            if not (language_code in vs.keys()):
                #We deliberately only take the first element for any particular language
                #(There shouldn't be more than one, but just in case...)
                vs[language_code]=label_text

    #Return values
    return vs


def get_label_list_value(parent_nd,parent_np,listname,itemname):
    """Returns the value (a dictionary) of the specified label list element"""
    #NB: In this context:
    # label-list-element := '<listname>ITEMS</listname>'
    # ITEMS := ITEM*
    # ITEM := '<itemname [lang="Code"]>Text</itemname>'

    #Find the list element. (For a label list, we assume that there is *exactly* 1)
    list_nds = get_elements_by_tag_name(parent_nd,parent_np,listname,1,1)
    num = len(list_nds)
    if num < 1:
            raise Ex_General("Product description file","Too few list nodes '%s' in parent node '%s' (actual: %d; expected at least: %d)"%(listname,parent_np,num,1))
    if num > 1:
            raise Ex_General("Product description file","Too many list nodes '%s' in parent node '%s' (actual: %d; expected at most: %d)"%(listname,parent_np,num,1))

    #If the list exists, get the label values that it contains
    list_nd = list_nds[0]
    list_np = npjoin(parent_np,list_nd.tagName)
    vs = get_label_element_values(list_nd,list_np,itemname,1)

    #Check that the label list to be returned contains at least one element for the default language
    if not g_default_language_code in vs:
        raise Ex_General("Product description file","Missing label entry for default language '%s' in parent node '%s'"%(g_default_language_code,list_np))

    #Return values
    return vs


def get_elements_by_tag_name(parent_nd,parent_np,tagname,min=-1,max=-1):
    """Returns all nodes with the specified tagname that are included within the specified parent node"""
    nds = parent_nd.getElementsByTagName(tagname)
    num = len(nds)
    if (min >= 0):
        if num < min:
            raise Ex_General("Product description file","Too few nodes named '%s' in parent node '%s' (actual: %d; expected at least: %d)"%(tagname,parent_np,num,min))
    if (max >= 0):
        if num > max:
            raise Ex_General("Product description file","Too many nodes named '%s' in parent node '%s' (actual: %d; expected at most: %d)"%(tagname,parent_np,num,min))
    return nds        

    
def read_product_instance_data(nd,np):
    """Read a product-instance node (putting data into product description dictionary)"""
    global g_dscrptn_dict

    #Get the product-instance node version identifier (currently unused)
    pi_node_format_version = get_attribute_value(nd,np,"version","1")
    log(g_log_INFO+1,"Product description file","Product-instance node format = '%s' (source: '%s')"%(pi_node_format_version,np))

    #Get primary product-instance identity
    g_dscrptn_dict["ProductLineId"] = get_simple_element_values(nd,np,"ProductLineId",1,1)[0]
    log(g_log_INFO+1,"Product description file","Product-line-id = '%s' (source: '%s')"%(g_dscrptn_dict["ProductLineId"],npjoin(np,"ProductLineId")))
    
    g_dscrptn_dict["ProductVersionId"] = get_simple_element_values(nd,np,"ProductVersionId",1,1)[0]
    log(g_log_INFO+1,"Product description file","Product-version-id = '%s' (source: '%s')"%(g_dscrptn_dict["ProductVersionId"],npjoin(np,"ProductVersionId")))
    
    #Get secondary product-instance identity    
    g_dscrptn_dict["ProductLineCanonicalName"] = get_simple_element_values(nd,np,"ProductLineCanonicalName",1,1)[0]
    log(g_log_INFO+1,"Product description file","Product-line-canonical-name = '%s' (source: '%s')"%(g_dscrptn_dict["ProductLineCanonicalName"],npjoin(np,"ProductLineCanonicalName")))

    g_dscrptn_dict["ProductVersionLabels"] = get_label_list_value(nd,np,"ProductVersionLabels","Label")    
    log(g_log_INFO+1,"Product description file","Found %d product-version-labels (source: '%s')"%(len(g_dscrptn_dict["ProductVersionLabels"]),npjoin(np,"ProductVersionLabels")))

    g_dscrptn_dict["ProductInstanceNames"] = get_label_list_value(nd,np,"ProductInstanceNames","Name")    
    log(g_log_INFO,"Product description file","Found %d product-instance-names (source: '%s')"%(len(g_dscrptn_dict["ProductInstanceNames"]),npjoin(np,"ProductInstanceNames")))

    vals = get_simple_element_values(nd,np,"EMLVersion",0,1)
    if len(vals) > 0:
        g_dscrptn_dict["EMLVersion"] = vals[0]
    log(g_log_INFO+1,"Product description file","EML-version-label = '%s' (source: '%s')"%(g_dscrptn_dict["EMLVersion"],npjoin(np,"EMLVersion")))

    vals = get_simple_element_values(nd,np,"Comment",0,1)
    if len(vals) > 0:
        g_dscrptn_dict["Comment"] = vals[0]
    log(g_log_INFO+1,"Product description file","Comment = '%s' (source: '%s')"%(g_dscrptn_dict["Comment"],npjoin(np,"Comment")))

    #Get product-instance form    
    g_dscrptn_dict["Form"] = get_simple_element_values(nd,np,"Form",1,1)[0]
    log(g_log_INFO+1,"Product description file","Product form = '%s' (source: '%s')"%(g_dscrptn_dict["Form"],npjoin(np,"Form")))

    managed_flag = int(get_simple_element_values(nd,np,"Managed",1,1)[0])
    if managed_flag:
        g_dscrptn_dict["Managed"] = True
    else:
        g_dscrptn_dict["Managed"] = False    
    log(g_log_INFO+1,"Product description file","Managed = %s (source: '%s')"%(str(g_dscrptn_dict["Managed"]),npjoin(np,"Managed")))
    
    g_dscrptn_dict["SignedManifest"] = int(get_simple_element_values(nd,np,"SignedManifest",1,1)[0])
    log(g_log_INFO+1,"Product description file","Signed manifest = %d (source: '%s')"%(g_dscrptn_dict["Managed"],npjoin(np,"SignedManifest")))

    #Get product-instance details
    default_home_folder_vals = get_simple_element_values(nd,np,"DefaultHomeFolder",0,1)
    if len(default_home_folder_vals) > 0:
        g_dscrptn_dict["DefaultHomeFolder"] = default_home_folder_vals [0]
    log(g_log_INFO+1,"Product description file","Default-home-folder = %s (source: '%s')"%(g_dscrptn_dict["DefaultHomeFolder"],npjoin(np,"DefaultHomeFolder")))

    g_dscrptn_dict["Features"] = get_simple_list_value(nd,np,"Features","Feature",0)
    log(g_log_INFO+1,"Product description file","Found %d features (source: '%s')"%(len(g_dscrptn_dict["Features"]),npjoin(np,"Features")))

    g_dscrptn_dict["Platforms"] = get_simple_list_value(nd,np,"Platforms","Platform",0)
    log(g_log_INFO+1,"Product description file","Found %d platforms (source: '%s')"%(len(g_dscrptn_dict["Platforms"]),npjoin(np,"Platforms")))

    g_dscrptn_dict["Roles"] = get_simple_list_value(nd,np,"Roles","Role",0)
    log(g_log_INFO+1,"Product description file","Found %d roles (source: '%s')"%(len(g_dscrptn_dict["Roles"]),npjoin(np,"Roles")))

    g_dscrptn_dict["TargetTypes"] = get_simple_list_value(nd,np,"TargetTypes","TargetType",1)
    log(g_log_INFO+1,"Product description file","Found %d target-types (source: '%s')"%(len(g_dscrptn_dict["TargetTypes"]),npjoin(np,"TargetTypes")))
    
    vals = get_simple_element_values(nd,np,"SAVLine",0,1)
    if len(vals) > 0:
        g_dscrptn_dict["SAVLine"] = vals[0]
    log(g_log_INFO+1,"Product description file","SAV-line = %s (source: '%s')"%(g_dscrptn_dict["SAVLine"],npjoin(np,"SAVLine")))
        
    vals = get_simple_element_values(nd,np,"SAVVersion",0,1)
    if len(vals) > 0:
        g_dscrptn_dict["SAVVersion"] = vals[0]
    log(g_log_INFO+1,"Product description file","SAV-version = %s (source: '%s')"%(g_dscrptn_dict["SAVVersion"],npjoin(np,"SAVVersion")))    

    vals = get_simple_element_values(nd,np,"VirusDataVersion",0,1)
    if len(vals) > 0:
        g_dscrptn_dict["VirusDataVersion"] = vals[0]
    log(g_log_INFO+1,"Product description file","Virus-data-version = %s (source: '%s')"%(g_dscrptn_dict["VirusDataVersion"],npjoin(np,"VirusDataVersion")))

    vals = get_simple_element_values(nd,np,"VirusEngineVersion",0,1)
    if len(vals) > 0:
        g_dscrptn_dict["VirusEngineVersion"] = vals[0]
    log(g_log_INFO+1,"Product description file","virus-engine-version = %s (source: '%s')"%(g_dscrptn_dict["VirusEngineVersion"],npjoin(np,"VirusEngineVersion")))    
    

def read_sdds_data(nd,np):
    """Read an sdds-data node (processes the product-instance nodes therein)"""
    
    #Get the SDDS node version identifier (currently unused)
    sdds_node_format_version = get_attribute_value(nd,np,"version","1")
    log(g_log_INFO+1,"Product description file","SDDS-data node format = '%s' (source: '%s')"%(sdds_node_format_version,np))

    #Find the product-instance description (ought to be one only)
    pi_nd = get_elements_by_tag_name(nd,np,"ProductInstance",1,1)[0]
    log(g_log_INFO+1,"Product description file","Found product-instance node '%s' (source: '%s')"%(pi_nd.tagName,npjoin(np,"ProductInstance")))

    #Process the product-instance description
    read_product_instance_data(pi_nd,npjoin(np,pi_nd.tagName))
                
    
def read_product_description_file():
    """Reads the product description file into the product description dictionary"""
    global g_dscrptn_filepath
    global g_dscrptn_dict

    #Report started reading product description file
    log(g_log_INFO,"Product description file","Reading product description file...")
    
    #Read the data from the product description file
    f = open(g_dscrptn_filepath,"rb")
    doc_nd = minidom.parse(f)
    f.close()

    #Identify the root-node
    np = "."
    root_nd = get_elements_by_tag_name(doc_nd,np,"SDDSData",1,1)[0]
    root_nd_name = root_nd.tagName
    log(g_log_INFO+1,"Product description file","Found root node '%s' (source: '%s')"%(root_nd_name,npjoin(np,"SDDSData")))

    #Process the root-node
    read_sdds_data(root_nd,npjoin(np,root_nd_name))

    #Report finished reading product description file
    log(g_log_INFO,"Product description file","...Product description file read")


#-----------------------------------------------------------------------------
#Deliverables folder processing
#-----------------------------------------------------------------------------

def read_deliverables_folder():
    """Read the deliverables folder and store file details in file list"""
    global g_dscrptn_filepath
    global g_dlvrbls_fldrpath
    global g_dlvrbls_info
    global g_log_filepath

    #Determine files to exclude from listing 
    excl_filepaths = []
    if g_log_filepath:
        #A non-'None' value for g_log_filepath means that we are writing this file;
        #it is *not* a deliverable, and should be excluded from the listing.
        excl_filepaths.append(g_log_filepath)
    if os.path.dirname(g_dscrptn_filepath) == g_dlvrbls_fldrpath:
        #The specified description file is in the deliverables folder; it is *not*
        #a file to be delivered, and should be excluded from the listing.
        excl_filepaths.append(g_dscrptn_filepath)

    #Determine and store files information
    rel_fldrpath = ''        
    for (fldrpath,subfldrnames,filenames) in os.walk(g_dlvrbls_fldrpath):
        rel_fldrpath = (fldrpath[len(g_dlvrbls_fldrpath)+1:]).replace("\\","/")
        for filename in filenames:
            filepath = os.path.join(fldrpath,filename)
            if not (filepath in excl_filepaths):
                #Determine the file contents (data)
                f = open(filepath,"rb")
                d = f.read()
                f.close()
                #Determine the file size (as decimal string)
                size_s = "%d"%(len(d))    #Hopefully, always the size in bytes !
                #Determine the file MD5 checksum (as hex string)
                md5 = hashlib.md5()
                md5.update(d)
                csum_s = md5.hexdigest()
                #Add info
                g_dlvrbls_info.append((filename,size_s,csum_s,rel_fldrpath))



#-----------------------------------------------------------------------------
#Write SDDS Import file
#-----------------------------------------------------------------------------

def get_product_type():
    """Returns product-type enumeration value"""
    global g_dscrptn_dict
    product_type = None
    if g_dscrptn_dict["Form"] == "compound":
        product_type = "Component Suite"
    elif g_dscrptn_dict["Form"] == "simple":
        product_type = "Component"
    if not product_type:
        raise Ex_General("SDDS import file","Unable to determine product-instance type from form '%s'"%(g_dscrptn_dict["Form"]))
    return product_type


def get_name_texts():
    """Returns dictionary containing short and long name texts in each language"""
    global g_dscrptn_dict
    texts = dict()    #texts={'lang-code':{'short':version-label,'long':instance-name},...}
    #Obtain raw data
    version_labels = g_dscrptn_dict["ProductVersionLabels"]
    instance_names = g_dscrptn_dict["ProductInstanceNames"]
    #Add all version labels to texts dictionary, with corresponding instance names.
        #Use the instance name in the default language if one does not exist in the required language
    for lang_code in version_labels.keys():
        if not (lang_code in texts.keys()):
            texts[lang_code] = dict()
            texts[lang_code]["short"]=version_labels[lang_code]
            if lang_code in instance_names.keys():
                texts[lang_code]["long"]=instance_names[lang_code]
            else:
                texts[lang_code]["long"]=instance_names[g_default_language_code]
    #Add remaining instance names (if any) to texts dictionary.
        #Use version label in the default language (by definition, one will not exist in the required language)
    for lang_code in instance_names.keys():
        if not (lang_code in texts.keys()):
            texts[lang_code] = dict()
            texts[lang_code]["long"]=instance_names[lang_code]
            texts[lang_code]["short"]=version_labels[g_default_language_code]
    #Return collated texts
    return texts

    
def write_element(doc,parent_nd,tagname,text=None,attributes={}):
    if (text or (len(attributes.keys()) > 0)):
        elem_nd = doc.createElement(tagname)
        for n in attributes.keys():
            elem_nd.setAttribute(n,attributes[n])
        if text:
            text_nd = doc.createTextNode(text)
            elem_nd.appendChild(text_nd)
        parent_nd.appendChild(elem_nd)


def write_simple_list(doc,parent_nd,listname,itemname,vals=[]):
    list_nd = doc.createElement(listname)
    for val in vals:
        write_element(doc,list_nd,itemname,None,{"Name":val,})
    parent_nd.appendChild(list_nd)


def write_file_list(doc,parent_nd,listname,itemname,vals=[]):
    """Write list of file data"""
    #NB: Can't write XML attributes in any specified order, because Python
    #dictionaries are inherently unordered. But this shouldn't matter.
    list_nd = doc.createElement(listname)
    for val in vals:
        #Generate XML attributes list
        attrs = {"Name":val[0],"Size":val[1],"MD5":val[2]}
        if val[3] and len(val[3]) > 0:
            attrs["Offset"] = val[3]        
        #Generate list item
        write_element(doc,list_nd,itemname,None,attrs)
    parent_nd.appendChild(list_nd)    


def write_names_list(doc,parent_nd):
    """Write-out names list"""
    #NB: In this context:
    # names-list := '<Language lang="lang-code">TEXTS</Language>'*
    # TEXTS := '<LongDesc>INSTANCE-NAME</LongDesc><ShortDesc>VERSION-LABEL</ShortDesc>'
    texts = get_name_texts()
    lang_codes = list(texts.keys())
    lang_codes.sort()
    for lang_code in lang_codes:
        lang_nd = doc.createElement("Language")
        lang_nd.setAttribute("lang",lang_code)
        write_element(doc,lang_nd,"ShortDesc",texts[lang_code]["short"])
        write_element(doc,lang_nd,"LongDesc",texts[lang_code]["long"])
        parent_nd.appendChild(lang_nd)
        
    
def write_sdds_import_file():
    """x"""
    global g_build_id
    global g_dlvrbls_info
    global g_dscrptn_dict
    global g_line_node    
    global g_pretty_xml
    global g_sdds_import_filepath

    #Report started writing SDDS import file file
    log(g_log_INFO,"SDDS import file","Writing SDDS import file...")
    
    #Create XML document and find its root node
    impl = minidom.getDOMImplementation()
    doc = impl.createDocument(None, "ComponentData", None)
    root_nd = doc.documentElement

    #Generate component node
    component_nd = doc.createElement("Component")
    write_element(doc,component_nd,"Name",g_dscrptn_dict["ProductLineCanonicalName"])    
    write_element(doc,component_nd,"RigidName",g_dscrptn_dict["ProductLineId"])
    write_element(doc,component_nd,"Version",g_dscrptn_dict["ProductVersionId"])
    write_element(doc,component_nd,"Build",g_build_id)
    write_element(doc,component_nd,"ProductType",get_product_type())
    write_element(doc,component_nd,"DefaultHomeFolder",g_dscrptn_dict["DefaultHomeFolder"])
    write_simple_list(doc,component_nd,"TargetTypes","TargetType",g_dscrptn_dict["TargetTypes"])    
    write_simple_list(doc,component_nd,"Roles","Role",g_dscrptn_dict["Roles"])
    write_simple_list(doc,component_nd,"Platforms","Platform",g_dscrptn_dict["Platforms"])
    write_simple_list(doc,component_nd,"Features","Feature",g_dscrptn_dict["Features"])
    write_element(doc,component_nd,"EMLVersion",g_dscrptn_dict["EMLVersion"])
    write_element(doc,component_nd,"SAVLine",g_dscrptn_dict["SAVLine"])
    write_element(doc,component_nd,"SAVVersion",g_dscrptn_dict["SAVVersion"])
    write_element(doc,component_nd,"VirusDataVersion",g_dscrptn_dict["VirusDataVersion"])
    write_element(doc,component_nd,"VirusEngineVersion",g_dscrptn_dict["VirusEngineVersion"])
    write_file_list(doc,component_nd,"FileList","File",g_dlvrbls_info)
    #...BEGIN Build value hack (until Badger can cope with alphanumeric string, build-id included in Comment)
    comment_s = "[%s]"%(g_build_id)
    if not g_dscrptn_dict["Comment"] == None:
        if len(g_dscrptn_dict["Comment"]) > 0:
            comment_s = comment_s + " %s"%(g_dscrptn_dict["Comment"])
    write_element(doc,component_nd,"Comment",comment_s)
    #...END Build value hack
    root_nd.appendChild(component_nd)    
    
    #Generate dictionary node
    dictionary_nd = doc.createElement("Dictionary")
    
    name_nd = doc.createElement("Name")
    label_nd = doc.createElement("Label")
    write_element(doc,label_nd,"Token","%s#%s"%(g_dscrptn_dict["ProductLineId"],g_dscrptn_dict["ProductVersionId"]))
    write_names_list(doc,label_nd)
    name_nd.appendChild(label_nd)
    dictionary_nd.appendChild(name_nd)

    if g_line_name:
        line_nd = doc.createElement("Line")
        label_nd = doc.createElement("Label")
        write_element(doc,label_nd,"Token","%s"%(g_dscrptn_dict["ProductLineId"]))
        lang_nd = doc.createElement("Language")
        lang_nd.setAttribute("lang","en")
        write_element(doc,lang_nd,"ShortDesc",g_line_name)
        write_element(doc,lang_nd,"LongDesc",g_line_name)
        label_nd.appendChild(lang_nd)
        line_nd.appendChild(label_nd)
        dictionary_nd.appendChild(line_nd)
    
    root_nd.appendChild(dictionary_nd)

    #Generate XML
    if os.path.exists(g_sdds_import_filepath):
        #Just incase one has magically appeared since initialisation check
        raise Ex_General("SDDS import file","SDDS import file '%s' already exists"%(g_sdds_import_filepath))
    f = open(g_sdds_import_filepath,"wb")
    if g_pretty_xml:
        f.write(doc.toprettyxml('\t','\r\n','utf-8'))
    else:
        f.write(doc.toxml("utf-8"))
    f.close()

    #Report finished writing SDDS import file file    
    log(g_log_INFO,"SDDS import file","...SDDS import file written")    
    log(g_log_INFO,"SDDS import file","SDDS import file written to '%s'"%(g_sdds_import_filepath))
    
#=============================================================================
#Main
#=============================================================================
def run(opts):
    """Entry point for script when used as a Python library"""

    #Reset global variables    
    reset()
    
    #Determine start time
    start_tm = strftime("%Y-%m-%dT%H:%M:%SZ",gmtime())

    #Initialise
    init(opts)

    #Read the product description file
    read_product_description_file()

    #Read the deliverables folder
    read_deliverables_folder()

    #Write the import file
    write_sdds_import_file()


def main(argv):
    """Entry point for script when run as command-line utility"""

    try:
        
        #Read the command-line
        opts = read_command_line(argv)

        #Execute the script
        run(opts)

        #Return success
        return 0

    except Ex_General as err:
        log(g_log_CRITICAL,"EX",str(err))
        return -1
    except IOError as err:
        log(g_log_CRITICAL,"IO",str(err))
        return -1
    

if __name__ == "__main__" :
    sys.exit(main(sys.argv))        