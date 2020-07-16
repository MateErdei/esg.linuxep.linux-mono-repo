/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <iostream>
#include <chrono>
#include <thread>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <string>
#include <fstream>
#include <vector>
#include <future>

/**
 * This executable induces various "bad things" when given the corresponding argument
 */

namespace
{
    std::mutex deadlockMutex;

    const char *loremIpsum =
            "\n"
            "\n"
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Phasellus tristique nulla ipsum, "
            "eget varius erat iaculis sit amet. Fusce nisi nibh, feugiat vehicula bibendum ac, lobortis "
            "quis sem. Cras vel laoreet augue. Praesent molestie vitae sem vel vulputate. Praesent nec condimentum "
            "augue, vel lacinia nulla. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere "
            "cubilia Curae; Vestibulum molestie neque vitae cursus molestie. Donec risus sapien, condimentum "
            "at vulputate vitae, accumsan in sem. Praesent faucibus porta diam, vel sollicitudin nisi cursus non.\n"
            "\n"
            "Etiam aliquam nulla lectus, eu pharetra felis sollicitudin eget. Suspendisse orci leo, convallis "
            "at ultrices ut, molestie at quam. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices "
            "posuere cubilia Curae; Quisque sed leo elementum, sodales diam a, tincidunt arcu. Integer id leo sit "
            "amet erat feugiat blandit vitae in quam. Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "Fusce consectetur, risus at commodo scelerisque, tortor dolor maximus risus, vel hendrerit sapien "
            "augue et lorem. Vestibulum dignissim mauris in risus molestie pellentesque. Phasellus mauris mi, "
            "malesuada non nisl sit amet, lobortis vulputate turpis.\n"
            "\n"
            "Integer suscipit purus ligula, ac imperdiet est interdum vel. Orci varius natoque penatibus "
            "et magnis dis parturient montes, nascetur ridiculus mus. Aliquam erat volutpat. Ut sed odio "
            "a nibh dapibus feugiat at in enim. In placerat ac nulla at sagittis. Sed ante mauris, pretium "
            "ac rhoncus non, scelerisque ut eros. Nulla eget est nec turpis elementum faucibus id eu elit. "
            "Vestibulum a ligula id augue sodales imperdiet vitae in urna. Interdum et malesuada fames ac ante"
            "ipsum primis in faucibus. Proin vehicula ut eros semper ultricies. Integer nec dignissim urna. "
            "Vestibulum ac velit eget ex varius sagittis quis eget elit.\n"
            "\n"
            "Cras ac aliquam urna, non euismod nunc. Cras feugiat magna metus. Ut eu tellus vel enim lobortis"
            "vestibulum a ut est. Vivamus nec nulla nulla. Nam eget condimentum nibh. Maecenas non eleifend "
            "justo. Donec malesuada, tortor vitae accumsan gravida, lorem mi sagittis tortor, eget consequat "
            "enim mi non urna. Fusce a magna pellentesque, laoreet eros et, volutpat purus. Suspendisse potenti. "
            "Quisque nec arcu auctor, maximus nisi sed, ultricies augue. Pellentesque sed vestibulum nulla. "
            "Curabitur nisl dolor, finibus eget ante ac, commodo luctus massa. Fusce bibendum, diam nec ultrices "
            "sodales, ante erat ultrices mi, quis maximus augue dolor ut arcu. Aliquam vulputate nisl quis porta "
            "dignissim. Ut ornare volutpat purus, sed luctus neque pulvinar sed.\n"
            "\n"
            "Quisque pharetra scelerisque metus vitae posuere. Sed vulputate enim sed libero imperdiet, vitae "
            "consequat ligula faucibus. Nullam ac purus sed massa venenatis convallis non non mauris. Proin laoreet, "
            "leo a iaculis sollicitudin, nibh lorem ullamcorper nunc, at condimentum ligula ex ut ante. Phasellus id "
            "egestas nibh, sed varius nisl. Sed nec gravida sem, sit amet ullamcorper eros. Donec tincidunt, dui non"
            "mattis faucibus, nulla purus porta elit, id mollis odio ligula in nibh. Nulla sit amet bibendum est, "
            "vitae fermentum diam.";
}

std::string argumentFile = "/tmp/PickYourPoisonArgument";

int printUsageAndExit()
{
    std::cerr << "Usage: <executable> <argument>\n"
                 "OR\n"
                 "Call the executable only with an argument in a file at \"/tmp/PickYourPoisonArgument\"\n"
                 "Valid arguments\n"
                 "\t--crash         - induce a crash after 5 seconds of running\n"
                 "\t--spam          - send lots of text to stdout and stderr\n"
                 "\t--deadlock      - induce a deadlock\n"
                 "\t--gobble        - continuously append items to a vector to use up progressively more memory\n"
                 "\t--run           - just run without induced errors\n"
                 "\t--hello-world\n"
                 "\t--exit\n"
                 "\t--help          - show this dialogue\n";

    return 1;
}

bool arg_file_exists()
{
    std::ifstream myFile(argumentFile);
    if (myFile.fail())
    {
        return false;
    }
    return true;
}

std::string readFile(std::string)
{
    std::ifstream inFileStream(argumentFile, std::ios::binary);

    if (!inFileStream.good())
    {
        abort();
    }

    try
    {
        inFileStream.seekg(0, std::istream::end);
        std::ifstream::pos_type size(inFileStream.tellg());
        if (size < 0)
        {
            throw std::runtime_error(
                    "Error, Failed to read file: '/tmp/PickYourPoisonArgument', failed to get file size");
        }
        else if (static_cast<unsigned long>(size) > 1000000)
        {
            throw std::runtime_error(
                    std::string("Error, Failed to read file: '/tmp/PickYourPoisonArgument', file too large"));
        }

        inFileStream.seekg(0, std::istream::beg);

        std::string content(static_cast<std::size_t>(size), 0);
        inFileStream.read(&content[0], size);
        return content;
    }
    catch (std::system_error &ex)
    {
        throw std::runtime_error(std::string("Error, Failed to read from file '/tmp/PickYourPoisonArgument'"));
    }
}

void crash()
{
    std::this_thread::sleep_for(std::chrono::seconds(5));
    abort();
}

void exit()
{
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::terminate();
}

void deadlock()
{

    auto t1 = std::async(std::launch::async, []() {
        std::cout << "Entering deadlock" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Now in deadlock" << std::endl;
        std::lock_guard<std::mutex> lock1(deadlockMutex);
    });

    std::lock_guard<std::mutex> lock1(deadlockMutex);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    t1.get();
    std::cout << "Out of deadlock" << std::endl;
}

void spamOutErr()
{
    while (true)
    {
        std::cout << "This is stdout" << std::endl;
        std::cerr << "This is stderr" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void numSpam()
{
    for (int i = 1; i < 1000; i++)
    {
        std::cout << i << std::endl;
    }
    std::cout << "Executable will keep Running" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(300));
}

void gobbleMemory()
{
    std::string aString = "this is a string";
    std::vector<std::string> bigVector;

    std::cout << "Gobbling Memory" << std::endl;

    while (true)
    {
        bigVector.emplace_back(loremIpsum);
        std::cout << "vector size: " << bigVector.size() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void justRun()
{
    std::cout << "Executable will keep Running" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(300));
}

void helloWorld()
{
    std::cout << "Hello, world!";
}

int main(int argc, char *argv[])
{
    std::cout << "Starting PickYourPoison Fault Injection Executable" << std::endl;
    std::string argument;

    if (arg_file_exists())
    {
        argument = readFile(argumentFile);
        argument.erase(std::remove(argument.begin(), argument.end(), '\n'), argument.end());
        std::cout << "using argument: " << argument << " from " << argumentFile << std::endl;
    }
    else if (argc == 2)
    {
        argument = argv[1];
        std::cout << "using argument: " << argument << " from command line" << std::endl;
    }
    else
    {
        return printUsageAndExit();
    }

    if (argument == "--exit")
    {
        exit();
    }

    if (argument == "--crash")
    {
        crash();
    }
    else if (argument == "--spam")
    {
        spamOutErr();
    }
    else if (argument == "--num-spam")
    {
        numSpam();
    }
    else if (argument == "--deadlock")
    {
        deadlock();
    }
    else if (argument == "--gobble")
    {
        gobbleMemory();
    }
    else if (argument == "--hello-world")
    {
        helloWorld();
    }
    else if (argument == "--run")
    {
        justRun();
    }
    else if (argument == "--help")
    {
        printUsageAndExit();
    }
    else
    {
        std::cout << "unrecognised argument: " << argument << std::endl;
        printUsageAndExit();
    }
    return 0;
}
