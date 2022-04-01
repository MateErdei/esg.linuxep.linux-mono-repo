/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Main.h"

#include "CentralRegistration.h"
#include "Logger.h"

#include <Common/UtilityImpl/StringUtils.h>
#include <Logging/ConsoleLoggingSetup.h>
#include <Logging/FileLoggingSetup.h>

namespace CentralRegistrationImpl
{
    void updateConfigOptions(const std::string& key, const std::string& value, std::map<std::string, std::string>& configOptions)
    {
        auto groupArgs = Common::UtilityImpl::StringUtils::splitString(value, "=");
        if(groupArgs.size() == 2)
        {
            configOptions[key] = groupArgs[1];
        }
    }

    std::map<std::string, std::string> processCommandLineOptions(int argc, char* argv[] )
    {
        //        parser.add_argument("--deregister",dest="deregister",action="store_true",default=False)
//        parser.add_argument("--reregister",dest="reregister",action="store_true",default=False)
//        parser.add_argument("--messagerelay",dest="messagerelay",action="store",default=None)
//        parser.add_argument("--proxycredentials",dest="proxycredentials",action="store",default=None)
//        parser.add_argument("--central-group",dest="central_group",action="store",default=None)
//        parser.add_argument("--customer-token",dest="customer_token",action="store",default=None)
//        parser.add_argument("--products",dest="selected_products",action="store",default=None)
        std::map<std::string, std::string> configOptions;

        for (int i=1; i < argc; i++)
        {
            std::string currentArg(argv[i]);

            if((i == 1) && !Common::UtilityImpl::StringUtils::startswith(currentArg, "--"))
            {
                configOptions["mcsToken"] = currentArg;
            }
            else if ((i==2) && !Common::UtilityImpl::StringUtils::startswith(currentArg, "--"))
            {
                configOptions["mcsUrl"] = currentArg;
            }
            else if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--central-group"))
            {
                updateConfigOptions("centralGroup", currentArg, configOptions);
            }
            else if(currentArg == "--customer-token")
            {
                configOptions["customerToken"] = argv[++i];
            }
            else if(currentArg == "--products")
            {
                updateConfigOptions("products", currentArg, configOptions);
            }
        }

        configOptions["mcsId"] = "";

        return configOptions;
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::ConsoleLoggingSetup loggerSetup;
        std::map<std::string, std::string> configOptions = processCommandLineOptions(argc, argv);

        CentralRegistration centralRegistration;
        centralRegistration.RegisterWithCentral(configOptions);
        return 0;
    }

} // namespace CentralRegistrationImpl
