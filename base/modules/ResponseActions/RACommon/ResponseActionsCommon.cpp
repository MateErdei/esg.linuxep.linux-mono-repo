// Copyright 2023 Sophos Limited. All rights reserved.

#include "ResponseActionsCommon.h"

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "toUtf8Exception.h"

#define BOOST_LOCALE_HIDE_AUTO_PTR
#include <boost/locale.hpp>
#include <optional>
#include <string>

namespace ResponseActions::RACommon
{
    void sendResponse(const std::string& correlationId, const std::string& content)
    {
        std::string tmpPath = Common::ApplicationConfiguration::applicationPathManager().getTempPath();
        std::string rootInstall = Common::ApplicationConfiguration::applicationPathManager().sophosInstall();
        std::string targetDir = Common::FileSystem::join(rootInstall, "base/mcs/response");
        std::string fileName = "CORE_" + correlationId + "_response.json";
        std::string fullTargetName = Common::FileSystem::join(targetDir, fileName);
        Common::FileSystem::createAtomicFileToSophosUser(content, fullTargetName, tmpPath);
    }

    std::optional<std::string> toUtf8(const std::string& str, bool appendConversion)
    {
        try
        {
            return boost::locale::conv::to_utf<char>(str, "UTF-8", boost::locale::conv::stop);
        }
        catch(const boost::locale::conv::conversion_error& e)
        {
            //?
        }

        std::vector<std::string> encodings{"EUC-JP", "Shift-JIS", "SJIS", "Latin1"};
        for (const auto& encoding : encodings)
        {
            try
            {
                if (appendConversion)
                {
                    std::string encoding_info = " (" + encoding + ")";
                    return boost::locale::conv::to_utf<char>(str,
                                                             encoding,
                                                             boost::locale::conv::stop).append(encoding_info);
                }
                else
                {
                    return boost::locale::conv::to_utf<char>(str, encoding, boost::locale::conv::stop);
                }

            }
            catch(const boost::locale::conv::conversion_error& e)
            {
                continue;
            }
            catch(const boost::locale::conv::invalid_charset_error& e)
            {
                throw toUtf8Exception(" due to invalid charset.");
            }
        }
        throw toUtf8Exception("");
    }
} // namespace ResponseActions::RACommon