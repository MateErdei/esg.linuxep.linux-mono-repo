//
// Created by pair on 26/04/18.
//

#include <iostream>
#include <sys/param.h>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <sys/stat.h>
#include <vector>
#include <map>
#include "SULRaii.h"
#include "DownloadReport.h"
#include "ConfigurationData.h"
#include "Warehouse.h"
#include "ProductSelection.h"
#include "Product.h"
#include "ConfigurationSettings.pb.h"
#include "DownloadReport.pb.h"
#include "SulDownloaderException.h"
#include "TimeTracker.h"
#include <cassert>

#include <google/protobuf/util/json_util.h>
#include "Logger.h"
namespace
{
    bool hasError( const std::vector<SulDownloader::Product> & products )
    {
        for( const auto & product: products)
        {
            if ( product.hasError())
            {
                return true;
            }
        }
        return false;
    }

    std::string protoBuf2Json( google::protobuf::Message & message)
    {
        using namespace google::protobuf::util;
        std::string json_output;
        JsonOptions options;
        options.add_whitespace = true;
        auto status = MessageToJsonString(message, &json_output,options );
        if ( !status.ok())
        {
            throw SulDownloader::SulDownloaderException(status.ToString());
        }
        return json_output;
    }

}

namespace SulDownloader
{


    DownloadReport runSULDownloader( ConfigurationData & configurationData)
    {
        assert( configurationData.isVerified());
        TimeTracker timeTracker;
        timeTracker.setStartTime( TimeTracker::getCurrTime());
        std::unique_ptr<Warehouse> warehouse = Warehouse::FetchConnectedWarehouse(configurationData);

        if ( warehouse->hasError())
        {
            return DownloadReport::Report(*warehouse, timeTracker);
        }

        auto productSelection = ProductSelection::CreateProductSelection(configurationData);

        warehouse->synchronize(productSelection);
        timeTracker.setSyncTime( TimeTracker::getCurrTime());

        if ( warehouse->hasError())
        {
            return DownloadReport::Report(*warehouse, timeTracker);
        }

        warehouse->distribute();

        if ( warehouse->hasError())
        {
            return DownloadReport::Report(*warehouse, timeTracker);
        }


        auto products = warehouse->getProducts();


        for( auto & product: products)
        {
            product.verify();
        }

        if ( hasError(products))
        {
            return DownloadReport::Report(products, timeTracker);
        }


        for( auto & product: products)
        {
            if (product.productHasChanged())
            {
                product.install();
            }
        }

        timeTracker.setFinishedTime( TimeTracker::getCurrTime());
        if ( hasError(products))
        {
            return DownloadReport::Report(products, timeTracker);
        }


        return DownloadReport::Report(products, timeTracker);

    }

    ConfigurationData fromJsonSettings( const std::string & settingsString )
    {
        using namespace google::protobuf::util;
        using SulDownloaderProto::ConfigurationSettings;

        ConfigurationSettings settings;
        auto status = JsonStringToMessage(settingsString, &settings );
        if ( !status.ok())
        {
            std::cout << "Failed to process jason message";
            throw SulDownloaderException( "Failed to process json message");
        }

        // load input string (json) into the configuration data
        // run runSULDownloader
        // and serialize teh DownloadReport into json and give the error code/or success
        auto sUrls = settings.sophosurls();
        std::vector<std::string> sophosURLs(std::begin(sUrls), std::end(sUrls) );
        sUrls = settings.updatecache();
        std::vector<std::string> updateCaches(std::begin(sUrls), std::end(sUrls) );
        Credentials credential( settings.credential().username(), settings.credential().password());
        Proxy proxy( settings.proxy().url(), Credentials(settings.proxy().credential().username(), settings.proxy().credential().password()));

        ConfigurationData configurationData(sophosURLs, credential, updateCaches, proxy);
        configurationData.setCertificatePath("/home/pair/dev_certificates");
        configurationData.setLocalRepository("/tmp/warehouse");
        ProductGUID primaryProductGUID;
        primaryProductGUID.Name = settings.primary();
        primaryProductGUID.Primary = true;
        primaryProductGUID.Prefix = false;
        primaryProductGUID.releaseTag = settings.releasetag();
        primaryProductGUID.baseVersion = settings.baseversion();
        configurationData.addProductSelection(primaryProductGUID);

        for(auto product : settings.fullnames())
        {
            ProductGUID productGUID;
            productGUID.Name = product;
            productGUID.releaseTag = settings.releasetag();
            productGUID.baseVersion = settings.baseversion();
            configurationData.addProductSelection(productGUID);
        }

        for(auto product : settings.prefixnames())
        {
            ProductGUID productGUID;
            productGUID.Name = product;
            productGUID.Prefix = true;
            productGUID.releaseTag = settings.releasetag();
            productGUID.baseVersion = settings.baseversion();
            configurationData.addProductSelection(productGUID);
        }

        configurationData.setCertificatePath(settings.certificatepath());

        return configurationData;
    }


    SulDownloaderProto::DownloadStatusReport fromReport( const DownloadReport & report)
    {
        SulDownloaderProto::DownloadStatusReport protoReport;
        protoReport.set_starttime(report.startTime());
        protoReport.set_finishtime(report.finishedTime());
        protoReport.set_synctime(report.syncTime());

        protoReport.set_status( toString( report.getStatus()));
        protoReport.set_description(report.getDescription());
        protoReport.set_sulerror(report.sulError());

        for ( auto & product : report.products())
        {
            SulDownloaderProto::ProductStatusReport * productReport = protoReport.add_products();
            productReport->set_productname( product.name);
            productReport->set_rigidname( product.rigidName);
            productReport->set_downloadversion( product.downloadedVersion);
            productReport->set_installedversion( product.installedVersion);
        }

        return protoReport;
    }

    std::tuple<int, std::string> configAndRunDownloader( const std::string & settingsString)
    {


        ConfigurationData configurationData = fromJsonSettings( settingsString);
        configurationData.verifySettingsAreValid();
        auto report = runSULDownloader(configurationData);
        auto protoReport = fromReport(report);
        std::string json = protoBuf2Json(protoReport);
        return {report.exitCode() , json };
    };


    int fileEntriesAndRunDownloader( const std::string & inputFilePath, const std::string & outputFilePath )
    {

        std::string settingsString = R"({
 "sophosURLs": [
  "https://ostia.eng.sophos/latest/Virt-vShield"
 ],
 "updateCache": [
  "https://ostia.eng.sophos/latest/Virt-vShieldBroken"
 ],
 "credential": {
  "username": "administrator",
  "password": "password"
 },
 "proxy": {
  "url": "noproxy:",
  "credential": {
   "username": "",
   "password": ""
  }
 },
 "certificatePath": "/home/pair/CLionProjects/everest-suldownloader/cmake-build-debug/certificates",
 "releaseTag": "RECOMMENDED",
 "baseVersion": "9",
 "primary": "FD6C1066-E190-4F44-AD0E-F107F36D9D40",
 "fullNames": [
  "1CD8A803-6047-47BC-8CBE-2D4AEB37BEE2"
 ],
 "prefixNames": [
  "1CD8A803"
 ]
})";

        // read the file
        // check can create the output
        // run configAndRunDownloader
        // return error code.
        auto result = configAndRunDownloader(settingsString);
        LOGSUPPORT(std::get<1>(result));
        return std::get<0>(result);
    }

    void create_fake_json_settings()
    {
        using SulDownloaderProto::ConfigurationSettings;
        ConfigurationSettings settings;
        Credentials credentials;

        settings.add_sophosurls("http://ostia.eng.sophos/latest/Virt-vShield");
        settings.add_updatecache("http://ostia.eng.sophos/latest/Virt-vShieldBroken");
        settings.mutable_credential()->set_username("administrator");
        settings.mutable_credential()->set_password("password");
        settings.mutable_proxy()->set_url("noproxy:");
        settings.mutable_proxy()->mutable_credential()->set_username("");
        settings.mutable_proxy()->mutable_credential()->set_password("");
        settings.set_baseversion("10");
        settings.set_releasetag("RECOMMENDED");
        settings.set_primary("FD6C1066-E190-4F44-AD0E-F107F36D9D40");
        settings.add_fullnames("A845A8B5-6532-4EF1-B19E-1DB2B3CB73D1");
        settings.add_prefixnames("A845A8B5");
        settings.set_certificatepath("/home/pair/CLionProjects/everest-suldownloader/cmake-build-debug/certificates");


        std::string json_output = protoBuf2Json( settings );

        std::cout << "Json serialized: " << json_output << std::endl;

    }

    int main_entry( int argc, char * argv[])
    {
        SULInit init;

        //create_fake_json_settings();
        //return 0;
        return fileEntriesAndRunDownloader("","");

    }



/*


class IFilter{
    book keepThis( ProductInfo & );
};



class ConnectionSetup
{

};
vector<ConnectionSEtup> ConnectionCandidates( settings );

Selection GetSelectionFilter( settings )


*/
//
//DownloadReport runSUL ( Settings & settings )
//{
//
//    auto warehouse = GetConnectedWarehouse( ConnectionCandidates(settings) );
//    if (hasError(warehouse))
//
//
//    auto products = warehouse.syncrhonize( GetProductSelection(settings) );
//    if( hasError(products))
//        return DownloadReport::create(products);
//
//    Distrubute(products);
//    if( hasError(products))
//        return DownloadReport::create(products);
//
//    Install(products);
//
//    return DownloadReport::create(products);
//}



    class AssertionFail : public std::runtime_error
    {
    public:
        AssertionFail(int ret)
                : std::runtime_error(std::to_string(ret))
        {
        }
    };

#define ASSERT_TRUE(_b)       do {if (!(_b)) {std::cerr << #_b << " is False at " << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << std::endl;throw AssertionFail(1);}} while(0)
#define ASSERT_NE(_v, _e) do {if (  (_v) == (_e) ) {std::cerr << #_v << "(" << _v << ") == " << #_e << "(" << _e << ")" << __FILE__ << ":" << __LINE__ << std::endl;throw std::runtime_error("Assertion failure");}} while(0)

//    static std::string safegetcwd()
//    {
//        char path[MAXPATHLEN];
//        char *res = getcwd(path, MAXPATHLEN);
//        ASSERT_NE(res, 0);
//        return res;
//    }

    //static const char *LOCAL_REPOSITORY = "tmp/warehouse";
    //static const char *LOCAL_LCD = "tmp/installset";

//    template<class T>
//    static bool in(const std::vector<T> &hay, const T &needle)
//    {
//        return std::find(hay.begin(), hay.end(), needle) != hay.end();
//    }

//def getTags(product):
//"""
//@return list of tagsets (tuple<tuple<baseversionAsInt>,tag,label,baseversion>)
//"""
//tags = []
//index = 0
//while True:
//tag = product.query_metadata("R_ReleaseTagsTag",index)
//if tag == "":
//break
//baseversion = product.query_metadata("R_ReleaseTagsBaseVersion",index)
//label = product.query_metadata("R_ReleaseTagsLabel",index)
//index += 1
//tags.append((convertVersionToTuple(baseversion),tag.upper(),label,baseversion))
//
//return tags

//    struct Tag
//    {
//        Tag(const std::string &t, const std::string &b, const std::string &l)
//                : tag(t), baseversion(b), label(l)
//        {
//        }
//
//        std::string tag;
//        std::string baseversion;
//        std::string label;
//    };
//
//    static std::vector<Tag> getTags(SU_PHandle &product)
//    {
//        std::vector<Tag> tags;
//        std::string tag;
//        int index = 0;
//
//        while (true)
//        {
//            tag = SU_queryProductMetadata(product, "R_ReleaseTagsTag", index);
//            if (tag == "")
//            {
//                break;
//            }
//            CStringOwner st(SU_queryProductMetadata(product, "R_ReleaseTagsBaseVersion", index));
//            std::string baseversion = st.get();
//            std::string label = SU_queryProductMetadata(product, "R_ReleaseTagsLabel", index);
//            tags.emplace_back(tag, baseversion, label);
//            index++;
//        }
//        return tags;
//    }

//    static bool hasRecommended(const std::vector<Tag> &tags)
//    {
//        for (auto &tag : tags)
//        {
//            if (tag.tag == "RECOMMENDED")
//            {
//                return true;
//            }
//        }
//        return false;
//    }

//    static int run(SU_Handle session)
//    {
//
//
//
//        SU_Result result;
//        SU_setLoggingLevel(session, SU_LoggingLevel_verbose);
//        std::string certificatePath(safegetcwd() + "/../certificates");
//
//        P("Certificates = " << certificatePath);
//        SU_setCertificatePath(session, certificatePath.c_str());
//
//        const std::string source = "SOPHOS";
////    const std::string username = "QA940267";
////    const std::string password = "54m5ung";
//        const std::string username = "administrator";
//        const std::string password = "password";
//
////    result = SU_addSophosLocation(session, "http://dci.sophosupd.com/update/");
////    ASSERT_TRUE(isSuccess(result));
////    result = SU_addSophosLocation(session, "http://dci.sophosupd.net/update/");
////    ASSERT_TRUE(isSuccess(result));
//        //result = SU_addSophosLocation(session, "http://localhost:3333/customer_files.live");
//        //result = SU_addSophosLocation(session, "http://ostia.eng.sophos/latest/Virt-vShield/1/74/174e6bde8263d4b72cbe69dff029e62a.dat");
//        result = SU_addSophosLocation(session, "http://ostia.eng.sophos/latest/Virt-vShield");
//
//        ASSERT_TRUE(isSuccess(result));
//
//        result = SU_addUpdateSource(session,
//                                    source.c_str(),
//                                    username.c_str(),
//                                    password.c_str(),
//                                    "",
//                                    "",
//                                    "");
//        ASSERT_TRUE(isSuccess(result));
//
//        std::string localRepository(safegetcwd() + "/" + LOCAL_REPOSITORY);
//        P("Local Repository = " << localRepository);
//        mkdir(localRepository.c_str(), 0700);
//        result = SU_setLocalRepository(session, localRepository.c_str());
//        ASSERT_TRUE(isSuccess(result));
//
//        result = SU_setUserAgent(session, "SULDownloader");
//        ASSERT_TRUE(isSuccess(result));
//
//        result = SU_readRemoteMetadata(session);
//        if (!isSuccess(result))
//        {
//            displayLogs(session);
//            ASSERT_TRUE(isSuccess(result));
//        }
//
//        std::vector<std::string> wantedGUIDs;
//
//        // TODO: Fill in wantedGUIDs from plugins...
////    wantedGUIDs.push_back("5CF594B0-9FED-4212-BA91-A4077CB1D1F3");
//        //wantedGUIDs.emplace_back("Linux-SENSORS-plugin");
//        //wantedGUIDs.emplace_back("Linux-SAV-plugin");
//        //wantedGUIDs.emplace_back("Linux-Beagle-Base");
//        wantedGUIDs.emplace_back("A845A8B5-6532-4EF1-B19E-1DB2B3CB73D1");
//        wantedGUIDs.emplace_back("FD6C1066-E190-4F44-AD0E-F107F36D9D40");
//
//
//        std::vector<SU_PHandle> wantedProducts;
//        std::vector<SU_PHandle> unwantedProducts;
//
//        while (true)
//        {
//            SU_PHandle product = SU_getProductRelease(session);
//            if (product == nullptr)
//            {
//                break;
//            }
//            std::string line = SU_queryProductMetadata(product, "R_Line", 0);
//            if (!in(wantedGUIDs, line))
//            {
//                P("Removing product without wanted R_line: " << line);
//                unwantedProducts.emplace_back(product);
//                continue;
//            }
//            std::vector<Tag> tags(getTags(product));
//            if (!hasRecommended(tags))
//            {
//                P("Removing product because it doesn't have recommended");
//                unwantedProducts.emplace_back(product);
//                continue;
//            }
//            P("Keeping wanted product " << line);
//
//            // TODO: other conditions
//            wantedProducts.emplace_back(product);
//        }
//
//        for (auto product : unwantedProducts)
//        {
//            SU_removeProduct(product);
//        }
//
//
//        result = SU_synchronise(session);
//        if (!isSuccess(result))
//        {
//            displayLogs(session);
//            ASSERT_TRUE(isSuccess(result));
//        }
//
//        for (auto product : wantedProducts)
//        {
//            result = SU_getSynchroniseStatus(product);
//            ASSERT_TRUE(isSuccess(result));
//        }
//
//        // distribute
//        std::map<SU_PHandle, std::string> distributionPaths;
//        for (auto product : wantedProducts)
//        {
//            const char *empty = "";
//            std::string distributePath = safegetcwd() + "/distribute/" + SU_queryProductMetadata(product, "R_Line", 0) +
//                                         SU_queryProductMetadata(product, "VersionId", 0);
//            P("Distribute to " << distributePath);
//            SU_addDistribution(product, distributePath.c_str(), SU_AddDistributionFlag_UseDefaultHomeFolder, empty,
//                               empty);
//            distributionPaths[product] = distributePath;
//        }
//        result = SU_distribute(session, SU_DistributionFlag_AlwaysDistribute);
//        if (!isSuccess(result))
//        {
//            displayLogs(session);
//            ASSERT_TRUE(isSuccess(result));
//        }
//        for (auto product : wantedProducts)
//        {
//            result = SU_getDistributionStatus(product, distributionPaths[product].c_str());
//            ASSERT_TRUE(isSuccess(result));
//        }
//
//        // TODO: verify - versig
//        for (auto product : wantedProducts)
//        {
//            std::string distributePath = distributionPaths[product];
//            // fork
//            // exec versig
//        }
//
//
//        // TODO: install
//        for (auto product : wantedProducts)
//        {
//            std::string distributePath = distributionPaths[product];
//            result = SU_getDistributionStatus(product, distributePath.c_str());
//            if (result == SU_Result_OK)
//            {
//                std::string installsh =
//                        distributePath + "/installdummy.sh"; // prevent install.sh running until we want to
//                std::string command = std::string("bash ") + installsh;
//                P("Running " << command);
//                system(command.c_str());
//                // fork
//                // exec install.sh
//            }
//        }
//
//        return 0;
//    }

}