
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
using AndromedaUtil::Options;
#include "Actions.hpp"
using AndromedaUtil::Actions;

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/account/Session.hpp"
using Andromeda::Account::Session;
#include "andromeda/account/SessionOptions.hpp"
using Andromeda::Account::SessionOptions;
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

enum class ExitCode : uint8_t
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT,
    ACTION_FAIL
};

int main(int argc, char** argv)
{
    Debug debug("main",nullptr); 
    
    ConfigOptions configOptions;
    HTTPOptions httpOptions;
    RunnerOptions runnerOptions;
    SessionOptions sessionOptions;

    Options options(configOptions, httpOptions, runnerOptions, sessionOptions);

    try
    {
        options.ParseConfig("libandromeda");
        options.ParseConfig("andromeda-util");
        const size_t args = options.ParseArgs(static_cast<size_t>(--argc), ++argv, true);
        argc -= static_cast<int>(args); argv += args;
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
            const std::string userAgent(std::string("andromeda-util/")
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

    RunnerPool runners(*runner, 1); // no multithreading
    std::unique_ptr<BackendImpl> backend;
    std::unique_ptr<Session> session;
    // TODO RAY !! do we really need to do a GetConfig call here? probably for versioning - maybe andromeda-cli could do it too, given an option? not by default
    
    try
    {
        backend = std::make_unique<BackendImpl>(configOptions, runners);

        session = sessionOptions.GetSession(*backend, !options.isQuiet());
        if (session)
            backend->SetSession(session.get());
        else if (!sessionOptions.username.empty())
            backend->SetSudoUsername(sessionOptions.username);
    }
    catch (const BackendException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    try
    {
        Actions actions(backend.get(), session.get());
        actions.RunAction(argc, argv);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }
    catch (const BaseException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::ACTION_FAIL);
    }

    DDBG_INFO(": returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
