#ifndef LIBA2_SESSION_H_
#define LIBA2_SESSION_H_

#include <string>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"

namespace Andromeda::Backend { class BackendImpl; }

namespace Andromeda::Account {
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
    static Session Create(Backend::BackendImpl& backend, const std::string& username, const std::string& password, const std::string& twofactor = "");

    /**
     * Creates a new session on the backend (interactive, prompts for input)
     * @param username username to use for authentication
     * @param password raw password for authentication
     * @throws BackendImpl::AuthenticationFailedException for invalid username/password
     * @throws BackendImpl::Exception for other backend issues
     */
    static Session CreateInteractive(Backend::BackendImpl& backend, const std::string& username, std::string password);

    /** Returns a reference to the backend for this session */
    Backend::BackendImpl& GetBackend() const { return mBackend; }
    /** Returns the account username in use */
    const std::string& GetUsername() const { return mUsername; }
    /** Returns the session ID in use */
    const std::string& GetSessionID() const { return mSessionID; }
    /** Returns the session key in use */
    const std::string& GetSessionKey() const { return mSessionKey; }

    /** Sets whether the session is temporary - if true, it is deleted from the server when destructed! */
    void SetTemporary(bool temp){ mTemporary = temp; }

private:

    Session(Backend::BackendImpl& backend, const std::string& username, const std::string& sessionID, const std::string& sessionKey, bool temporary); // constructor

    /**
     * Returns the passkey (client-side hashing) to use with the backend for authentication
     * @param backend backend reference (will call GetPasswordSalt)
     * @param username username of the account
     * @param password raw password of the account
     * @return std::string passkey derived from the password
     */
    static std::string GetPasskey(Backend::BackendImpl& backend, const std::string& username, const std::string& password);

    mutable Debug mDebug;
    Backend::BackendImpl& mBackend;

    std::string mUsername;
    std::string mSessionID;
    std::string mSessionKey;

    /** True if the session should be deleted when destructed */
    bool mTemporary;
};

} // namespace Andromeda::Account

#endif // LIBA2_SESSION_H_
