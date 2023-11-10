// Copyright 2018-2023 Sophos Limited. All rights reserved.
#include "UpdateStatus.h"

#include "PropertyTreeHelper.h"

#include "Common/UtilityImpl/TimeUtils.h"

namespace
{
    namespace pt = boost::property_tree;
    using ProductStatus = UpdateSchedulerImpl::configModule::ProductStatus;

    /**
     * Insert add subscription nodes:
     * <subscriptions>
     *  <subscription rigidname="" version="" displayVersion=""/>
     * </subscriptions>
     * @param products
     * @param subscriptionsNode
     */
    void addSubscriptionElements(
        const std::vector<ProductStatus>& products,
        const std::vector<std::string>& subscriptionsInPolicy,
        pt::ptree& subscriptionsNode)
    {
        for (auto& product : products)
        {
            // Central will only process subscription data which it has been hard cooded to know about.
            // Need to filter out products based on subscription list taken from ALC policy.
            bool subscriptionValid = false;
            for (auto& validSubscriptionRigidName : subscriptionsInPolicy)
            {
                if (product.RigidName == validSubscriptionRigidName)
                {
                    subscriptionValid = true;
                    break;
                }
            }

            if (subscriptionValid)
            {
                pt::ptree subNode;
                subNode.put("<xmlattr>.rigidName", product.RigidName);
                subNode.put("<xmlattr>.version", product.DownloadedVersion);
                subNode.put("<xmlattr>.displayVersion", product.DownloadedVersion);
                subscriptionsNode.add_child("subscription", subNode);
            }
        }
    }

    /**
     * Insert add product nodes:
     * <products>
     *  <product rigidname="" version="" displayVersion="" productName=""/>
     * </products>
     * @param products
     * @param productsNode
     */
    void addProductsElements(const std::vector<ProductStatus>& products, pt::ptree& subscriptionsNode)
    {
        for (auto& product : products)
        {
            pt::ptree subNode;
            subNode.put("<xmlattr>.rigidName", product.RigidName);
            subNode.put("<xmlattr>.productName", product.ProductName);
            subNode.put("<xmlattr>.downloadedVersion", product.DownloadedVersion);
            subNode.put("<xmlattr>.installedVersion", product.InstalledVersion);
            subscriptionsNode.add_child("product", subNode);
        }
    }

    void addFeatures(const std::vector<std::string>& features, pt::ptree& featuresNode)
    {
        for (auto& feature : features)
        {
            pt::ptree subNode;
            subNode.put("<xmlattr>.id", feature);
            featuresNode.add_child("Feature", subNode);
        }
    }

    void addStates(pt::ptree& autoUpdate, const UpdateSchedulerImpl::StateData::StateMachineData& stateMachineData)
    {
        // Adding Download State Machine Data
        std::string downloadStateStatusValue("bad");

        if (stateMachineData.getDownloadState() == "0")
        {
            downloadStateStatusValue = "good";
        }
        std::string convertedtimeStamp;
        autoUpdate.put("downloadState.state", downloadStateStatusValue);

        if (stateMachineData.getDownloadState() != "0")
        {
            long epoch = stateMachineData.getDownloadFailedSinceTime().empty()
                             ? 0
                             : std::stol(stateMachineData.getDownloadFailedSinceTime());
            std::chrono::system_clock::time_point tpdownloadFailed((std::chrono::seconds(epoch)));
            convertedtimeStamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(tpdownloadFailed);
            autoUpdate.add("downloadState.failedSince", convertedtimeStamp);
        }

        // Adding Install State Machine Data

        std::string installStateStatusValue("bad");

        if (stateMachineData.getInstallState() == "0")
        {
            installStateStatusValue = "good";
        }

        autoUpdate.put("installState.state", installStateStatusValue);

        if (stateMachineData.getInstallState() == "0")
        {
            long epoch = stateMachineData.getLastGoodInstallTime().empty()
                             ? 0
                             : std::stol(stateMachineData.getLastGoodInstallTime());
            std::chrono::system_clock::time_point tpLastGoodInstall((std::chrono::seconds(epoch)));
            convertedtimeStamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(tpLastGoodInstall);
            autoUpdate.add("installState.lastGood", convertedtimeStamp);
        }
        else
        {
            long epoch = stateMachineData.getInstallFailedSinceTime().empty()
                             ? 0
                             : std::stol(stateMachineData.getInstallFailedSinceTime());
            std::chrono::system_clock::time_point tpInstallFailed((std::chrono::seconds(epoch)));
            convertedtimeStamp = Common::UtilityImpl::TimeUtils::MessageTimeStamp(tpInstallFailed);
            autoUpdate.add("installState.failedSince", convertedtimeStamp);
        }
    }


} // namespace

namespace UpdateSchedulerImpl::configModule
{
    std::string SerializeUpdateStatus(
        const UpdateSchedulerImpl::configModule::UpdateStatus& status,
        const std::string& revID,
        const std::string& versionId,
        const std::string& machineId,
        const std::vector<std::string>& subscriptionsInPolicy,
        const std::vector<std::string>& features,
        const StateData::StateMachineData& stateMachineData)
    {
        static const std::string L_STATUS_TEMPLATE{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
<CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
<autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="@@version@@">
    <endpoint id="@@endpointid@@" />
<rebootState>
        <required>no</required>
</rebootState>
<downloadState>
        <state>@@downloadStateValue@@</state>
        <!-- <failedSince>@@downloadFailedSinceTime@@</failedSince> -->
</downloadState>
<installState>
        <state>@@installStateValue@@</state>
        <!-- <lastGood>@@lastGoodInstallTime@@</lastGood>
        <failedSince>@@installFailedSinceTime@@</failedSince> -->
</installState>
</autoUpdate>
<subscriptions><!-- @@subscriptionsElement@@ -->
</subscriptions>
<products><!-- @@productsElement@@ -->
</products>
<Features>
</Features>
</status>)sophos" };

        namespace pt = boost::property_tree;
        pt::ptree tree = parseString(L_STATUS_TEMPLATE);


        auto& statusNode = tree.get_child("status"); // Needs to be a reference so that we mutate the actual tree!

        statusNode.put("CompRes.<xmlattr>.RevID", revID);
        auto& autoUpdate =
            statusNode.get_child("autoUpdate"); // Needs to be a reference so that we mutate the actual tree!

        autoUpdate.put("<xmlattr>.version", versionId);
        autoUpdate.put("endpoint.<xmlattr>.id", machineId);

        addStates(autoUpdate, stateMachineData);

        addSubscriptionElements(status.Subscriptions, subscriptionsInPolicy, statusNode.get_child("subscriptions"));
        addProductsElements(status.Products, statusNode.get_child("products"));
        addFeatures(features, statusNode.get_child("Features"));
        return toString(tree);
    }
} // namespace UpdateSchedulerImpl::configModule