/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#ifndef SSPL_PLUGIN_MAV_RECONNECTSETTINGS_H
#define SSPL_PLUGIN_MAV_RECONNECTSETTINGS_H

#define TOTAL_MAX_RECONNECTS 600
/* Number of attempts for initial connection to socket, before scanning any file */
#define MAX_CONN_RETRIES 60

#ifdef USING_LIBFUZZER
#define MAX_SCAN_RETRIES 1
#else
/* Number of attempt to scan a particular file, reconnecting if required */
#define MAX_SCAN_RETRIES 60
#endif

#endif // SSPL_PLUGIN_MAV_RECONNECTSETTINGS_H
