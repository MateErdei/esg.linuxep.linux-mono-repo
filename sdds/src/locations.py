import os
import os.path

cwd = os.getcwd()
LOGS = os.path.join(cwd, "..", "logs")
WAREHOUSE_WRITE = os.path.join(cwd, "..", "output", "warehouse")
CUSTOMER_FILES_WRITE = os.path.join(cwd, "..", "output", "customer")
WAREHOUSE_READ  = ["http://ostia.eng.sophos/dev/%(codeline)s/%(buildid)s/warehouse"]
REPOSITORY_ID = "ES+C"
