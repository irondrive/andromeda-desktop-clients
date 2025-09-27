
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
#include "Resource.hpp"
using AndromedaUtil::Resource;

#include "andromeda/BaseException.hpp"
using Andromeda::BaseException;
#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/Debug.hpp"
using Andromeda::Debug;
#include "andromeda/account/SessionOptions.hpp"
using Andromeda::Account::SessionOptions;
#include "andromeda/backend/BackendException.hpp"
using Andromeda::Backend::BackendException;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;

enum class ExitCode : uint8_t
{
    SUCCESS,
    BAD_USAGE,
    BACKEND_FAIL,
    OTHER_FAIL
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

    try
    {
        Resource resource(options);
        Actions actions(resource);
        actions.RunAction(argc, argv);
    }
    catch (const Options::Exception& ex)
    {
        std::cout << ex.what() << std::endl << std::endl;
        std::cout << Options::HelpText() << std::endl;
        return static_cast<int>(ExitCode::BAD_USAGE);
    }    
    catch (const BackendException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::BACKEND_FAIL);
    }
    catch (const BaseException& ex)
    {
        std::cout << ex.what() << std::endl;
        return static_cast<int>(ExitCode::OTHER_FAIL);
    }

    DDBG_INFO(": returning success...");
    return static_cast<int>(ExitCode::SUCCESS);
}
