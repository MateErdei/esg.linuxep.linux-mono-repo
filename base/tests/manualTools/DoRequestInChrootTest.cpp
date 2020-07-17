#include <modules/CommsComponent/SplitProcesses.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/FileSystem/IFilePermissions.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

#include <Common/HttpSenderImpl/HttpSender.h>
#include <Common/HttpSenderImpl/RequestConfig.h>

#include <Common/SecurityUtils/ProcessSecurityUtils.h>
#include <Common/FileSystemImpl/FilePermissionsImpl.h>


void printUsageAndExit(std::string name, int code)
{
    std::cerr << "Usage: " << name << " usernameOfParent  usernameOfChild  url" << std::endl;
    exit(code);

}

using ReadOnlyMount = std::pair<std::string, std::string>;

void setUpCurlDependencies(const std::string& chrootDir, CommsComponent::UserConf childConf);

void mountDependenciesReadOnly(const std::string& chrootDir, CommsComponent::UserConf userConf,
                               std::vector<ReadOnlyMount> dependencies);

int main(int argc, char *argv[])
{
    using namespace CommsComponent;
    std::string parentRoot{"/tmp/testdorequest"};
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, parentRoot);

    std::string progName{argv[0]};
    if (argc != 4)
    {
        std::cerr << "Incorrect number of arguments" << std::endl;
        printUsageAndExit(progName, 1);
    }
    auto fs = Common::FileSystem::fileSystem();


    std::string networkChrootDir{Common::FileSystem::join(parentRoot, "jail")};


    CommsComponent::UserConf parentConf{argv[1], argv[1], "logparent"};
    CommsComponent::UserConf childConf{argv[2], argv[2], "logchild"};
    std::string url{argv[3]};


    if (fs->exists(parentRoot))
    {
        std::cerr << "Warning: Directory parentRoot already exists" << std::endl;
    }
    else
    {
        fs->makedirs(parentRoot);
        auto fp = Common::FileSystem::filePermissions();

        fp->chown(parentRoot, parentConf.userName, parentConf.userGroup);
        fp->chmod(parentRoot, 0770);
    }

    if (fs->exists(networkChrootDir))
    {
        std::cerr << "Warning: Directory networkChrootDir already exists" << std::endl;
    }
    else
    {
        fs->makedirs(networkChrootDir);
        auto fp = Common::FileSystem::filePermissions();

        fp->chown(networkChrootDir, childConf.userName, childConf.userGroup);
        fp->chmod(networkChrootDir, 0770);
    }
    setUpCurlDependencies(networkChrootDir, childConf);


    //fs->copyFileAndSetPermissions(execPath, targetExec, 0777, childConf.userName, childConf.userGroup);
    CommsComponent::CommsConfigurator configurator(networkChrootDir, childConf, parentConf,
                                                   std::vector<ReadOnlyMount>());

    auto childProc = [&](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi&/*childProxy*/) {
        std::cout << "Running child request" << std::endl;
        std::shared_ptr<Common::HttpSenderImpl::HttpSender> httpSender;
        std::vector<std::string> additionalHeaders;

        std::shared_ptr<Common::HttpSender::ICurlWrapper> curlWrapper =
                std::make_shared<Common::HttpSenderImpl::CurlWrapper>();

        httpSender = std::make_shared<Common::HttpSenderImpl::HttpSender>(curlWrapper);

        std::string m_defaultCertPath;

        Path defaultCertPathAfterGoogle = "/certs/ca-certificates.crt";

        Common::HttpSenderImpl::RequestConfig req(Common::HttpSenderImpl::RequestType::GET, additionalHeaders, url, 443,
                                                  defaultCertPathAfterGoogle,
                                                  "");

        try
        {
            auto after_chroot = httpSender->doHttpsRequest(req);
            std::cout << "Going to do request" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "return code after_chroot" << after_chroot << std::endl;
        }
        catch (const std::exception& ex)
        {
            std::cout << "do request exceptiont" << ex.what() << std::endl;
        }
    };

    auto parentProc = [](std::shared_ptr<MessageChannel> channel, OtherSideApi& childProxy) {
        childProxy.pushMessage("dorequest");
        std::string msg;
        channel->pop(msg, std::chrono::milliseconds(2000));
        std::cout << "Parent proc executed" << std::endl;
    };
    std::cout << "Running the splitProcesses Reactors" << std::endl;
    int code = CommsComponent::splitProcessesReactors(parentProc, std::move(childProc), configurator);
    std::cout << "Return code: " << code << std::endl;
    return 0;
}

void setUpCurlDependencies(const std::string& chrootDir, CommsComponent::UserConf childConf)
{

//    auto fs = Common::FileSystem::fileSystem();
//    auto ifperms = Common::FileSystem::FilePermissionsImpl();

    std::vector<std::pair<std::string, std::string>> mountSources = {
            {"/lib",                               "lib"},
            {"/usr/lib",                           "usr/lib"},
            {"/etc/ssl/certs/ca-certificates.crt", "certs/ca-certificates.crt"},
            {"/etc/hosts",                         "etc/hosts"},
            {"/etc/resolv.conf",                   "etc/resolv.conf"}};

    mountDependenciesReadOnly(chrootDir, childConf, mountSources);
//    for (const auto& target : mountSources)
//    {
//        auto sourcePath = target.first;
//        auto targetPath = Common::FileSystem::join(chrootDir, target.second);
//
//        //attempting to mount to an existing file will overwrite
//        if(fs->exists(targetPath))
//        {
//            std::stringstream errorMessage;
//            if(Common::SecurityUtils::isAlreadyMounted(targetPath))
//            {
//                errorMessage << "Source '" << sourcePath << "' is already mounted on '" << targetPath;
//                std::cerr << errorMessage.str() << std::endl;
//                if(Common::SecurityUtils::unMount(targetPath))
//                {
//                    //failed to unmount this is bad
//                    fs->removeFileOrDirectory(targetPath);
//                }
//            }
//            //this could be anything do not mount ontop
//            throw;
//        }
//
//        if (fs->isFile(sourcePath))
//        {
//
//            auto dirpath = Common::FileSystem::dirName(targetPath);
//            std::cout << "dirname " << dirpath << std::endl;
//            fs->makedirs(dirpath);
//            fs->writeFile(targetPath, "");
//            ifperms.chown(targetPath, childConf.userName, childConf.userGroup); //test-spl-user //test-spl-grp
//            ifperms.chmod(dirpath, 0755);
//
//        }
//        else if(fs->isDirectory(sourcePath))
//        {
//            fs->makedirs(targetPath);
//            ifperms.chown(targetPath, childConf.userName, childConf.userGroup);
//            ifperms.chmod(targetPath, 0700);
//        }
//        Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath);
//    }
//
//    //make fake /tmp
//
//    auto fakeTmp = Common::FileSystem::join(chrootDir, "tmp");
//    fs->makedirs(fakeTmp);
//    ifperms.chown(fakeTmp, childConf.userName, childConf.userGroup);
//    ifperms.chmod(fakeTmp, 0700);
}

void mountDependenciesReadOnly(const std::string& chrootDir, CommsComponent::UserConf userConf,
                               std::vector<ReadOnlyMount> dependencies)
{
    auto fs = Common::FileSystem::fileSystem();
    auto ifperms = Common::FileSystem::FilePermissionsImpl();

    for (const auto& target : dependencies)
    {
        auto sourcePath = target.first;
        auto targetPath = Common::FileSystem::join(chrootDir, target.second);

        //attempting to mount to an existing file will overwrite
        if (fs->exists(targetPath)&&!Common::SecurityUtils::isFreeMountLocation(targetPath))
        {
            std::stringstream errorMessage;
            if (Common::SecurityUtils::isAlreadyMounted(targetPath))
            {
                errorMessage << "Source '" << sourcePath << "' is already mounted on '" << targetPath;
                std::cerr << errorMessage.str() << std::endl;
                continue;
            }
            //this could be anything do not mount ontop
            perror("file without the expected content found in the mount location");
            exit(EXIT_FAILURE);
        }

        if (fs->isFile(sourcePath))
        {
            auto dirpath = Common::FileSystem::dirName(targetPath);
            std::cout << "dirname " << dirpath << std::endl;
            fs->makedirs(dirpath);
            fs->writeFile(targetPath, "");
            ifperms.chown(targetPath, userConf.userName, userConf.userGroup); //test-spl-user //test-spl-grp
            ifperms.chmod(dirpath, 0755);
        }

        else if (fs->isDirectory(sourcePath))
        {
            fs->makedirs(targetPath);
            ifperms.chown(targetPath, userConf.userName, userConf.userGroup);
            ifperms.chmod(targetPath, 0700);
        }
        Common::SecurityUtils::bindMountReadOnly(sourcePath, targetPath);
    }
}