// Copyright 2018-2023 Sophos Limited. All rights reserved.
#pragma once

#include "gmock/gmock.h"

#include <SulDownloader/suldownloaderdata/IVersig.h>

class MockVersig : public SulDownloader::suldownloaderdata::IVersig
{
public:
    MOCK_METHOD(SulDownloader::suldownloaderdata::IVersig::VerifySignature, verify,
                (const settings_t& configurationData,
                 const std::string& directoryPath), (const, override));
};
