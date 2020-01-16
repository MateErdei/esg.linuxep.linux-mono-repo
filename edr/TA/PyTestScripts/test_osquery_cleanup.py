import os
import time

def test_edr_plugin_purges_database_when_threshold_reached(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    i = 105
    time.sleep(3)
    while (i > 0):
        db_path = edr_plugin_instance.osquery_database_path()
        path = os.path.join(db_path, str(i))
        with open(path, 'a')as file:
            file.write("blah")
        i -= 1

    edr_plugin_instance.stop_edr()
    edr_plugin_instance.start_edr()

    assert edr_plugin_instance.wait_log_contains("Purging Database")
