/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "SulDownloader/sdds3/ISdds3Wrapper.h"
#include <mutex>
#include <memory>
namespace Tests
{
    void replaceSdds3Wrapper(std::unique_ptr<SulDownloader::ISdds3Wrapper>);
    void restoreSdds3Wrapper();
    struct ScopedReplaceSdds3Wrapper{
        std::lock_guard<std::mutex> m_guard;
        ScopedReplaceSdds3Wrapper(std::unique_ptr<SulDownloader::ISdds3Wrapper>);
        ScopedReplaceSdds3Wrapper();
        void replace(std::unique_ptr<SulDownloader::ISdds3Wrapper>);
        ~ScopedReplaceSdds3Wrapper();
    };
} // namespace Tests

