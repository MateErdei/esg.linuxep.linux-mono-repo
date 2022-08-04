
#include "ProxyUtils.h"

#include "json.hpp"

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"
#include "FileSystem/IFileSystem.h"
#include "ObfuscationImpl/Obfuscate.h"
#include "UtilityImpl/StringUtils.h"

#include <sstream>

namespace
{
    constexpr auto PROXY_KEY = "proxy";
    constexpr auto CRED_KEY = "credentials";
}
namespace Common
{
        std::tuple<std::string, std::string> ProxyUtils::getCurrentProxy()
        {
            auto fileSystem = Common::FileSystem::fileSystem();
            auto currentProxyFilePath =
                Common::ApplicationConfiguration::applicationPathManager().getMcsCurrentProxyFilePath();
            std::string proxy;
            std::string credentials;

            if (fileSystem->isFile(currentProxyFilePath))
            {
                try
                {
                    std::string fileContents = fileSystem->readFile(currentProxyFilePath);
                    if (!fileContents.empty())
                    {
                        nlohmann::json j = nlohmann::json::parse(fileContents);
                        if (j.contains(PROXY_KEY))
                        {
                            proxy = j[PROXY_KEY];
                            if (j.contains(CRED_KEY))
                            {
                                credentials = j[CRED_KEY];
                            }
                        }
                    }
                }
                catch (const std::exception& exception)
                {
                    std::stringstream errorMessage;
                    errorMessage << "Could not parse current proxy file: " << currentProxyFilePath
                                 << " with error: " << exception.what();
                    throw std::runtime_error(errorMessage.str());
                }
            }
            return { proxy, credentials };
        }

        std::tuple<std::string, std::string> ProxyUtils::deobfuscateProxyCreds(const std::string& creds)
        {
            if (!creds.empty())
            {
                std::vector<std::string> values =
                    Common::UtilityImpl::StringUtils::splitString(Common::ObfuscationImpl::SECDeobfuscate(creds), ":");
                if (values.size() == 2)
                {
                    std::string username = values[0];
                    std::string password = values[1];
                    return { username, password };
                }
            }
            return {};
        }
    }