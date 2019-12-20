import psutil


def test_edr_plugin_starts_osquery(sspl_mock, edr_plugin_instance):
    edr_plugin_instance.start_edr()
    assert "osqueryd" in (p.name() for p in psutil.process_iter())
        
    
    



########TODO Add tests for all the failure and restart cases for osquery and the WD

