/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Main.h"

#include "CentralRegistration.h"
#include "Logger.h"

#include <cmcsrouter/Config.h>

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
                configOptions[MCS::MCS_TOKEN] = currentArg;
            }
            else if ((i==2) && !Common::UtilityImpl::StringUtils::startswith(currentArg, "--"))
            {
                configOptions[MCS::MCS_URL] = currentArg;
            }
            else if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--group"))
            {
                updateConfigOptions(MCS::CENTRAL_GROUP, currentArg, configOptions);
            }
            else if(currentArg == "--customer-token")
            {
                configOptions[MCS::MCS_CUSTOMER_TOKEN] = argv[++i];
            }
            else if(Common::UtilityImpl::StringUtils::startswith(currentArg, "--products"))
            {
                updateConfigOptions(MCS::MCS_PRODUCTS, currentArg, configOptions);
            }
        }

        // default set of options, note these options are populated later after registration
        // defaulting here to make is clearer when they are being set.
        configOptions[MCS::MCS_ID] = "";
        configOptions[MCS::MCS_PASSWORD] = "";
        configOptions[MCS::MCS_PRODUCT_VERSION] ="";

        return configOptions;
    }

    std::map<std::string, std::string> registerAndObtainMcsOptions(std::map<std::string, std::string>& configOptions)
    {
        CentralRegistration centralRegistration;
        centralRegistration.RegisterWithCentral(configOptions);
        // return updated configOPtions
        return configOptions;
    }

    int main_entry(int argc, char* argv[])
    {
        Common::Logging::ConsoleLoggingSetup loggerSetup;
        std::map<std::string, std::string> configOptions = processCommandLineOptions(argc, argv);
        configOptions = registerAndObtainMcsOptions(configOptions);
        return 0;
    }

} // namespace CentralRegistrationImpl
