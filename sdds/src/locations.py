import os
import os.path

cwd = os.getcwd()
LOGS = os.path.join(cwd, "..", "logs")
WAREHOUSE_WRITE = os.path.join(cwd, "..", "output", "warehouse", "content")
CUSTOMER_FILES_WRITE = os.path.join(cwd, "..", "output", "warehouse")

branch = os.environ.get("PIPELINE_NAME") # The mode/name from pipeline.py

# Map the mode/name (as specified in pipeline.py)
# to the URL expected by everest-base/testUtils/system-product-test-release-package.xml
branch = {
    "release-package" : "develop",
    "release-package-edr-999" : "edr-999",
    "release-package-mdr-999" : "mdr-999",
    "release-package-edr-mdr-999" : "edr-mdr-999",
}.get(branch, branch)

WAREHOUSE_READ = [
    "http://ostia.eng.sophos/dev/sspl-warehouse/{}/warehouse/warehouse".format(branch),
    "http://d1.sophosupd.net/update/"
]
REPOSITORY_ID = "ES+C"
