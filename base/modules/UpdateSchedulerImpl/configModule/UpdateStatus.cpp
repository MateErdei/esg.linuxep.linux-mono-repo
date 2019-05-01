/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "UpdateStatus.h"

#include "PropertyTreeHelper.h"

#include "../Logger.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <boost/foreach.hpp>

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
        <endpoint id="@@endpointid@@" />
    </autoUpdate>
    <subscriptions><!-- @@subscriptionsElement@@ -->
    </subscriptions>
</status>)sophos" };

            namespace pt = boost::property_tree;
            pt::ptree tree = parseString(L_STATUS_TEMPLATE);

            std::string bootTime = iFormattedTime.bootTime();

            auto& statusNode = tree.get_child("status"); // Needs to be a reference so that we mutate the actual tree!

            statusNode.put("CompRes.<xmlattr>.RevID", revID);
            auto& autoUpdate =
                statusNode.get_child("autoUpdate"); // Needs to be a reference so that we mutate the actual tree!

            autoUpdate.put("<xmlattr>.version", versionId);
            autoUpdate.put("endpoint.<xmlattr>.id", machineId);

            addSubscriptionElements(status.Products, statusNode.get_child("subscriptions"));

            return toString(tree);
        }

    } // namespace configModule
} // namespace UpdateSchedulerImpl