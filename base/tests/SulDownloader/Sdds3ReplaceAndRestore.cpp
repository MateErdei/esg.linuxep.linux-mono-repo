/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Sdds3ReplaceAndRestore.h"

#include <SulDownloader/sdds3/Sdds3Wrapper.h>
#include <mutex>
#include<iostream>
namespace{
    std::mutex preventTestsToUseEachOtherSdds3WrapperMock;
}


void Tests::replaceSdds3Wrapper(std::unique_ptr<SulDownloader::ISdds3Wrapper> pointerToReplace)
{
    SulDownloader::sdds3WrapperStaticPointer().reset(pointerToReplace.get());
    pointerToReplace.release();
}

void Tests::restoreSdds3Wrapper()
{
    SulDownloader::sdds3WrapperStaticPointer().reset(new SulDownloader::Sdds3Wrapper());
}

namespace Tests{

    ScopedReplaceSdds3Wrapper::ScopedReplaceSdds3Wrapper() : m_guard{preventTestsToUseEachOtherSdds3WrapperMock}{
        restoreSdds3Wrapper();
    }

    ScopedReplaceSdds3Wrapper::ScopedReplaceSdds3Wrapper(std::unique_ptr<SulDownloader::ISdds3Wrapper> pointerToReplace) : ScopedReplaceSdds3Wrapper()
    {
        replace(std::move(pointerToReplace));
    }

    void ScopedReplaceSdds3Wrapper::replace(std::unique_ptr<SulDownloader::ISdds3Wrapper> pointerToReplace)
    {
        replaceSdds3Wrapper(std::move(pointerToReplace));
    }

    ScopedReplaceSdds3Wrapper::~ScopedReplaceSdds3Wrapper()
    {
        restoreSdds3Wrapper();
    }
}