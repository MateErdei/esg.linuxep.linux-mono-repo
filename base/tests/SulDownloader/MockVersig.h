/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <SulDownloader/suldownloaderdata/IVersig.h>
#include "gmock/gmock.h"

class MockVersig : public SulDownloader::suldownloaderdata::IVersig
{
public:
    MOCK_CONST_METHOD2(verify, SulDownloader::suldownloaderdata::IVersig::VerifySignature(const std::string& certificatePath, const std::string & directoryPath));
};

