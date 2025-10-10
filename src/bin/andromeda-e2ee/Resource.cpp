#include "nlohmann/json.hpp"

#include "Resource.hpp"
#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/account/Account.hpp"
using Andromeda::Account::Account;
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

namespace AndromedaE2ee {

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
                const std::string userAgent(std::string("andromeda-e2ee/")
                    +ANDROMEDA_VERSION+"/"+SYSTEM_NAME);

                runner = std::make_unique<HTTPRunner>(options.apiUrl,
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
        session = options.sessionOptions.GetSession(*backend, !options.mQuiet);
        if (session != nullptr) backend->SetSession(session.get());
    }

    return session.get();
}

/*****************************************************/
Session& Resource::GetSession()
{
    (void)GetBackend(); // initialize
    options.sessionOptions.forceSession = true;

    if (session == nullptr)
    {
        session = options.sessionOptions.GetSession(*backend, !options.mQuiet);
        if (session == nullptr) throw Options::MissingOptionException("username/sessionid");

        backend->SetSession(session.get());
    }

    return *session;
}

/*****************************************************/
Account& Resource::GetAccount()
{
    (void)GetBackend(); // initialize

    if (account != nullptr)
        return *account;
    else if (session != nullptr)
        return session->GetAccount();

    session = options.sessionOptions.GetSession(*backend, !options.mQuiet);

    if (session != nullptr)
    {
        backend->SetSession(session.get());
        return session->GetAccount();
    }
    else if (!options.sessionOptions.username.empty())
    {
        backend->SetSudoUsername(options.sessionOptions.username);
        // NOTE don't use { } to initialize data, nlohmann assumes you want an array
        const nlohmann::json data = backend->GetAccount();

        account = std::make_unique<Account>(*backend, data);
        return *account;
    }
    else throw Options::MissingOptionException("username/sessionid");
}
   
} // namespace AndromedaE2ee