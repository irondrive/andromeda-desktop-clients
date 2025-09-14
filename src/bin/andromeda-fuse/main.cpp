
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
using AndromedaFuse::Options;
#include "andromeda-fuse/FuseAdapter.hpp"
using AndromedaFuse::FuseAdapter;
#include "andromeda-fuse/FuseOptions.hpp"
using AndromedaFuse::FuseOptions;

#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/account/Session.hpp"
using Andromeda::Account::Session;
#include "andromeda/backend/BackendException.hpp"
using Andromeda::Backend::BackendException;
#include "andromeda/backend/BaseRunner.hpp"
using Andromeda::Backend::BaseRunner;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/CLIRunner.hpp"
using Andromeda::Backend::CLIRunner;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;
#include "andromeda/backend/RunnerPool.hpp"
using Andromeda::Backend::RunnerPool;

#include "andromeda/filesystem/Folder.hpp"
using Andromeda::Filesystem::Folder;
#include "andromeda/filesystem/FSResource.hpp"
using Andromeda::Filesystem::FSResource;
#include "andromeda/filesystem/folders/PlainFolder.hpp"
using Andromeda::Filesystem::Folders::PlainFolder;
#include "andromeda/filesystem/folders/Filesystem.hpp"
using Andromeda::Filesystem::Folders::Filesystem;
#include "andromeda/filesystem/folders/SuperRoot.hpp"
using Andromeda::Filesystem::Folders::SuperRoot;
#include "andromeda/filesystem/filedata/CacheManager.hpp"
using Andromeda::Filesystem::Filedata::CacheManager;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;
#include "andromeda/filesystem/filedata/CachingAllocator.hpp"
using Andromeda::Filesystem::Filedata::CachingAllocator;

enum class ExitCode : uint8_t
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT,
    FUSE_INIT
};

int main(int argc, char** argv)
{
    Debug debug("main",nullptr); 
    
    ConfigOptions configOptions;
    HTTPOptions httpOptions;
    RunnerOptions runnerOptions;
    CacheOptions cacheOptions;
    FuseOptions fuseOptions;

    Options options(configOptions, httpOptions, runnerOptions, cacheOptions, fuseOptions);

    try
    {
        options.ParseConfig("libandromeda");
        options.ParseConfig("andromeda-fuse");

        options.ParseArgs(static_cast<size_t>(argc), argv);

        options.Validate();
    }
    catch (const Options::ShowHelpException& ex)
    {
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::ShowVersionException& ex)
    {
        std::cout << "version: " << ANDROMEDA_VERSION << std::endl;
        FuseAdapter::ShowVersionText();
        return static_cast<int>(ExitCode::SUCCESS);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }

    DDBG_INFO("()");

    std::unique_ptr<BaseRunner> runner;
    switch (options.GetApiType())
    {
        case Options::ApiType::API_URL:
        {
            const std::string userAgent(std::string("andromeda-fuse/")
                +ANDROMEDA_VERSION+"/"+SYSTEM_NAME);

            runner = std::make_unique<HTTPRunner>(options.GetApiPath(),
                userAgent, runnerOptions, httpOptions);
        }; break;
        case Options::ApiType::API_PATH:
        {
            runner = std::make_unique<CLIRunner>(
                options.GetApiPath(), runnerOptions);
        }; break;
        case Options::ApiType::API_INVALID: break; // can't happen due to Validate() call
    }

    RunnerPool runners(*runner, configOptions.runnerPoolSize);
    
    std::unique_ptr<CacheManager> cacheMgr;
    std::unique_ptr<CachingAllocator> pageAlloc;
    if (cacheOptions.disable)
        pageAlloc = std::make_unique<CachingAllocator>(0);
    else cacheMgr = std::make_unique<CacheManager>(cacheOptions, false); // don't start thread yet

    // these must be after cacheMgr/runners!
    std::unique_ptr<BackendImpl> backend;
    std::unique_ptr<Session> session;

    std::unique_ptr<FSResource> fsResource;
    std::unique_ptr<Folder> folder;
    
    try
    {
        backend = std::make_unique<BackendImpl>(configOptions, runners);

        if (options.HasSession())
            session = std::make_unique<Session>(Session::FromExisting(*backend, options.GetSessionID(), options.GetSessionKey()));
        else if (options.HasUsername())
        {
            if (backend->RequiresSession() || options.GetForceSession() || !options.GetPassword().empty())
            {
                if (options.isQuiet())
                    session = std::make_unique<Session>(Session::Create(*backend, options.GetUsername(), options.GetPassword()));
                else session = std::make_unique<Session>(Session::CreateInteractive(*backend, options.GetUsername(), options.GetPassword()));
            }
            else backend->SetSudoUsername(options.GetUsername());
        }

        backend->SetSession(session.get());

        fsResource = std::make_unique<FSResource>(*backend, cacheMgr.get(),
            cacheMgr ? cacheMgr->GetPageAllocator() : *pageAlloc);

        switch (options.GetMountRootType())
        {
            case Options::RootType::SUPERROOT:
                folder = std::make_unique<SuperRoot>(*fsResource); break;
            case Options::RootType::STORAGE:
                folder = Filesystem::LoadByID(*fsResource, options.GetMountItemID()); break;
            case Options::RootType::FOLDER:
                folder = PlainFolder::LoadByID(*fsResource, options.GetMountItemID()); break;
        }
    }
    catch (const BackendException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    runner->EnableRetry(); // no retries during init

    try
    {
        FuseAdapter fuseAdapter(options.GetMountPath(), *folder, fuseOptions);

        // In either case, StartFuse() will block until unmounted
        if (options.isForeground())
        {
            if (cacheMgr) cacheMgr->StartThreads();
            fuseAdapter.StartFuse(
                FuseAdapter::RunMode::FOREGROUND);
        }
        else
        { // daemonize kills threads, start cacheMgr in the callback
            fuseAdapter.StartFuse(
                FuseAdapter::RunMode::DAEMON,
                [&](){ if (cacheMgr) cacheMgr->StartThreads(); });
        }
    }
    catch (const FuseAdapter::Exception& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::FUSE_INIT);
    }

    DDBG_INFO(": returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
