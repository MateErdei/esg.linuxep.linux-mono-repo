import os
import os.path
import locations

def make_empty_folder(folder):
    if not folder == '' and not os.path.exists(folder):
        os.makedirs(folder)

if __name__ == '__main__':
    make_empty_folder(locations.LOGS)
    make_empty_folder(locations.WAREHOUSE_WRITE)
    make_empty_folder(locations.CUSTOMER_FILES_WRITE)
