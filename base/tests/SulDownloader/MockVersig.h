/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <SulDownloader/suldownloaderdata/IVersig.h>
#include "gmock/gmock.h"

class MockVersig : public SulDownloader::IVersig
{
public:
    MOCK_CONST_METHOD2(verify, SulDownloader::IVersig::VerifySignature(const std::string& certificatePath, const std::string & directoryPath));
};

