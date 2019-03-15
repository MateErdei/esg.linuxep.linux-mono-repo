/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystemImpl/FileSystemImpl.h>

namespace diagnose
{
    class GatherFiles
    {


    public:
        GatherFiles() = default;

        void copyLogFiles(std::string installLocation, std::string destination);
        void copyMcsConfigFiles(std::string installLocation, std::string destination);
        std::string createDiagnoseFolder(std::string path);

    private:
        std::vector<std::string> m_logFilePaths{"logs/base/sophosspl/mcs_envelope.log",
                                                "logs/base/sophosspl/mcsrouter.log",
                                                "logs/base/sophosspl/sophos_managementagent.log",
                                                "logs/base/wdctl.log",
                                                "logs/base/watchdog.log",
                                                "logs/base/updatescheduler.log",
                                                "plugins/AuditPlugin/log/AuditPlugin.log",
                                                "plugins/EventProcessor/log/EventProcessor.log"};

        std::vector<std::string> m_mcsConfigDirectories{"/base/mcs/action/",
                                                        "/base/mcs/policies/",
                                                        "/base/mcs/status/"};

        Common::FileSystem::FileSystemImpl m_fileSystem;
    };

}
