import logging
def detect_failure(func):
    def wrapper_function(sspl_mock, edr_plugin_instance, caplog):
        try:
            caplog.set_level(logging.INFO)
            v = func(sspl_mock, edr_plugin_instance)
            return v
        except:
            edr_plugin_instance.set_failed()
            raise
    return wrapper_function

