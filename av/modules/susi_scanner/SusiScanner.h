/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <scan_messages/ScanResponse.h>
#include <datatypes/AutoFd.h>
namespace susi_scanner
{
    class SusiScanner
    {
    public: scan_messages::ScanResponse scan(datatypes::AutoFd& fd, const std::string& file_path);
    };
}
