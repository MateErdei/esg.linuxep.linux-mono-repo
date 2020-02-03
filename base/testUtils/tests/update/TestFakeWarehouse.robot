*** Settings ***
Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/UpdateServer.py
Library     ${libs_directory}/LogUtils.py

Default Tags    MANUAL
Test Timeout    NONE
*** Test Case ***
# Generates a warehouse and serves it over HTTPS

Generate Warehouse
    Generate Install File In Directory    ./tmp/TestInstallFiles/
	Generate Warehouse    ./tmp/TestInstallFiles/    ./tmp/temp_warehouse/    ${BASE_RIGID_NAME}
	Start Update Server    1233    ./tmp/temp_warehouse/customer_files/
	Start Update Server    1234    ./tmp/temp_warehouse/warehouses/sophosmain/
	Can Curl Url    https://localhost:1234/catalogue/sdds.live.xml
	Can Curl Url    https://localhost:1233
