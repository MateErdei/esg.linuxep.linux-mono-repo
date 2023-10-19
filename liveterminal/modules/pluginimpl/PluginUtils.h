// Copyright 2023 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/FileUtils.h"
#include <optional>

namespace Plugin
{

    class PluginUtils
            {
    public:
        /**
        * Read a json file to get the proxy info
        * @param filepath, path for where the file containing the proxy info is located
        * @param fileSystem, to check if file exists
        * @return returns the address and password of proxy in a tuple
        */
        static std::tuple<std::string, std::string> readCurrentProxyInfo(const std::string& filepath, Common::FileSystem::IFileSystem *fileSystem);

        /**
        * Strips the path and scheme prefix off an url
        * @param url, the server link that comes down in the live response action
        * @return the domain name of the server
        */
        static std::string getHostnameFromUrl(const std::string& url);

        /**
        * Search a given file for a given key and return the value expect values in format key=value
        * @param key, key to search for in the file
        * @param filepath, file to search
        * @return returns the value unless key not found in which case returns nullopt
        */
        static std::optional<std::string> readKeyValueFromConfig(const std::string& key, const std::string& filepath);

        /**
        * Search a given file for a given key and return the value expect values in format key = value
        * @param key, key to search for in the file
        * @param filepath, file to search
        * @return returns the value unless key not found in which case returns nullopt
        */
        static std::optional<std::string> readKeyValueFromIniFile(const std::string& key, const std::string& filepath);
        /**
       * Search the version of the operating system (e.g.:)
       * @return returns the name of the operating system or "Linux" in case it did not find one
       */
        static std::string getVersionOS();
    };
}


