
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;
#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda/Backend.hpp"
using Andromeda::Backend;
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/BaseRunner.hpp"
using Andromeda::BaseRunner;
#include "andromeda/CLIRunner.hpp"
using Andromeda::CLIRunner;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/HTTPRunner.hpp"
using Andromeda::HTTPRunner;
#include "andromeda/HTTPOptions.hpp"
using Andromeda::HTTPOptions;

#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;
#include "andromeda/filesystem/folders/PlainFolder.hpp"
using Andromeda::Filesystem::Folders::PlainFolder;
#include "andromeda/filesystem/folders/Filesystem.hpp"
using Andromeda::Filesystem::Folders::Filesystem;
#include "andromeda/filesystem/folders/SuperRoot.hpp"
using Andromeda::Filesystem::Folders::SuperRoot;

#define VERSION "0.1-alpha"

using AndromedaFuse::FuseAdapter;

enum class ExitCode
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT,
    FUSE_INIT
};

int main(int argc, char** argv)
{
    Debug debug("main"); 
    
    ConfigOptions configOptions;
    HTTPOptions httpOptions;
    FuseOptions fuseOptions;

    Options options(configOptions, httpOptions, fuseOptions);

    try
    {
        std::list<std::string> paths { 
            "/etc/andromeda", "/usr/local/etc/andromeda" };

        #if WIN32
            #pragma warning(push)
            #pragma warning(disable:4996) // getenv is safe in C++11
        #endif

        const char* homeDir { std::getenv("HOME") };
        if (homeDir != nullptr) paths.push_back(
            std::string(homeDir)+"/.config/andromeda");

        #if WIN32
            #pragma warning(pop)
        #endif

        paths.push_back("."); for (std::string path : paths)
        {
            path += "/andromeda-fuse.conf";

            if (std::filesystem::is_regular_file(path))
                options.ParseFile(path);
        }

        options.ParseArgs(argc, argv);

        options.Validate();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << VERSION << std::endl;
        FuseAdapter::ShowVersionText();
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    Debug::SetLevel(options.GetDebugLevel());

    debug << __func__ << "()"; debug.Info();

    std::unique_ptr<BaseRunner> runner;
    switch (options.GetApiType())
    {
        case Options::ApiType::API_URL:
        {
            HTTPRunner::HostUrlPair urlPair { 
                HTTPRunner::ParseURL(options.GetApiPath()) };

            runner = std::make_unique<HTTPRunner>(
                urlPair.first, urlPair.second, httpOptions);
        }; break;
        case Options::ApiType::API_PATH:
        {
            runner = std::make_unique<CLIRunner>(
                options.GetApiPath()); break;
        }; break;
    }

    Backend backend(configOptions, *runner);
    std::unique_ptr<Folder> folder;

    try
    {
        backend.Initialize();

        if (options.HasSession())
            backend.PreAuthenticate(options.GetSessionID(), options.GetSessionKey());
        else if (options.HasUsername())
            backend.AuthInteractive(options.GetUsername(), options.GetPassword(), options.GetForceSession());

        switch (options.GetMountRootType())
        {
            case Options::RootType::SUPERROOT:
                folder = std::make_unique<SuperRoot>(backend); break;
            case Options::RootType::FILESYSTEM:
                folder = Filesystem::LoadByID(backend, options.GetMountItemID()); break;
            case Options::RootType::FOLDER:
                folder = PlainFolder::LoadByID(backend, options.GetMountItemID()); break;
        }
    }
    catch (const BaseException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    try { (dynamic_cast<HTTPRunner&>(*runner)).EnableRetry(); }
    catch (const std::bad_cast& ex) { } // no retries during init

    try
    {
        FuseAdapter fuseAdapter(options.GetMountPath(), *folder, fuseOptions, 
            static_cast<bool>(Debug::GetLevel()) ? FuseAdapter::RunMode::FOREGROUND : FuseAdapter::RunMode::DAEMON);
    }
    catch (const FuseAdapter::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    debug.Info("returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
