#default values are set by Central
# https://wiki.sophos.net/display/SophosCloud/EMP%3A+policy-mcs#EMP:policy-mcs-ExtendforMCSPushServer

def get_default_command_poll():
    return 20

def get_default_flags_poll():
    return 4*3600

def get_default_push_ping_timeout():
    return 90

def get_max_for_any_value():
    return 24*3600
