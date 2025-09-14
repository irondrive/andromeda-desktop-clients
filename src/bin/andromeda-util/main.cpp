
#include <list>
#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include <cstdlib>

#include "Options.hpp"
using AndromedaUtil::Options;

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

enum class ExitCode : uint8_t
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_INIT
};

int main(int argc, char** argv)
{
    Debug debug("main",nullptr); 
    
    ConfigOptions configOptions;
    HTTPOptions httpOptions;
    RunnerOptions runnerOptions;

    Options options(configOptions, httpOptions, runnerOptions);

    try
    {
        options.ParseConfig("libandromeda");
        options.ParseConfig("andromeda-util");

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

    RunnerPool runners(*runner, configOptions);
    std::unique_ptr<BackendImpl> backend;
    std::unique_ptr<Session> session;
    // TODO RAY !! do we really need to do a GetCoreConfig call here? yes - but combine into a single call, server side
    
    try
    {
        // TODO RAY !! make an alternate BackendImpl signature that takes only 1 runner
        backend = std::make_unique<BackendImpl>(configOptions, runners);
        // TODO RAY !! some actions will need pre-auth, some not, not sure how to do this

        if (options.HasSession())
            session = std::make_unique<Session>(Session::FromExisting(*backend, options.GetSessionID(), options.GetSessionKey()));
        else if (options.HasUsername())
        {
            // TODO RAY !! make this a SessionOptions function for commonality
            if (backend->RequiresSession() || options.GetForceSession() || !options.GetPassword().empty())
            {
                if (configOptions.quiet)
                    session = std::make_unique<Session>(Session::Create(*backend, options.GetUsername(), options.GetPassword()));
                else session = std::make_unique<Session>(Session::CreateInteractive(*backend, options.GetUsername(), options.GetPassword()));
            }
            else backend->SetSudoUsername(options.GetUsername());
        }

        backend->SetSession(session.get());
    }
    catch (const BackendException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_INIT);
    }

    // TODO RAY !! implement me further - need a "action" on the command line - move to a new class
    DDBG_INFO(": action:" << options.GetAction());

    DDBG_INFO(": returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
