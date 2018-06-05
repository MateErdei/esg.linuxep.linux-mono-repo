//
// Created by pair on 26/04/18.
//

extern "C" {
#include <SUL.h>
}
#include <iostream>
#include <sys/param.h>
#include <unistd.h>
#include <exception>
#include <algorithm>
#include <sys/stat.h>
#include <vector>
#include <map>

#define P(x) std::cerr << x << std::endl

class SULSession
{
public:
    SULSession()
    {
        m_session = SU_beginSession();
    }
    ~SULSession()
    {
        if (m_session != nullptr)
        {
            SU_endSession(m_session);
        }
    }
    SU_Handle m_session;
};

class SULInit
{
public:
    SULInit() {SU_init();}
    ~SULInit() {SU_deinit();}
};

static bool isSuccess(SU_Result result)
{
    return result == SU_Result_OK
           || result == SU_Result_nullSuccess
           || result == SU_Result_notAttempted;
}

static void displayLogs(SU_Handle ses)
{
    P("LastError:"<<SU_getLastError(ses));
    P("Error:"<<SU_getErrorDetails(ses));
    const char* log;
    while (true)
    {
        log = SU_getLogEntry(ses);
        if (log == 0)
        {
            break;
        }
        P("Log:"<<log);
    }


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
//    auto products = warehouse.syncrhonize( GetSelectionFilter(settings) );
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
    {}
};

#define ASSERT_TRUE(_b)       do {if (!(_b)) {std::cerr << #_b << " is False at " << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << std::endl;throw AssertionFail(1);}} while(0)
#define ASSERT_NE(_v,_e) do {if (  (_v) == (_e) ) {std::cerr << #_v << "(" << _v << ") == " << #_e << "(" << _e << ")" << __FILE__ << ":" << __LINE__ << std::endl;throw std::runtime_error("Assertion failure");}} while(0)

static std::string safegetcwd()
{
    char path[MAXPATHLEN];
    char* res = getcwd(path,MAXPATHLEN);
    ASSERT_NE(res,0);
    return res;
}

static const char* LOCAL_REPOSITORY = "tmp/warehouse";
static const char* LOCAL_LCD        = "tmp/installset";

template<class T>
static bool in(const std::vector<T>& hay, const T& needle)
{
    return std::find(hay.begin(),hay.end(),needle) != hay.end();
}

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

struct Tag
{
    Tag(const std::string& t, const std::string& b, const std::string& l)
            : tag(t),baseversion(b),label(l) {}
    std::string tag;
    std::string baseversion;
    std::string label;
};

static std::vector<Tag> getTags(SU_PHandle& product)
{
    std::vector<Tag> tags;
    std::string tag;
    int index = 0;

    while (true)
    {
        tag = SU_queryProductMetadata(product, "R_ReleaseTagsTag", index);
        if (tag == "")
        {
            break;
        }
        std::string baseversion = SU_queryProductMetadata(product, "R_ReleaseTagsBaseVersion", index);
        std::string label = SU_queryProductMetadata(product, "R_ReleaseTagsLabel", index);
        tags.emplace_back(tag,baseversion,label);
        index++;
    }
    return tags;
}

static bool hasRecommended(const std::vector<Tag> &tags)
{
    for (auto& tag : tags)
    {
        if (tag.tag == "RECOMMENDED")
        {
            return true;
        }
    }
    return false;
}

static int run(SU_Handle session)
{
    SU_Result result;
    SU_setLoggingLevel(session,SU_LoggingLevel_verbose);
    std::string certificatePath(safegetcwd()+"/../certificates");

    P("Certificates = "<<certificatePath);
    SU_setCertificatePath(session,certificatePath.c_str());

    const std::string source = "SOPHOS";
//    const std::string username = "QA940267";
//    const std::string password = "54m5ung";
    const std::string username = "administrator";
    const std::string password = "password";

//    result = SU_addSophosLocation(session, "http://dci.sophosupd.com/update/");
//    ASSERT_TRUE(isSuccess(result));
//    result = SU_addSophosLocation(session, "http://dci.sophosupd.net/update/");
//    ASSERT_TRUE(isSuccess(result));
    //result = SU_addSophosLocation(session, "http://localhost:3333/customer_files.live");
    //result = SU_addSophosLocation(session, "http://ostia.eng.sophos/latest/Virt-vShield/1/74/174e6bde8263d4b72cbe69dff029e62a.dat");
    result = SU_addSophosLocation(session, "http://ostia.eng.sophos/latest/Virt-vShield");

    ASSERT_TRUE(isSuccess(result));

    result = SU_addUpdateSource(session,
                                source.c_str(),
                                username.c_str(),
                                password.c_str(),
                                "",
                                "",
                                "");
    ASSERT_TRUE(isSuccess(result));

    std::string localRepository(safegetcwd()+"/"+LOCAL_REPOSITORY);
    P("Local Repository = "<<localRepository);
    mkdir(localRepository.c_str(),0700);
    result = SU_setLocalRepository(session,localRepository.c_str());
    ASSERT_TRUE(isSuccess(result));

    result = SU_setUserAgent(session, "SULDownloader");
    ASSERT_TRUE(isSuccess(result));

    result = SU_readRemoteMetadata(session);
    if (!isSuccess(result))
    {
        displayLogs(session);
        ASSERT_TRUE(isSuccess(result));
    }

    std::vector<std::string> wantedGUIDs;

    // TODO: Fill in wantedGUIDs from plugins...
//    wantedGUIDs.push_back("5CF594B0-9FED-4212-BA91-A4077CB1D1F3");
    //wantedGUIDs.emplace_back("Linux-SENSORS-plugin");
    //wantedGUIDs.emplace_back("Linux-SAV-plugin");
    //wantedGUIDs.emplace_back("Linux-Beagle-Base");
    wantedGUIDs.emplace_back("A845A8B5-6532-4EF1-B19E-1DB2B3CB73D1");
    wantedGUIDs.emplace_back("FD6C1066-E190-4F44-AD0E-F107F36D9D40");


    std::vector<SU_PHandle> wantedProducts;
    std::vector<SU_PHandle> unwantedProducts;

    while(true)
    {
        SU_PHandle product = SU_getProductRelease(session);
        if (product == nullptr)
        {
            break;
        }
        std::string line = SU_queryProductMetadata(product,"R_Line",0);
        if (! in(wantedGUIDs,line) )
        {
            P("Removing product without wanted R_line: "<<line);
            unwantedProducts.emplace_back(product);
            continue;
        }
        std::vector<Tag> tags(getTags(product));
        if (! hasRecommended(tags))
        {
            P("Removing product because it doesn't have recommended");
            unwantedProducts.emplace_back(product);
            continue;
        }
        P("Keeping wanted product "<<line);

        // TODO: other conditions
        wantedProducts.emplace_back(product);
    }

    for (auto product : unwantedProducts)
    {
        SU_removeProduct(product);
    }


    result = SU_synchronise(session);
    if (!isSuccess(result))
    {
        displayLogs(session);
        ASSERT_TRUE(isSuccess(result));
    }

    for (auto product : wantedProducts)
    {
        result = SU_getSynchroniseStatus(product);
        ASSERT_TRUE(isSuccess(result));
    }

    // distribute
    std::map<SU_PHandle, std::string> distributionPaths;
    for (auto product : wantedProducts)
    {
        const char* empty = "";
        std::string distributePath = safegetcwd()+"/distribute/" + SU_queryProductMetadata(product,"R_Line",0) + SU_queryProductMetadata(product,"VersionId",0);
        P("Distribute to "<<distributePath);
        SU_addDistribution(product,distributePath.c_str(),SU_AddDistributionFlag_UseDefaultHomeFolder,empty,empty);
        distributionPaths[product] = distributePath;
    }
    result = SU_distribute(session,SU_DistributionFlag_AlwaysDistribute);
    if (!isSuccess(result))
    {
        displayLogs(session);
        ASSERT_TRUE(isSuccess(result));
    }
    for (auto product : wantedProducts)
    {
        result = SU_getDistributionStatus(product, distributionPaths[product].c_str());
        ASSERT_TRUE(isSuccess(result));
    }

    // TODO: verify - versig
    for (auto product : wantedProducts)
    {
        std::string distributePath = distributionPaths[product];
        // fork
        // exec versig
    }


    // TODO: install
    for (auto product : wantedProducts)
    {
        std::string distributePath = distributionPaths[product];
        result = SU_getDistributionStatus(product, distributePath.c_str());
        if (result == SU_Result_OK)
        {
            std::string installsh = distributePath+"/installdummy.sh"; // prevent install.sh running until we want to
            std::string command = std::string("bash ") + installsh;
            P("Running "<<command);
            system(command.c_str());
            // fork
            // exec install.sh
        }
    }

    return 0;
}

int main_entry()
{
    SULInit init;
    SULSession session;
    if (session.m_session == nullptr)
    {
        return 10;
    }
    return run(session.m_session);
}