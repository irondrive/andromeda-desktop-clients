
#include "nlohmann/json.hpp"

#include "Account.hpp"
#include "Session.hpp"
#include "SessionStore.hpp"
#include "andromeda/StringUtil.hpp"
#include "andromeda/PlatformUtil.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda::Account {

namespace { // anonymous
Debug sDebug("Session",nullptr); // NOLINT(cert-err58-cpp)
} // anonymous namespace

// TODO nit - should take JSON for the session itself too? - may need backend GetSession function? if more metadata about the session is needed

/*****************************************************/
Session::Session(BackendImpl& backend, const nlohmann::json& account, const std::string& sessionID, const std::string& sessionKey, bool temporary):
    mDebug(__func__, this), mBackend(backend), mAccount(std::make_unique<Account>(backend, account, this)),
    mSessionID(sessionID), mSessionKey(sessionKey), mTemporary(temporary)
{
    MDBG_INFO("(sessionID:" << sessionID << ")");
}

/*****************************************************/
Session::Session(Session&& old) noexcept: // move constructor
    // NOTE whenever adding new member variables, must add them to this list! unfortunate
    mDebug(__func__, this), mBackend(old.mBackend), mAccount(std::move(old.mAccount)), mE2ee_pwsubkey(std::move(old.mE2ee_pwsubkey)),
    mSessionID(std::move(old.mSessionID)), mSessionKey(std::move(old.mSessionKey)), mTemporary(old.mTemporary)
{
    MDBG_INFO("(move old:" << & old << ")");
    old.mTemporary = false; // don't delete twice
    mAccount->SetSession(this);
}

/*****************************************************/
Session::~Session()
{
    MDBG_INFO("(sessionID:" << mSessionID << ")");
    if (!mTemporary) return; // don't delete

    try
    {
        BackendImpl::WithSession(this, [&](){ mBackend.DeleteClient(); });
    }
    catch (const Backend::BackendException& ex) 
    { 
        MDBG_ERROR("... " << ex.what());
    }
}

/*****************************************************/
Session Session::FromExisting(BackendImpl& backend, const SessionStore& session)
{
    return Session::FromExisting(backend, session.GetSessionID(), session.GetSessionKey());
}

/*****************************************************/
Session Session::FromExisting(Backend::BackendImpl& backend, const std::string& sessionID, const std::string& sessionKey)
{ 
    nlohmann::json account;
    // doing an action now also ensures the session is valid!
    BackendImpl::WithSession(sessionID, sessionKey, [&](){ account = backend.GetAccount(); });

    try
    {
        return Session(backend, account, sessionID, sessionKey, false);
    }
    catch (const nlohmann::json::exception& ex) {
       throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
Session Session::Create(BackendImpl& backend, const std::string& username, const SecureBuffer& password, const std::string& twofactor)
{
    SDBG_INFO("(username:" << username << ")");

    const Account::PasswordKeys pwkeys { Account::GetPasskeys(backend, username, password) };
    const std::string passkey64 { StringUtil::base64_encode(pwkeys.authsubkey) };
    const nlohmann::json resp(backend.CreateSession(username, passkey64, twofactor));

    try
    {
        std::string sessionID;
        std::string sessionKey;
        resp.at("client").at("session").at("id").get_to(sessionID);
        resp.at("client").at("session").at("authkey").get_to(sessionKey);

        SDBG_INFO("... sessionID:" << sessionID);

        Session retval(backend, resp.at("account"), sessionID, sessionKey, true);
        retval.mE2ee_pwsubkey = pwkeys.e2eesubkey;
        return retval;
    }
    catch (const nlohmann::json::exception& ex) {
       throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
Session Session::CreateInteractive(BackendImpl& backend, const std::string& username, SecureBuffer& password)
{
    SDBG_INFO("(username:" << username << ")");

    if (password.empty())
    {
        std::cout << "Enter password: ";
        password = PlatformUtil::SecureReadConsole();
    }

    std::string twofactor;
    while (true) // prompt for things one at a time
    {
        try
        {
            return Session::Create(backend, username, password, twofactor);
        }
        catch (const BackendImpl::TwoFactorRequiredException&)
        {
            std::cout << "Enter two factor: ";
            const SecureBuffer tfBuf { PlatformUtil::SecureReadConsole() };
            twofactor = tfBuf.Insecure_ToStr(); // doesn't matter
        }
    }
}

} // namespace Andromeda::Account