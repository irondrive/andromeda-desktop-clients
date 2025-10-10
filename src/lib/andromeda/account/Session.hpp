#ifndef LIBA2_SESSION_H_
#define LIBA2_SESSION_H_

#include <memory>
#include <string>
#include "nlohmann/json_fwd.hpp"

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/SecureBuffer.hpp"

namespace Andromeda::Backend { class BackendImpl; }

namespace Andromeda::Account {

class Account;
class SessionStore;

/** Represents an authenticated session that can be used with the backend */
class Session
{
public:

    virtual ~Session();
    DELETE_COPY(Session);
    
    Session(Session&& old) noexcept; // move constructor
    Session& operator=(Session&& old) noexcept = delete;

    /** 
     * Returns a session from the given SessionStore (won't delete on destruct) 
     * Runs an action with the backend to check the session is valid
     * @throws BackendImpl::InvalidSession if the session is not valid
     * @throws BackendImpl::Exception for other backend issues
     */
    static Session FromExisting(Backend::BackendImpl& backend, const SessionStore& session);

    /** 
     * Returns a Session from the given ID and key (won't delete on destruct) 
     * Runs an action with the backend to check the session is valid
     * @throws BackendImpl::InvalidSession if the session is not valid
     * @throws BackendImpl::Exception for other backend issues
     */
    static Session FromExisting(Backend::BackendImpl& backend, const std::string& sessionID, const std::string& sessionKey);

    /**
     * Creates a new session on the backend (non-interactive)
     * @param username username to use for authentication
     * @param password raw password for authentication
     * @param twofactor two factor code for authentication (optional)
     * @throws BackendImpl::AuthenticationFailedException for invalid username/password
     * @throws BackendImpl::TwoFactorRequiredException if two factor is required and not given
     * @throws BackendImpl::Exception for other backend issues
     */
    static Session Create(Backend::BackendImpl& backend, const std::string& username, const SecureBuffer& password, const std::string& twofactor = "");

    /**
     * Creates a new session on the backend (interactive, prompts for input)
     * @param username username to use for authentication
     * @param password raw password for authentication
     * @throws BackendImpl::AuthenticationFailedException for invalid username/password
     * @throws BackendImpl::Exception for other backend issues
     */
    static Session CreateInteractive(Backend::BackendImpl& backend, const std::string& username, SecureBuffer& password);

    /** Returns a reference to the backend for this session */
    inline Backend::BackendImpl& GetBackend() const { return mBackend; }

    /** Returns the account associated with this session */
    inline const Account& GetAccount() const { return *mAccount; }
    inline Account& GetAccount() { return *mAccount; }

    /** Returns the session ID in use */
    inline const std::string& GetSessionID() const { return mSessionID; }
    /** Returns the session key in use */
    inline const std::string& GetSessionKey() const { return mSessionKey; }

    /** Returns the e2ee password subkey, if known (e.g. session was just created), else an empty buffer */
    inline const SecureBuffer& TryGetE2eePwSubkey() const { return mE2ee_pwsubkey; }

    /** Sets whether the session is temporary - if true, it is deleted from the server when destructed! */
    inline void SetTemporary(bool temp){ mTemporary = temp; }

private:

    /** @throws BackendImpl::JSONErrorException if the account JSON data is bad */
    Session(Backend::BackendImpl& backend, const nlohmann::json& account, const std::string& sessionID, const std::string& sessionKey, bool temporary);

    mutable Debug mDebug;
    Backend::BackendImpl& mBackend;

    std::unique_ptr<Account> mAccount; // NEVER NULL
    /** cached e2ee subkey if we used the password */
    SecureBuffer mE2ee_pwsubkey;

    std::string mSessionID;
    std::string mSessionKey;

    /** True if the session should be deleted when destructed */
    bool mTemporary;
};

} // namespace Andromeda::Account

#endif // LIBA2_SESSION_H_
