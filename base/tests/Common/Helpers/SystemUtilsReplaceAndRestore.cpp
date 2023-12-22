// Copyright 2023 Sophos All rights reserved.

#include "SystemUtilsReplaceAndRestore.h"

#include "base/modules/Common/OSUtilitiesImpl/SystemUtils.h"

namespace{
    std::mutex preventTestsToUseEachOtherSystemUtilsMock;
}


void Tests::replaceSystemUtils(std::unique_ptr<OSUtilities::ISystemUtils> pointerToReplace)
{
    OSUtilities::systemUtilsStaticPointer().reset(pointerToReplace.get());
    pointerToReplace.release();
}

void Tests::restoreSystemUtils()
{
    OSUtilities::systemUtilsStaticPointer().reset(new OSUtilities::SystemUtilsImpl());
}

namespace Tests{

    ScopedReplaceSystemUtils::ScopedReplaceSystemUtils() : m_guard{preventTestsToUseEachOtherSystemUtilsMock}{
        restoreSystemUtils();
    }

    ScopedReplaceSystemUtils::ScopedReplaceSystemUtils(std::unique_ptr<OSUtilities::ISystemUtils> pointerToReplace) : ScopedReplaceSystemUtils()
    {
        replace(std::move(pointerToReplace));
    }

    void ScopedReplaceSystemUtils::replace(std::unique_ptr<OSUtilities::ISystemUtils> pointerToReplace)
    {
        replaceSystemUtils(std::move(pointerToReplace));
    }

    ScopedReplaceSystemUtils::~ScopedReplaceSystemUtils()
    {
        restoreSystemUtils();
    }
}