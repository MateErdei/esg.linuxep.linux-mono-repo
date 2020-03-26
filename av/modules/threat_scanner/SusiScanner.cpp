/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "SusiScanner.h"

#include <string>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>

using namespace susi_scanner;

scan_messages::ScanResponse
SusiScanner::scan(datatypes::AutoFd& fd, const std::string& file_path)
{
    char buffer[512];

    // Test reading the file
    ssize_t bytesRead = read(fd.get(), buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = 0;
    std::cout << "Read " << bytesRead << " from " << file_path << '\n';

    // Test stat the file
    struct stat statbuf = {};
    ::fstat(fd.get(), &statbuf);
    std::cout << "size:" << statbuf.st_size << '\n';

    scan_messages::ScanResponse response;
    std::string contents(buffer);
    bool clean = (contents.find("EICAR") == std::string::npos);
    response.setClean(clean);
    if (!clean)
    {
        response.setThreatName("EICAR");
    }

    return response;
}
