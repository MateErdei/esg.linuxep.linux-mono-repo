/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanningServerSocket.h"

#include "SigUSR1Monitor.h"

#include "../Logger.h"

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
            if (!m_scannerFactory->update())
            {
                throw std::runtime_error("Failed to update threat scanner");
            }
        }
    private:
        threat_scanner::IThreatScannerFactorySharedPtr m_scannerFactory;

    };
}

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path,
        mode_t mode,
        threat_scanner::IThreatScannerFactorySharedPtr scannerFactory,
        unixsocket::IMonitorablePtr monitorable)
        : ImplServerSocket<ScanningServerConnectionThread>(path, mode, std::move(monitorable)),
          m_scannerFactory(std::move(scannerFactory))
{
    if (m_scannerFactory.get() == nullptr)
    {
        throw std::runtime_error("Attempting to create ScanningServerSocket without scanner factory");
    }
}

unixsocket::ScanningServerSocket::ScanningServerSocket(
        const std::string& path, const mode_t mode,
        const threat_scanner::IThreatScannerFactorySharedPtr& scannerFactory)
        : ScanningServerSocket(path, mode, scannerFactory,
                         std::make_shared<SigUSR1Monitor>(std::make_shared<Reloader>(scannerFactory)))
{
}

unixsocket::ScanningServerSocket::~ScanningServerSocket()
{
    // Need to do this before our members are destroyed
    requestStop();
    join();
}