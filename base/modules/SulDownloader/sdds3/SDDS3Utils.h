/***********************************************************************************************

Copyright 2022 Sophos Limited. All rights reserved.

***********************************************************************************************/

#pragma once

#include <SulDownloader/sdds3/SDDS3Repository.h>

namespace SulDownloader
{

    std::string createSUSRequestBody(const SUSRequestParameters& parameters);
    void removeSDDS3Cache();

} // namespace SulDownloader
