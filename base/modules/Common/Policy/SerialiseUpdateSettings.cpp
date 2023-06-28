// Copyright 2023 Sophos Limited. All rights reserved.

#include "SerialiseUpdateSettings.h"

#include "ConfigurationSettings.pb.h"
#include "PolicyParseException.h"

#include "Common/ProtobufUtil/MessageUtility.h"

#include <google/protobuf/util/json_util.h>

using namespace Common::Policy;

namespace
{
    class UpdatePolicySerialisationException : public PolicyParseException
    {
    public:
        using PolicyParseException::PolicyParseException;
    };

    ProductSubscription getSubscription(
        const PolicyProto::ConfigurationSettings_Subscription& proto_subscription)
    {
        return ProductSubscription(
            proto_subscription.rigidname(),
            proto_subscription.baseversion(),
            proto_subscription.tag(),
            proto_subscription.fixedversion());
    }

    void setProtobufEntries(
        const ProductSubscription& subscription,
        PolicyProto::ConfigurationSettings_Subscription* proto_subscription)
    {
        assert( proto_subscription != nullptr);
        proto_subscription->set_rigidname(subscription.rigidName());
        proto_subscription->set_baseversion(subscription.baseVersion());
        proto_subscription->set_tag(subscription.tag());
        proto_subscription->set_fixedversion(subscription.fixedVersion());
    }

}

UpdateSettings SerialiseUpdateSettings::fromJsonSettings(const std::string& settingsString)
{
    using namespace google::protobuf::util;
    using PolicyProto::ConfigurationSettings;

    ConfigurationSettings settings;
    JsonParseOptions jsonParseOptions;
    jsonParseOptions.ignore_unknown_fields = true;

    auto status = JsonStringToMessage(settingsString, &settings, jsonParseOptions);

    if (!status.ok())
    {
        std::stringstream errorMsg;
        errorMsg << "Failed to process json message with error: " << status.ToString();
        throw UpdatePolicySerialisationException(LOCATION, errorMsg.str());
    }

    UpdateSettings updateSettings;

    auto sUrls = settings.sophosurls();
    std::vector<std::string> sophosURLs(std::begin(sUrls), std::end(sUrls));
    updateSettings.setSophosLocationURLs(sophosURLs);

    Credentials credential(settings.credential().username(), settings.credential().password());
    updateSettings.setCredentials(credential);

    auto updateCacheUrls = settings.updatecache();
    std::vector<std::string> updateCaches(std::begin(updateCacheUrls), std::end(updateCacheUrls));
    updateSettings.setLocalUpdateCacheHosts(updateCaches);


    Proxy proxy(
        settings.proxy().url(),
        ProxyCredentials(
            settings.proxy().credential().username(),
            settings.proxy().credential().password(),
            settings.proxy().credential().proxytype()));
    updateSettings.setPolicyProxy(proxy);

    ProductSubscription primary = getSubscription(settings.primarysubscription());
    std::vector<ProductSubscription> products;
    for (const auto& ProtoSubscription : settings.products())
    {
        products.emplace_back(getSubscription(ProtoSubscription));
    }
    std::vector<std::string> features;
    for (const auto& feature : settings.features())
    {
        features.emplace_back(feature);
    }
    updateSettings.setPrimarySubscription(primary);
    updateSettings.setProductsSubscription(products);
    updateSettings.setFeatures(features);
//    updateSettings.setJWToken(settings.jwtoken());
    updateSettings.setVersigPath(settings.versigpath());
    updateSettings.setUpdateCacheCertPath(settings.updatecachecertpath());
//    updateSettings.setTenantId(settings.tenantid());
//    updateSettings.setDeviceId(settings.deviceid());
    updateSettings.setDoForcedUpdate(settings.forceupdate());
    updateSettings.setDoForcedPausedUpdate(settings.forcepausedupdate());

    std::vector<std::string> installArgs(
        std::begin(settings.installarguments()), std::end(settings.installarguments()));

    updateSettings.setInstallArguments(installArgs);

    using LogLevel = UpdateSettings::LogLevel;
    LogLevel level = (settings.loglevel() == ::PolicyProto::ConfigurationSettings_LogLevelOption_NORMAL)
                         ? LogLevel::NORMAL
                         : LogLevel::VERBOSE;
    updateSettings.setLogLevel(level);

//    updateSettings.setForceReinstallAllProducts(settings.forcereinstallallproducts());

    std::vector<std::string> manifestnames(std::begin(settings.manifestnames()), std::end(settings.manifestnames()));

    updateSettings.setManifestNames(manifestnames);

    std::vector<std::string> optionalManifestnames(std::begin(settings.optionalmanifestnames()), std::end(settings.optionalmanifestnames()));

    updateSettings.setOptionalManifestNames(optionalManifestnames);

    updateSettings.setUseSlowSupplements(settings.useslowsupplements());

    return updateSettings;

}

std::string SerialiseUpdateSettings::toJsonSettings(const UpdateSettings& updateSettings)
{
    using namespace google::protobuf::util;
    using PolicyProto::ConfigurationSettings;

    PolicyProto::ConfigurationSettings settings;
    for (const auto& url : updateSettings.getSophosLocationURLs())
    {
        settings.add_sophosurls()->assign(url);
    }

    for (const auto& cacheUrl : updateSettings.getLocalUpdateCacheHosts())
    {
        settings.add_updatecache()->assign(cacheUrl);
    }

    settings.mutable_credential()->set_username(updateSettings.getCredentials().getUsername());
    settings.mutable_credential()->set_password(updateSettings.getCredentials().getPassword());

    settings.mutable_proxy()->mutable_credential()->set_username(
        updateSettings.getPolicyProxy().getCredentials().getUsername());
    settings.mutable_proxy()->mutable_credential()->set_password(
        updateSettings.getPolicyProxy().getCredentials().getPassword());
    settings.mutable_proxy()->mutable_credential()->set_proxytype(
        updateSettings.getPolicyProxy().getCredentials().getProxyType());
    settings.mutable_proxy()->mutable_url()->assign(updateSettings.getPolicyProxy().getUrl());

//    settings.mutable_jwtoken()->assign(updateSettings.getJWToken());
    settings.mutable_versigpath()->assign(updateSettings.getVersigPath());
    settings.mutable_updatecachecertpath()->assign(updateSettings.getUpdateCacheCertPath());
//    settings.mutable_tenantid()->assign(updateSettings.getTenantId());
//    settings.mutable_deviceid()->assign(updateSettings.getDeviceId());
    settings.set_forceupdate(updateSettings.getDoForcedUpdate());
    settings.set_forcepausedupdate(updateSettings.getDoPausedForcedUpdate());
    const auto& primarySubscription = updateSettings.getPrimarySubscription();
    setProtobufEntries(primarySubscription, settings.mutable_primarysubscription());
    for (const auto& product : updateSettings.getProductsSubscription())
    {
        setProtobufEntries(product, settings.add_products());
    }
    for (const auto& feature : updateSettings.getFeatures())
    {
        settings.add_features(feature);
    }

    for (const auto& installarg : updateSettings.getInstallArguments())
    {
        settings.add_installarguments()->assign(installarg);
    }

    if (updateSettings.getLogLevel() == UpdateSettings::LogLevel::NORMAL)
    {
        settings.set_loglevel(::PolicyProto::ConfigurationSettings_LogLevelOption_NORMAL);
    }
    else
    {
        settings.set_loglevel(::PolicyProto::ConfigurationSettings_LogLevelOption_VERBOSE);
    }

    for (const auto& manifestName : updateSettings.getManifestNames())
    {
        settings.add_manifestnames(manifestName);
    }

    settings.set_useslowsupplements(updateSettings.getUseSlowSupplements());

    for (const auto& optionalManifestName : updateSettings.getOptionalManifestNames())
    {
        settings.add_optionalmanifestnames(optionalManifestName);
    }

    return Common::ProtobufUtil::MessageUtility::protoBuf2Json(settings);
}