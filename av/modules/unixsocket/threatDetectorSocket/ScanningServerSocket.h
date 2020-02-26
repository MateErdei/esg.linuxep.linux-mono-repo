/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "unixsocket/BaseServerSocket.h"
#include "ScanningServerConnectionThread.h"


namespace unixsocket
{
    using ScanningServerSocket = ImplServerSocket<ScanningServerConnectionThread>;
}
