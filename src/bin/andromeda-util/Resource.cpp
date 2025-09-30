#include "Resource.hpp"
#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/account/Session.hpp"
using Andromeda::Account::Session;
#include "andromeda/account/SessionOptions.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/CLIRunner.hpp"
using Andromeda::Backend::CLIRunner;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/RunnerPool.hpp"
using Andromeda::Backend::RunnerPool;

namespace AndromedaUtil {

Resource::Resource(Options& options_): options(options_){ } 
Resource::~Resource() = default;

/*****************************************************/
BackendImpl& Resource::GetBackend()
{
    if (backend == nullptr)
    {
        switch (options.apiType)
        {
            case Options::ApiType::API_URL:
            {
                const std::string userAgent(std::string("andromeda-util/")
                    +ANDROMEDA_VERSION+"/"+SYSTEM_NAME);

                runner = std::make_unique<HTTPRunner>(options.apiPath,
                    userAgent, options.runnerOptions, options.httpOptions);
            }; break;
            case Options::ApiType::API_PATH:
            {
                runner = std::make_unique<CLIRunner>(
                    options.apiPath, options.runnerOptions);
            }; break;
            case Options::ApiType::API_INVALID:
                throw BaseOptions::MissingOptionException("apiurl/apipath");
        }

        runnerPool = std::make_unique<RunnerPool>(*runner, 1);
        backend = std::make_unique<BackendImpl>(*runnerPool);
    }

    return *backend;
}

/*****************************************************/
Session* Resource::TryGetSession()
{
    (void)GetBackend(); // initialize

    if (session == nullptr)
    {
        session = options.sessionOptions.GetSession(*backend, !options.isQuiet());
        if (session != nullptr)
            backend->SetSession(session.get());
        else if (!options.sessionOptions.username.empty())
            backend->SetSudoUsername(options.sessionOptions.username);
    }

    return session.get();
}
    
} // namespace AndromedaUtil