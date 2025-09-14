
#include "nlohmann/json.hpp"

#include "Session.hpp"
#include "SessionStore.hpp"
#include "andromeda/Crypto.hpp"
#include "andromeda/SecureBuffer.hpp"
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
    MDBG_INFO("(username: " << username << " sessionID: " << sessionID << ")");
}

/*****************************************************/
Session::~Session()
{
    MDBG_INFO("(username: " << mUsername << " sessionID: " << mSessionID << ")");
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

    const std::string passkey64 { StringUtil::base64_encode(GetPasskey(backend, username, password)) };
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

/*****************************************************/
std::string Session::GetPasskey(BackendImpl& backend, const std::string& username, const std::string& password)
{
    SDBG_INFO("(username:" << username << ")");

    // TODO RAY !! should be using SecureBuffer for password as long as possible (and sessionkey too?) input+output here
    const SecureBuffer passwordBuf { SecureBuffer::Insecure_FromBuf(password.data(), password.size()) };

    const std::string password_salt { backend.GetPasswordSalt(username) };
    if (password_salt.size() != Crypto::SaltLength())
        throw BackendImpl::JSONErrorException("incorrect salt length "+std::to_string(password_salt.size()));
    SDBG_INFO("... password_salt:"); sDebug.Info(sDebug.DumpBytes(password_salt.data(), password_salt.size()));

    const SecureBuffer password_superkey { Crypto::DeriveKey(passwordBuf, password_salt, Crypto::SuperKeyLength()) };
    SDBG_INFO("... password_superkey:"); sDebug.Info(sDebug.DumpBytes(password_superkey.data(), password_superkey.size()));
    
    const SecureBuffer password_cryptkey { Crypto::DeriveSubkey(password_superkey, 0, "a2pwe2ee") };
    const SecureBuffer password_authkey { Crypto::DeriveSubkey(password_superkey, 1, "a2pwauth") };
    SDBG_INFO("... password_cryptkey:"); sDebug.Info(sDebug.DumpBytes(password_cryptkey.data(), password_cryptkey.size()));
    SDBG_INFO("... password_authkey:"); sDebug.Info(sDebug.DumpBytes(password_authkey.data(), password_authkey.size()));

    return std::string(password_authkey.data(), password_authkey.size());
}

} // namespace Andromeda::Account