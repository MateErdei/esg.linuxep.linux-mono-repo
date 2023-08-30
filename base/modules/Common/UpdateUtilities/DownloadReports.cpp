// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "DownloadReports.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/StringUtils.h"

namespace Common::UpdateUtilities
{
    std::vector<std::string> listOfAllPreviousReports(const std::string& outputParentPath)
    {
        std::vector<std::string> filesInReportDirectory = Common::FileSystem::fileSystem()->listFiles(outputParentPath);

        // Filter file list to make sure all the files are report files based on file name
        std::vector<std::string> previousReportFiles;
        std::string startPattern("update_report");
        std::string endPattern(".json");

        for (auto& file : filesInReportDirectory)
        {
            std::string fileName = Common::FileSystem::basename(file);

            // make sure file name begins with 'report' and ends with '.json'
            using strUtils = Common::UtilityImpl::StringUtils;

            if (strUtils::startswith(fileName, startPattern) && strUtils::endswith(fileName, endPattern))
            {
                previousReportFiles.emplace_back(file);
            }
        }
        return previousReportFiles;
    }
} // namespace Common::UpdateUtilities
