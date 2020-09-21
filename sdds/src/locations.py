import os
import os.path

cwd = os.getcwd()
LOGS = os.path.join(cwd, "..", "logs")
WAREHOUSE_WRITE = os.path.join(cwd, "..", "output", "warehouse")
CUSTOMER_FILES_WRITE = os.path.join(cwd, "..", "output", "customer")
branch = os.environ.get("SOURCE_CODE_BRANCH").replace("/","-")
WAREHOUSE_READ  = ["http://ostia.eng.sophos/dev/sspl-warehouse/{}/warehouse/warehouse".format(branch)]
REPOSITORY_ID = "ES+C"
