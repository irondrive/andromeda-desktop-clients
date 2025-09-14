
#include "BackendContext.hpp"

#include "andromeda/account/Session.hpp"
using Andromeda::Account::Session;
#include "andromeda/account/SessionStore.hpp"
using Andromeda::Account::SessionStore;
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;
#include "andromeda/backend/HTTPRunner.hpp"
using Andromeda::Backend::HTTPRunner;
#include "andromeda/backend/RunnerPool.hpp"
using Andromeda::Backend::RunnerPool;
#include "andromeda/database/ObjectDatabase.hpp"
using Andromeda::Database::ObjectDatabase;

namespace AndromedaGui {

/*****************************************************/
BackendContext::BackendContext(
    const std::string& url, const std::string& username, 
    const std::string& password, const std::string& twofactor) : 
    mDebug(__func__,this) 
{
    MDBG_INFO("(url:" << url << ", username:" << username << ")");

    InitializeBackend(url);

    mSession = std::make_unique<Session>(Session::Create(*mBackend, username, password, twofactor));
    mBackend->SetSession(mSession.get());
    mRunner->EnableRetry(); // no retry during init
}

/*****************************************************/
BackendContext::BackendContext(SessionStore& session) : 
    mDebug(__func__,this), mSessionStore(&session)
{
    MDBG_INFO("(url:" << session.GetServerUrl() << ")");

    InitializeBackend(session.GetServerUrl());

    mSession = std::make_unique<Session>(Session::FromExisting(*mBackend, session));
    mBackend->SetSession(mSession.get());
    mRunner->EnableRetry(); // no retry during init
}

/*****************************************************/
BackendContext::~BackendContext()
{
    MDBG_INFO("()");
}

/*****************************************************/
std::string BackendContext::GetName(bool human) const
{
    std::string hostname { mRunners->GetUnlocked().GetHostname() };
    const std::string username { mSession->GetUsername() };

    if (username.empty()) return hostname;
    
    if (human) return username+" on "+hostname;
    else return hostname+"_"+username;
}

/*****************************************************/
void BackendContext::StoreSession(ObjectDatabase& objdb)
{
    mSessionStore = &SessionStore::Create(objdb, mRunner->GetFullURL(), *mSession);

    mSessionStore->Save(); // store to DB
    mSession->SetTemporary(false);
}

/*****************************************************/
void BackendContext::InitializeBackend(const std::string& url)
{
    mRunner = std::make_unique<HTTPRunner>(url, 
        GetUserAgent(), mRunnerOptions, mHttpOptions);

    mRunners = std::make_unique<RunnerPool>(*mRunner, mConfigOptions.runnerPoolSize);

    mBackend = std::make_unique<BackendImpl>(mConfigOptions, *mRunners);
}

/*****************************************************/
std::string BackendContext::GetUserAgent()
{
    return std::string("andromeda-gui/")+ANDROMEDA_VERSION+"/"+SYSTEM_NAME;
}

} // namespace AndromedaGui
