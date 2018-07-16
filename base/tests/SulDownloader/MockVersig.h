/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#ifndef EVEREST_BASE_MOCKVERSIG_H
#define EVEREST_BASE_MOCKVERSIG_H
#include "SulDownloader/IVersig.h"
#include "gmock/gmock.h"

class MockVersig : public SulDownloader::IVersig
{
public:
    MOCK_CONST_METHOD2(verify, SulDownloader::IVersig::VerifySignature(const std::string& certificatePath, const std::string & directoryPath));
};
#endif //EVEREST_BASE_MOCKVERSIG_H
