//
// Created by pair on 13/01/2020.
//

#ifndef SSPL_PLUGIN_AV_SCANREQUEST_H
#define SSPL_PLUGIN_AV_SCANREQUEST_H

#include <ScanRequest.capnp.h>
#include <string>

namespace scan_messages
{
    class ScanRequest
    {
    public:
        ScanRequest();
        ScanRequest(int fd, Sophos::ssplav::FileScanRequest::Reader& requestMessage);
        ~ScanRequest();
        void resetRequest(int fd, Sophos::ssplav::FileScanRequest::Reader& requestMessage);

        int fd();
        std::string path();

    private:
        int m_fd;
        std::string m_path;
        void close();
        void setRequestFromMessage(Sophos::ssplav::FileScanRequest::Reader& requestMessage);
    };
}

#endif //SSPL_PLUGIN_AV_SCANREQUEST_H
