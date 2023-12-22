// Copyright 2023 Sophos All rights reserved.

#include "CommsDataUtil.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/OSUtilitiesImpl/SystemUtils.h"

using namespace Common::UpdateUtilities;

void CommsDataUtil::writeCommsTelemetryIniFile(
        const std::string& fileName,
        bool usedProxy,
        bool usedUpdateCache,
        bool usedMessageRelay,
        const std::string& proxyOrMessageRelayURL)
{
//        Means we only write a file when inside thininstaller
    std::string tempDirPath = OSUtilities::systemUtils()->getEnvironmentVariable("SOPHOS_TEMP_DIRECTORY");
    if (!tempDirPath.empty())
    {
        std::stringstream content;
        content << "usedProxy = " << (usedProxy ? "true" : "false") << std::endl;
        content << "usedUpdateCache = " << (usedUpdateCache ? "true" : "false") << std::endl;
        content << "usedMessageRelay = " << (usedMessageRelay ? "true" : "false") << std::endl;
        content << "proxyOrMessageRelayURL = " << proxyOrMessageRelayURL << std::endl;

        std::string iniPath = Common::FileSystem::join(tempDirPath, fileName);
        Common::FileSystem::fileSystem()->writeFile(iniPath, content.str());
    }
}