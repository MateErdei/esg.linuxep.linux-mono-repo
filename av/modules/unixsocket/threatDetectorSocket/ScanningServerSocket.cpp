/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include "../Logger.h"

#include "SigURS1Monitor.h"

namespace
{
    class Reloader : public unixsocket::IReloadable
    {
    public:
        explicit Reloader(threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
            : m_scannerFactory(std::move(scannerFactory))
        {}
        void reload() override
        {
            // TODO: LINUXDAR-2365 Implement SUSI hot-reloading
            LOGFATAL("Forcing SUSI Reload: TODO hot-reloading");
            exit(0); // Actually reachable except during fuzzing...
        }
    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;

    };
}



unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path, const mode_t mode,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory)
        : ImplServerSocket<ScanningServerConnectionThread>(path, mode,
                                                     std::make_shared<SigURS1Monitor>(std::make_shared<Reloader>(scannerFactory))),
                m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to create ScanningServerSocket without scanner factory");
    }
}
unixsocket::ScanningServerSocket::~ScanningServerSocket()
{
    // Need to do this before our members are destroyed
    requestStop();
    join();
}
