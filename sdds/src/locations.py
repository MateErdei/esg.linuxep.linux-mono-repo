import os
import os.path

cwd = os.getcwd()
LOGS = os.path.join(cwd, "..", "logs")
WAREHOUSE_WRITE = os.path.join(cwd, "..", "output", "warehouse")
CUSTOMER_FILES_WRITE = os.path.join(cwd, "..", "output", "customer")
print("JAKE LOOK HERE")
print(os.environ)
print("JAKE LOOK HERE")
branch = os.environ.get("PIPELINE_NAME")
WAREHOUSE_READ  = ["http://ostia.eng.sophos/dev/sspl-warehouse/{}/warehouse/warehouse".format(branch)]
REPOSITORY_ID = "ES+C"
