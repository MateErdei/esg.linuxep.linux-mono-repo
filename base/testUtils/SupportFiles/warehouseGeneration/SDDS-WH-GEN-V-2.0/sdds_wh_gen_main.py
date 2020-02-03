import sdds_wh_gen_proc_wh_dscr as proc_wh_dscr
import sdds_wh_gen_proc_imports as proc_imports
import sdds_wh_gen_proc_dic as proc_dic
import sdds_wh_gen_publish as publish
import sdds_wh_gen_log as log
import sys
import os


def checkArgs( args, logger ):
    if not os.path.isdir( args[1] ) or not os.path.exists( args[1] ):
        logger.errorMessage( "Warehouse target: '%s' is invalid" % ( args[1] ) )
        return False

    if os.path.isdir( args[3] ) or not os.path.exists( args[3] ):
        logger.errorMessage( "Warehouse description XML file path: '%s' is invalid" % ( args[3] ) )
        return False

    return True

def processWarehouseDescription( wh_dscr_path, logger ):
    process_wh_dscr = proc_wh_dscr.ProcessWarehouseDescription( wh_dscr_path, logger )

    process_wh_dscr._load()
    assert process_wh_dscr.error_state == 0, "Failed to load / parse XML"

    process_wh_dscr.parse()

    if process_wh_dscr.error_state == 0:
        logger.statusMessage( "processWarehouseDescription() succeeded" )
    else:
        logger.errorMessage( "processWarehouseDescription() failed" )
        sys.exit( 1 )

    return process_wh_dscr

def importSDDSImportFiles( li_product_instances, logger ):
    process = proc_imports.ProcessSDDSImports( li_product_instances, logger )

    process.processImports()

    if process.error_state == 0:
        logger.statusMessage( "importSDDSImportFiles() succeeded" )
    else:
        logger.errorMessage( "importSDDSImportFiles() failed" )
        sys.exit( 1 )

    return process.error_state, process.li_product_instances, process.li_child_objects

def importDictionary( li_dictionary_paths, logger ):
    process_dics = proc_dic.ProcessDictionaries( li_dictionary_paths, logger )

    process_dics.processDics()

    if process_dics.error_state == 0:
        logger.statusMessage( "importDictionary() succeeded" )
    else:
        logger.errorMessage( "importDictionary() failed" )
        sys.exit( 1 )

    return process_dics.error_state, process_dics.dictionary_set

def publishWarehouse( warehouse_description, logger ):

    publish_warehouse = publish.Publish( warehouse_description, logger )

    publish_warehouse.beginPublishing()

    if publish_warehouse.error_state == 0:
        logger.statusMessage( "publishWarehouse() succeeded" )
    else:
        logger.errorMessage( "publishWarehouse() failed" )
        sys.exit( 1 )

def main( args ):
    logger = log.Logger()

    wh_path = None
    wh_TLC = None
    wh_dscr_path = None

    if len( args ) != 4:
        logger.errorMessage( "Invalid number of arguments" )
        return 2

    if not checkArgs( args, logger ):
        return 1
    else:
        wh_path = args[1] # arg 1 = warehouse path
        wh_TLC = args[2] # arg 2 = top level catalogue name
        wh_dscr_path = args[3] # arg 3 = warehouse description XML file

    assert os.path.isfile(wh_dscr_path)

    warehouse_description = processWarehouseDescription( wh_dscr_path, logger )
    li_product_instances, li_dictionary_paths = warehouse_description.li_product_instances, warehouse_description.li_dictionary_paths

    error_state = 1
    if li_product_instances and li_dictionary_paths:
        error_state, li_product_instances, li_child_objects = importSDDSImportFiles( li_product_instances, logger )

    if error_state == 0:
        error_state, dictionary_set = importDictionary(li_dictionary_paths, logger)

    if error_state == 0:
        warehouse_description.wh_path = wh_path
        warehouse_description.wh_TLC = wh_TLC
        warehouse_description.dictionary_set = dictionary_set
        warehouse_description.li_product_instances = li_product_instances
        warehouse_description.li_child_objects = li_child_objects

        publishWarehouse( warehouse_description, logger )
    return 0

if __name__ == "__main__":
    sys.exit(main( sys.argv ))
