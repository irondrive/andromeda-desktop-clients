
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

/*****************************************************/
Session::Session(BackendImpl& backend, const std::string& username, const std::string& sessionID, const std::string& sessionKey, bool temporary):
    mDebug(__func__, this), mBackend(backend), mUsername(username), mSessionID(sessionID), mSessionKey(sessionKey), mTemporary(temporary)
{
    MDBG_INFO("(username:" << username << " sessionID:" << sessionID << ")");
}

/*****************************************************/
Session::~Session()
{
    MDBG_INFO("(username:" << mUsername << " sessionID:" << mSessionID << ")");
    if (mTemporary)
    {
        try { mBackend.DeleteClient(this); }
        catch (const Backend::BackendException& ex) 
        { 
            MDBG_ERROR("... " << ex.what());
        }
    }
}

/*****************************************************/
Session::Session(Session&& old) noexcept: // move constructor
    Session(old.mBackend, old.mUsername, old.mSessionID, old.mSessionKey, old.mTemporary)
{
    old.mTemporary = false; // don't delete twice
}

/*****************************************************/
Session Session::FromExisting(BackendImpl& backend, const SessionStore& session)
{
    return Session::FromExisting(backend, session.GetSessionID(), session.GetSessionKey());
}

/*****************************************************/
Session Session::FromExisting(Backend::BackendImpl& backend, const std::string& sessionID, const std::string& sessionKey)
{ 
    Session session(backend, "", sessionID, sessionKey, false);
    // doing an action now also ensures the session is valid!
    const nlohmann::json resp(backend.GetAccount(&session));

    try
    {
        resp.at("username").get_to(session.mUsername);
        SDBG_INFO("... username:" << session.mUsername);
    }
    catch (const nlohmann::json::exception& ex) {
       throw BackendImpl::JSONErrorException(ex.what()); }

    return session;
}

/*****************************************************/
Session Session::Create(BackendImpl& backend, const std::string& username, const std::string& password, const std::string& twofactor)
{
    SDBG_INFO("(username:" << username << ")");

    const std::string passkey64 { StringUtil::base64_encode(Account::GetPasskeys(backend, username, password).authkey) };
    const nlohmann::json resp(backend.CreateSession(username, passkey64, twofactor));

    std::string sessionID;
    std::string sessionKey;
    try
    {
        //resp.at("account").at("id").get_to(accountID);
        resp.at("client").at("session").at("id").get_to(sessionID);
        resp.at("client").at("session").at("authkey").get_to(sessionKey);

        SDBG_INFO("... sessionID:" << sessionID);
    }
    catch (const nlohmann::json::exception& ex) {
       throw BackendImpl::JSONErrorException(ex.what()); }

    return Session(backend, username, sessionID, sessionKey, true);
}

/*****************************************************/
Session Session::CreateInteractive(BackendImpl& backend, const std::string& username, std::string password)
{
    SDBG_INFO("(username:" << username << ")");

    if (password.empty())
    {
        std::cout << "Password? ";
        PlatformUtil::SilentReadConsole(password);
    }

    try
    {
        return Session::Create(backend, username, password);
    }
    catch (const BackendImpl::TwoFactorRequiredException&)
    {
        std::string twofactor; std::cout << "Two Factor? ";
        PlatformUtil::SilentReadConsole(twofactor);

        return Session::Create(backend, username, password, twofactor);
    }
}

} // namespace Andromeda::Account