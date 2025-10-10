#ifndef LIBA2_ACCOUNT_H_
#define LIBA2_ACCOUNT_H_

#include <string>
#include "nlohmann/json_fwd.hpp"

#include "andromeda/common.hpp"
#include "andromeda/BaseException.hpp"
#include "andromeda/Crypto.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/SecureBuffer.hpp"

namespace Andromeda::Backend { class BackendImpl; }

namespace Andromeda::Account {
class Session;
class SessionStore;

/**
 * Represents a user account on the backend
 * Provides e2ee management and utilities
 */
class Account
{
public:
    /** Base Exception for Account issues */
    class Exception : public Andromeda::BaseException { public:
        /** @param message error message */
        explicit Exception(const std::string& message) :
            Andromeda::BaseException("Account Error: "+message) {}; };

    /** Exception indicating a recovery key is needed for unlocking e2ee */
    class RecoveryKeyRequiredException : public Exception { public:
        explicit RecoveryKeyRequiredException() : 
            Exception("Need a recovery key to unlock e2ee") {}; };

    /** Exception indicating the account already has e2ee keys */
    class AlreadyInitializedException : public Exception { public:
        explicit AlreadyInitializedException() :
            Exception("E2EE keys are already initialized") {}; };

    /** Exception indicating hhat the e2ee keys are not unlocked */
    class KeysNotAvailableException : public Exception { public:
        explicit KeysNotAvailableException() :
            Exception("E2EE keys are not unlocked") {}; };

    /** Exception indicating the recovery key is invalid */
    class RecoveryKeyInvalidException : public Crypto::Exception { public:
        explicit RecoveryKeyInvalidException() : 
            Crypto::Exception("Invalid recovery key format") {}; };

    /** 
     * Construct an account using JSON data from the backend
     * @param session the session that was used to load this account, if any
     * @throws BackendImpl::JSONErrorException if the JSON data is bad
     */
    Account(Backend::BackendImpl& backend, const nlohmann::json& data, const Session* session = nullptr);

    virtual ~Account() = default;
    DELETE_COPY(Account);
    DELETE_MOVE(Account);

    /** Returns the account's username */
    [[nodiscard]] inline const std::string& GetUsername() const { return mUsername; }

    /** Sets the session to use for backend requests (e.g. e2ee init) - should NOT use this manually! */
    inline void SetSession(const Session* session) { mSession = session; }

    /** subkeys derived from a password */
    struct PasswordKeys
    {
        inline PasswordKeys(const std::string& auth, const SecureBuffer& e2ee):
            authsubkey(auth),e2eesubkey(e2ee){};
        /** subkey used for backend authentication */
        std::string authsubkey;
        /** subkey used to wrap the e2ee master key */
        SecureBuffer e2eesubkey;
    };

    /**
     * Returns the passkey (client-side hashing) to use with the backend for authentication
     * @param backend backend reference (will call GetPasswordSalt)
     * @param username username of the account
     * @param password raw password of the account
     * @return PasswordKeys subkeys derived from the password
     * @throws BackendImpl::Exception for other backend issues
     */
    static PasswordKeys GetPasskeys(Backend::BackendImpl& backend, const std::string& username, const SecureBuffer& password);

    /** Returns true if the account has e2ee initialized */
    inline bool HasE2eeKeys() const { return !mE2ee_rkmaster.empty(); }

    /** Returns true if the account can unlock e2ee from a password */
    inline bool HasE2eePwKey() const { return !mE2ee_pwmaster.empty(); }

    /** Returns the master key as a std::string (insecure, use only for console output, etc.) */
    inline std::string Insecure_GetMasterKey() const { return mE2ee_master.Insecure_ToStr(); }

    /** 
     * Stores the master key to the session store
     * @throws KeysNotAvailableException if e2ee is not unlocked
     */
    void StoreMasterKey(SessionStore& sessionStore);

    /**
     * Initializes an e2ee key set and sends them to the backend
     * NOTE - requires a session to be set, or we'll attempt to use auth_sudouser with the backend
     * @param force if true, force overwriting existing keys
     * @return SecureBuffer raw e2ee recovery key
     * @throws AlreadyInitializedException if e2ee is already initialized, and not force
     * @throws BackendImpl::Exception for other backend issues
     */
    SecureBuffer InitE2eeKeys(bool force = false);

    /**
     * Wraps the e2ee master key with the password e2ee subkey, and stores on the backend (allow signin with password only)
     * NOTE - requires a session to be set, or we'll attempt to use auth_sudouser with the backend
     * @param pwsubkey e2ee subkey derived from the password
     * @throws KeysNotAvailableException if e2ee is not unlocked
     * @throws BackendImpl::Exception for other backend issues
     */
    void EnableE2eePwKey(const SecureBuffer& pwsubkey);

    /**
     * Deletes the password subkey-wrapped master key from the backend (disallow signin with password only)
     * NOTE - requires a session to be set, or we'll attempt to use auth_sudouser with the backend
     * @throws BackendImpl::Exception for other backend issues
     */
    void DisableE2eePwKey();

    /**
     * Unlocks e2ee crypto by directly giving the recovery key (will be validated!)
     * @throws Crypto::DecryptFailedException if the key is not valid
     */
    void UnlockE2eeDirectly(const SecureBuffer& masterkey);

    /**
     * Unlocks e2ee crypto from a raw recovery key
     * @throws Crypto::DecryptFailedException if decryption fails
     */
    void UnlockE2eeFromRecovery(const SecureBuffer& recovery);

    /**
     * Unlocks e2ee crypto from a password-derived e2ee subkey
     * @throws RecoveryKeyRequiredException if pwmaster is not available
     * @throws Crypto::DecryptFailedException if decryption fails
     */
    void UnlockE2eeFromPwSubkey(const SecureBuffer& pwsubkey);

    /**
     * Unlocks e2ee crypto from a raw password (fetches salt from the server)
     * @throws RecoveryKeyRequiredException if pwmaster is not available
     * @throws Crypto::DecryptFailedException if decryption fails
     * @throws BackendImpl::Exception for other backend issues
     */
    void UnlockE2eeFromPassword(const SecureBuffer& password);

    /**
     * Unlocks e2ee interactively, prompting for console input as needed
     * @param password password if known, can be empty
     * @param recoveryb64 full recovery key if known, can be empty
     * @param session optional session pointer (might have the password subkey available)
     * @throws Crypto::DecryptFailedException if decryption fails
     * @throws BackendImpl::Exception for other backend issues
     */
    void UnlockE2eeInteractive(SecureBuffer& password, SecureBuffer& recoveryb64, const Session* session = nullptr);

    /** Encodes a raw recovery key into its user-facing format (base64) */
    static SecureBuffer EncodeRecoveryKey(const SecureBuffer& rawkey);

    /**
     * Decodes an encoded recovery key into its raw format
     * @throws Crypto::Exception if the format is invalid
     */
    static SecureBuffer DecodeRecoveryKey(const SecureBuffer& fullkey);

    /** Returns true if e2ee is unlocked and crypto functions can be used */
    inline bool isE2eeUnlocked() const { return !mE2ee_master.empty(); }

    /** 
     * Encrypts a string using the e2ee master key (see Crypto::EncryptSecret)
     * @throws KeysNotAvailableException if e2ee is not unlocked
     */
    std::string EncryptSecret(const SecureBuffer& msg, const std::string& nonce, const std::string& extra = "");
    
    /** 
     * Decrypts a string using the e2ee master key (see Crypto::DecryptSecret) 
     * @throws KeysNotAvailableException if e2ee is not unlocked
     */
    SecureBuffer DecryptSecret(const std::string& msg, const std::string& nonce, const std::string& extra = "");

private:

    /** Runs a function on the backend with this account or its session */
    void RunBackend(const std::function<void()>& func);

    /** Enum of master key purposes, to prefix non-random nonces with to ensure uniqueness */
    enum KeyUsage : char
    {
        PRIVATE_KEY = 1
    };

    /** Returns the nonce to use for encrypting the private key */
    static std::string GetPrivateKeyNonce();

    /** Parses a key (base64 decode) from the json data, returning "" if invalid */
    std::string ParseServerKey(const nlohmann::json& data, const std::string& name);

    /**
     * Unlocks the e2ee private key, using the given master key
     * @throws Crypto::DecryptFailedException if decryption fails
     */
    void UnlockE2eePrivateKey(const SecureBuffer& master);

    mutable Debug mDebug;
    Backend::BackendImpl& mBackend;
    /** Session to use for backend requests, else will use auth_sudouser */
    const Session* mSession { nullptr };

    std::string mAccountID;
    std::string mUsername;

    /** master key encrypted by the password e2ee subkey */
    std::string mE2ee_pwmaster;
    /** master key encrypted by the recovery key */
    std::string mE2ee_rkmaster;
    /** master key used for all things e2ee */
    SecureBuffer mE2ee_master;

    /** private key encrypted by the master key */
    std::string mE2ee_privateenc;
    /** private key used for sharing files */
    SecureBuffer mE2ee_private;
};

} // namespace Andromeda::Account

#endif // LIBA2_ACCOUNT_H_