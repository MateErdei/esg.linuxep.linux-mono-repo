/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateStatus.h"

#include "../Logger.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace
{
    namespace pt = boost::property_tree;
    using ProductStatus = UpdateSchedulerImpl::configModule::ProductStatus;

    void addSubscriptionElements(const std::vector<ProductStatus>& products, pt::ptree& subscriptionsNode)
    {
        for (auto& product : products)
        {
            pt::ptree subNode;
            subNode.put("<xmlattr>.rigidName", product.RigidName);
            subNode.put("<xmlattr>.version", product.DownloadedVersion);
            subNode.put("<xmlattr>.displayVersion", product.DownloadedVersion);
            subscriptionsNode.add_child("subscription", subNode);
        }
    }

    // static void dump(const boost::property_tree::ptree& t, const std::string& indent="")
    //{
    //    if (!t.data().empty())
    //    {
    //        LOGDEBUG(indent << t.data());
    //    }
    //    BOOST_FOREACH(
    //            const boost::property_tree::ptree::value_type &v,
    //            t)
    //    {
    //        LOGDEBUG(indent << "." << v.first);
    //        dump(v.second, indent+" ");
    //    }
    //}

} // namespace

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        std::string SerializeUpdateStatus(
            const UpdateSchedulerImpl::configModule::UpdateStatus& status,
            const std::string& revID,
            const std::string& versionId,
            const std::string& machineId,
            const Common::UtilityImpl::IFormattedTime& iFormattedTime)
        {
            static const std::string L_STATUS_TEMPLATE{ R"sophos(<?xml version="1.0" encoding="utf-8" ?>
<status xmlns="com.sophos\mansys\status" type="sau">
    <CompRes xmlns="com.sophos\msys\csc" Res="Same" RevID="@@revid@@" policyType="1" />
    <autoUpdate xmlns="http://www.sophos.com/xml/mansys/AutoUpdateStatus.xsd" version="@@version@@">
        <lastBootTime>@@boottime@@</lastBootTime>
        <lastStartedTime>@@lastStartedTime@@</lastStartedTime>
        <lastSyncTime>@@lastSyncTime@@</lastSyncTime>
        <lastInstallStartedTime>@@lastInstallStartedTime@@</lastInstallStartedTime>
        <lastFinishedTime>@@lastFinishedTime@@</lastFinishedTime><!-- @@firstfailedTimeElement@@ -->
        <lastResult>@@lastResult@@</lastResult>
        <endpoint id="@@endpointid@@" />
    </autoUpdate>
    <subscriptions><!-- @@subscriptionsElement@@ -->
    </subscriptions>
</status>)sophos" };

            namespace pt = boost::property_tree;
            pt::ptree tree;
            std::istringstream reader(L_STATUS_TEMPLATE);
            pt::xml_parser::read_xml(reader, tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);

            std::string bootTime = iFormattedTime.bootTime();

            auto& statusNode = tree.get_child("status"); // Needs to be a reference so that we mutate the actual tree!

            statusNode.put("CompRes.<xmlattr>.RevID", revID);
            auto& autoUpdate =
                statusNode.get_child("autoUpdate"); // Needs to be a reference so that we mutate the actual tree!

            autoUpdate.put("<xmlattr>.version", versionId);
            autoUpdate.put("lastBootTime", bootTime);
            autoUpdate.put("lastStartedTime", status.LastStartTime);
            autoUpdate.put("lastSyncTime", status.LastSyncTime);
            autoUpdate.put("lastInstallStartedTime", status.LastInstallStartedTime);
            autoUpdate.put("lastFinishedTime", status.LastFinishdTime);
            if (!status.FirstFailedTime.empty())
            {
                pt::ptree::assoc_iterator it =
                    autoUpdate.find("lastResult"); // insert adds node before iterator element
                assert(it != autoUpdate.not_found());
                tree.insert(autoUpdate.to_iterator(it), { "firstFailedTime", pt::ptree(status.FirstFailedTime) });
                //                dump(tree);
            }
            autoUpdate.put("lastResult", std::to_string(status.LastResult));
            autoUpdate.put("endpoint.<xmlattr>.id", machineId);

            addSubscriptionElements(status.Products, statusNode.get_child("subscriptions"));

            std::ostringstream xmlOutput;
            pt::xml_parser::write_xml(xmlOutput, tree);
            return xmlOutput.str();
        }

    } // namespace configModule
} // namespace UpdateSchedulerImpl