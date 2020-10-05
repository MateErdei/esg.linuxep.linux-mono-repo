#include "sophos_threat_detector/threat_scanner/SusiWrapperFactory.cpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#define BASE "/tmp/TestSusiWrapperFactory/chroot/"
using namespace threat_scanner;

void setupFilesForTestingGlobalRep()
{
    auto& appConfig = Common::ApplicationConfiguration::applicationConfiguration();
    appConfig.setData("SOPHOS_INSTALL", BASE);
    fs::path fakeBaseInstallation = BASE;
    fs::path fakeChroot =  fakeBaseInstallation;
    fs::path fakeEtcDirectory  = fakeChroot / "base/etc/";
    fs::path fakeMcsDirectory = fakeChroot / "base/update/var/";

    fs::create_directories(fakeEtcDirectory);
    fs::create_directories(fakeMcsDirectory);

    std::ofstream machineIdFile;
    machineIdFile.open(fakeEtcDirectory / "machine_id.txt");
    ASSERT_TRUE(machineIdFile.good());
    machineIdFile << "ab7b6758a3ab11ba8a51d25aa06d1cf4";
    machineIdFile.close();

    std::string alcContents = R"sophos({
 "sophosURLs": [
  "http://dci.sophosupd.com/update",
  "http://dci.sophosupd.net/update"
 ],
 "credential": {
  "username": "5af36d92f3773a9083bdea545230a507",
  "password": "5af36d92f3773a9083bdea545230a507"
 },
 "proxy": {
  "url": "",
  "credential": {
   "username": "",
   "password": "",
   "proxyType": ""
  }
 },
 "certificatePath": "/opt/sophos-spl/base/update/certs",
 "installationRootPath": "/opt/sophos-spl",
 "systemSslPath": ":system:",
 "cacheUpdateSslPath": "",
 "installArguments": [
  "--instdir",
  "/opt/sophos-spl"
 ],
 "logLevel": "VERBOSE",
 "manifestNames": [
  "manifest.dat"
 ],
 "primarySubscription": {
  "rigidName": "ServerProtectionLinux-Base",
  "baseVersion": "",
  "tag": "RECOMMENDED",
  "fixedVersion": ""
 },
 "products": [
  {
   "rigidName": "ServerProtectionLinux-Plugin-AV",
   "baseVersion": "",
   "tag": "RECOMMENDED",
   "fixedVersion": ""
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-EDR",
   "baseVersion": "",
   "tag": "RECOMMENDED",
   "fixedVersion": ""
  },
  {
   "rigidName": "ServerProtectionLinux-Plugin-MDR",
   "baseVersion": "",
   "tag": "RECOMMENDED",
   "fixedVersion": ""
  }
 ],
 "features": [
  "APPCNTRL",
  "AV",
  "CORE",
  "DLP",
  "DVCCNTRL",
  "EFW",
  "HBT",
  "LIVEQUERY",
  "LIVETERMINAL",
  "MDR",
  "MTD",
  "NTP",
  "SAV",
  "SDU",
  "WEBCNTRL"
 ]
})sophos";

    std::ofstream customerIdFileStream;
    customerIdFileStream.open(fakeMcsDirectory / "update_config.json");
    ASSERT_TRUE(customerIdFileStream.good());
    customerIdFileStream << alcContents;
    customerIdFileStream.close();
}

TEST(TestSusiWrapperFactory, getCustomerIdReturnsUnknown) // NOLINT
{
    EXPECT_EQ(getCustomerId(),"c1cfcf69a42311a6084bcefe8af02c8a");
}

TEST(TestSusiWrapperFactory, getEndpointIdReturnsUnknown) // NOLINT
{
    EXPECT_EQ(getEndpointId(),"66b8fd8b39754951b87269afdfcb285c");
}

TEST(TestSusiWrapperFactory, getEndpointIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getEndpointId(),"ab7b6758a3ab11ba8a51d25aa06d1cf4");
}

TEST(TestSusiWrapperFactory, geCustomerIdReturnsId) // NOLINT
{
    setupFilesForTestingGlobalRep();
    EXPECT_EQ(getCustomerId(),"d22829d94b76c016ec4e04b08baeffaa");
}