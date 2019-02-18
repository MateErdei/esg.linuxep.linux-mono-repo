/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "gmock/gmock.h"

#include <SulDownloader/suldownloaderdata/IVersig.h>

class MockVersig : public SulDownloader::suldownloaderdata::IVersig
{
public:
    MOCK_CONST_METHOD2(
        verify,
        SulDownloader::suldownloaderdata::IVersig::VerifySignature(
            const SulDownloader::suldownloaderdata::ConfigurationData& configurationData,
            const std::string& directoryPath));
};
