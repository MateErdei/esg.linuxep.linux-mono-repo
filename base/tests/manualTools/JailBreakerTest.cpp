#include <modules/CommsComponent/SplitProcesses.h>
#include <modules/Common/FileSystem/IFileSystem.h>
#include <modules/Common/FileSystem/IFilePermissions.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

void printUsageAndExit(std::string name, int code)
{
        std::cerr <<  "Usage: " << name << " usernameOfParent  usernameOfChild  pathOfExecutable" << std::endl; 
        exit(code); 

}

/* Usage: ./JailBreakerTest usernameOfParent  usernameOfChild  pathOfExecutable
*    The jail will be placed at /tmp/testJail
*    And the group for the child and parent will be the same and set to sophos-spl-group
*    Executables need to be compiled as static binaries as the libraries for loading is not 'yet' configured (JIRA)
*    Consider some examples in https://github.com/earthquake/chw00t 
*
*    For example, create a basic hello.cpp

#include <iostream>
#include <fstream>
int main()
{
    std::cout << "Running" << std::endl;
    std::fstream out{"hello.txt", std::fstream::out};
    out << "from child" << std::endl;

    return 0;
}

*     build it: g++ hello.cpp -o hello -static
*     ./JailBreakerTest  pair pair  /tmp/hello
*/
int main(int argc, char * argv[])
{
    using namespace CommsComponent; 
    std::string parentRoot{"/tmp/testBreak"};
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, parentRoot);

    std::string progName{argv[0]};
    if (argc != 4)
    {
        std::cerr << "Incorrect number of arguments" << std::endl; 
        printUsageAndExit(progName, 1);         
    }
    auto fs = Common::FileSystem::fileSystem(); 

    std::string execPath{argv[3]};
    if ( !fs->isExecutable(execPath))
    {
        std::cerr << execPath << " is not an executable" << std::endl; 
        printUsageAndExit(progName, 2); 
    }    

    
    std::string jailRoot{Common::FileSystem::join(parentRoot, "jail")}; 
    std::string jailBreaker{"jailBreaker"};
    std::string targetExec{ Common::FileSystem::join(jailRoot, jailBreaker)};

    std::string group{"sophos-spl-group"};

    CommsComponent::UserConf parentConf{argv[1], group, "logparent" }; 
    CommsComponent::UserConf childConf{argv[2], group, "logchild"}; 


    if (fs->exists(parentRoot))
    {
        std::cerr << "Warning: Directory parentRoot already exists" << std::endl; 
    }
    else
    {
        fs->makedirs(parentRoot); 
        auto fp = Common::FileSystem::filePermissions(); 

        fp->chown(parentRoot, parentConf.userName, group); 
        fp->chmod(parentRoot, 0770); 
    }

    if (fs->exists(jailRoot))
    {
        std::cerr << "Warning: Directory jailRoot already exists" << std::endl; 
    }
    else
    {
        fs->makedirs(jailRoot); 
        auto fp = Common::FileSystem::filePermissions(); 

        fp->chown(jailRoot, childConf.userName, group); 
        fp->chmod(jailRoot, 0770); 
    }


    fs->copyFileAndSetPermissions(execPath, targetExec, 0777, childConf.userName, childConf.userGroup); 
    CommsComponent::CommsConfigurator configurator(jailRoot, childConf, parentConf,
                                                std::vector<ReadOnlyMount>() ); 

    auto childProc = [jailBreaker](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi &/*childProxy*/){
        std::string jailBreakerPath = Common::FileSystem::join("/",jailBreaker); 
        std::cout << "Running jail breaker" << std::endl; 
        char *newargv[] = { jailBreakerPath.data(), NULL };
        int code =  execv(jailBreakerPath.c_str(),newargv); 
        if (code != 0)
        {
            perror("Failed to run system"); 
        }
        return code; 
    };

    auto parentProc = [](std::shared_ptr<MessageChannel>/*channel*/, OtherSideApi &/*childProxy*/){
        std::cout << "Parent proc executed" << std::endl; 
        return 0; 
    };
    std::cout << "Running the splitProcesses Reactors" << std::endl; 
    int code =  CommsComponent::splitProcessesReactors(parentProc, std::move(childProc), configurator); 
    std::cout << "Return code: " << code << std::endl; 
    return 0; 
}