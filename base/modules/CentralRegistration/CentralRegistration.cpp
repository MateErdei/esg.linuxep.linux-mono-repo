// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "CentralRegistration.h"

#include "Logger.h"

#include "cmcsrouter/AgentAdapter.h"
#include "cmcsrouter/Config.h"
#include "cmcsrouter/MCSApiCalls.h"

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/UpdateUtilities/CommsDataUtil.h"

#include <nlohmann/json.hpp>

namespace CentralRegistration
{
    bool CentralRegistration::tryPreregistration(
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy,
        const std::shared_ptr<MCS::MCSHttpClient>& httpClient)
    {
        MCS::MCSApiCalls mcsApi;
        try
        {
            std::string preregistrationBody = mcsApi.preregisterEndpoint(httpClient, configOptions, statusXml, proxy);
            if (!preregistrationBody.empty())
            {
                std::string newMcsToken = processPreregistrationBody(preregistrationBody);

                if (!newMcsToken.empty())
                {
                    configOptions.config[MCS::MCS_TOKEN] = newMcsToken;
                    return true;
                }
            }
        }
        catch (const std::exception& ex)
        {
            LOGERROR("Exception caught while attempting preregister: " << ex.what());
        }

        return false;
    }

    bool CentralRegistration::tryRegistration(
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::string& proxy,
        const std::shared_ptr<MCS::MCSHttpClient>& httpClient)
    {
        MCS::MCSApiCalls mcsApi;
        return mcsApi.registerEndpoint(httpClient, configOptions, statusXml, proxy);
    }

    bool CentralRegistration::tryRegistrationWithProxies(
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::shared_ptr<MCS::MCSHttpClient>& httpClient,
        bool (*registrationFunction)(
            MCS::ConfigOptions&,
            const std::string&,
            const std::string&,
            const std::shared_ptr<MCS::MCSHttpClient>& httpClient))
    {
        for (auto& messageRelay : configOptions.messageRelays) // These need to be ordered before this
        {
            std::string proxy = messageRelay.address + ":" + messageRelay.port;
            if (registrationFunction(configOptions, statusXml, proxy, httpClient))
            {
                configOptions.config[MCS::MCS_CONNECTED_PROXY] = proxy;

                //message relays
                writeToIni(false, true, proxy);
                return true;
            }
        }
        if (configOptions.config[MCS::MCS_CONNECTED_PROXY].empty() && !configOptions.config[MCS::MCS_PROXY].empty())
        {
            if (registrationFunction(configOptions, statusXml, configOptions.config[MCS::MCS_PROXY], httpClient))
            {
                //proxy
                configOptions.config[MCS::MCS_CONNECTED_PROXY] = configOptions.config[MCS::MCS_PROXY];
                writeToIni(true, false);
                return true;
            }
        }
        //direct
        writeToIni(false, false);
        return registrationFunction(configOptions, statusXml, "", httpClient);
    }

    void CentralRegistration::preregistration(
        MCS::ConfigOptions& configOptions,
        const std::string& statusXml,
        const std::shared_ptr<MCS::MCSHttpClient>& httpClient
        )
    {
        // Check options are all there: customer token + selected products
        if (configOptions.config.empty() || configOptions.config[MCS::MCS_CUSTOMER_TOKEN].empty() ||
            configOptions.config[MCS::MCS_PRODUCTS].empty())
        {
            return;
        }
        LOGINFO("Carrying out Preregistration for selected products: " << configOptions.config[MCS::MCS_PRODUCTS]);

        if (!tryRegistrationWithProxies(configOptions, statusXml, httpClient, tryPreregistration))
        {
            LOGINFO("Preregistration failed - continuing with default registration");
        }
    }

    std::string CentralRegistration::processPreregistrationBody(const std::string& preregistrationBody)
    {
        LOGDEBUG("\nPreregistrationBody:\n" << preregistrationBody << "\n\n");
        auto bodyAsJson = nlohmann::json::parse(preregistrationBody);
        std::string deploymentRegistrationToken = bodyAsJson.value("registrationToken", "");
        if (deploymentRegistrationToken.empty())
        {
            LOGERROR("No MCS Token returned from Central - Preregistration request failed");
            return "";
        }

        nlohmann::basic_json products = bodyAsJson.value("products", nlohmann::json::array({}));
        for (auto& product : products)
        {
            if (!product.value("supported", false))
            {
                LOGWARN("Unsupported product chosen for preregistration: " << product["product"]);
            }
        }

        LOGDEBUG("Preregistration successful, new deployment registration token: " << deploymentRegistrationToken);
        return deploymentRegistrationToken;
    }

    void CentralRegistration::registerWithCentral(
        MCS::ConfigOptions& configOptions,
        const std::shared_ptr<MCS::MCSHttpClient>& httpClient,
        const std::shared_ptr<MCS::IAdapter>& agentAdapter)
    {
        LOGINFO("Beginning product registration");
        std::string statusXml = agentAdapter->getStatusXml(configOptions.config);
        LOGDEBUG("Status XML:\n" + statusXml);

        preregistration(configOptions, statusXml, httpClient);

        httpClient->setCertPath(configOptions.config[MCS::MCS_CERT]);

        // This check saves retrying all proxies if preregistration succeeded on a given proxy
        // if this MCS_CONNecteD_proxy is set, then we have pre registered
        if (!configOptions.config[MCS::MCS_CONNECTED_PROXY].empty() &&
            tryRegistration(configOptions, statusXml, configOptions.config[MCS::MCS_CONNECTED_PROXY], httpClient))
        {
            LOGINFO("Product successfully registered via proxy: " << configOptions.config[MCS::MCS_CONNECTED_PROXY]);
        }
        else if (tryRegistrationWithProxies(configOptions, statusXml, httpClient, tryRegistration))
        {
            if (!configOptions.config[MCS::MCS_CONNECTED_PROXY].empty())
            {
                LOGINFO(
                    "Product successfully registered via proxy: " << configOptions.config[MCS::MCS_CONNECTED_PROXY]);
            }
            else
            {
                LOGINFO("Product successfully registered");
            }
        }
        else
        {
            LOGERROR("Product registration failed");
        }
    }
    void CentralRegistration::writeToIni(bool usedProxy, bool usedMessageRelay, const std::string& proxyOrMessageRelayURL)
    {
        try
        {
            Common::UpdateUtilities::CommsDataUtil::writeCommsTelemetryIniFile("registration_comms_check.ini", usedProxy, false, usedMessageRelay, proxyOrMessageRelayURL);
        }
        catch (Common::FileSystem::IFileSystemException& ex)
        {
            LOGWARN("Failed to write to Central Registration telemetry file: " << ex.what());
        }
    }
} // namespace CentralRegistration
