
#include "BackendContext.hpp"

#include "andromeda/SecureBuffer.hpp"
using Andromeda::SecureBuffer;
#include "andromeda/account/Account.hpp"
using Andromeda::Account::Account;
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
    const std::string& url, const std::string& username, const Andromeda::SecureBuffer& password, 
    const Andromeda::SecureBuffer& e2ee_recovery, const std::string& twofactor) : 
    mDebug(__func__,this) 
{
    MDBG_INFO("(url:" << url << ", username:" << username << ")");

    InitializeBackend(url);

    mSession = std::make_unique<Session>(Session::Create(*mBackend, username, password, twofactor));
    mBackend->SetSession(mSession.get());
    mRunner->EnableRetry(); // no retry during init

    Account& account { mSession->GetAccount() };
    if (account.HasE2eeKeys())
    {
        if (account.HasE2eePwKey() && e2ee_recovery.empty())
            account.UnlockE2eeFromPassword(password);
        else account.UnlockE2eeFromRecovery(Account::DecodeRecoveryKey(e2ee_recovery));
    }
}

/*****************************************************/
BackendContext::BackendContext(SessionStore& sessionStore) : 
    mDebug(__func__,this), mSessionStore(&sessionStore)
{
    MDBG_INFO("(url:" << sessionStore.GetServerUrl() << ")");

    InitializeBackend(sessionStore.GetServerUrl());

    mSession = std::make_unique<Session>(Session::FromExisting(*mBackend, sessionStore));
    mBackend->SetSession(mSession.get());
    mRunner->EnableRetry(); // no retry during init

    Account& account { mSession->GetAccount() };
    if (account.HasE2eeKeys())
    {
        const SecureBuffer* masterkey { sessionStore.TryGetMasterKey() };
        if (masterkey) account.UnlockE2eeDirectly(*masterkey);
        else MDBG_ERROR("... no master key in the session, running without e2ee!");
    }
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
    const std::string username { mSession->GetAccount().GetUsername() };

    if (username.empty()) return hostname;
    
    if (human) return username+" on "+hostname;
    else return hostname+"_"+username;
}

/*****************************************************/
void BackendContext::StoreSession(ObjectDatabase& objdb)
{
    mSessionStore = &SessionStore::Create(objdb, mRunner->GetFullURL(), *mSession);
    mSession->GetAccount().StoreMasterKey(*mSessionStore);

    mSessionStore->Save(); // store to DB
    mSession->SetTemporary(false);
}

/*****************************************************/
void BackendContext::InitializeBackend(const std::string& url)
{
    mRunner = std::make_unique<HTTPRunner>(url, 
        GetUserAgent(), mRunnerOptions, mHttpOptions);

    mRunners = std::make_unique<RunnerPool>(*mRunner, mRunnerOptions.poolSize);
    mBackend = std::make_unique<BackendImpl>(*mRunners);
}

/*****************************************************/
std::string BackendContext::GetUserAgent()
{
    return std::string("andromeda-gui/")+ANDROMEDA_VERSION+"/"+SYSTEM_NAME;
}

} // namespace AndromedaGui
