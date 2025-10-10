
#include "nlohmann/json.hpp"

#include "Account.hpp"
#include "Session.hpp"

#include "andromeda/Crypto.hpp"
#include "andromeda/PlatformUtil.hpp"
#include "andromeda/SecureBuffer.hpp"
#include "andromeda/StringUtil.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda::Account {

namespace { // anonymous
Debug sDebug("Session",nullptr); // NOLINT(cert-err58-cpp)
} // anonymous namespace

#define DBGINFO_KEY(name) { SDBG_INFO("... " << #name << ":"); \
    sDebug.Info(sDebug.DumpBytes((name).data(), (name).size())); }

/*****************************************************/
std::string Account::ParseServerKey(const nlohmann::json& data, const std::string& name)
{
    if (data.contains(name) && data.at(name) != nullptr)
    {
        std::string keyb64; data.at(name).get_to(keyb64);
        const std::optional<std::string> key { StringUtil::base64_decode(keyb64) };
        if (!key) { MDBG_ERROR(mAccountID << " " << name << " invalid base64"); } // carry on...
        else return *key;
    }
    return "";
}

/*****************************************************/
Account::Account(BackendImpl& backend, const nlohmann::json& data, const Session* session):
    mDebug(__func__, this), mBackend(backend), mSession(session)
{
    MDBG_INFO("()");

    MDBG_INFO(data.dump(4));

    try
    {
        data.at("id").get_to(mAccountID);
        data.at("username").get_to(mUsername);
        MDBG_INFO("... id:" << mAccountID << " username:" << mUsername);

        mE2ee_rkmaster = ParseServerKey(data, "e2ee_rkmaster");
        mE2ee_pwmaster = ParseServerKey(data, "e2ee_pwmaster");
        mE2ee_privateenc = ParseServerKey(data, "e2ee_private");
    }
    catch (const nlohmann::json::exception& ex) {
        throw BackendImpl::JSONErrorException(ex.what()); }
}

/*****************************************************/
Account::PasswordKeys Account::GetPasskeys(BackendImpl& backend, const std::string& username, const SecureBuffer& password)
{
    SDBG_INFO("(username:" << username << ")");

    const std::string password_salt { backend.GetPasswordSalt(username) }; DBGINFO_KEY(password_salt);
    if (password_salt.size() != Crypto::SaltLength())
        throw BackendImpl::JSONErrorException("incorrect salt length "+std::to_string(password_salt.size()));

    const SecureBuffer password_superkey { Crypto::DeriveKey(password, password_salt, Crypto::SuperKeyLength()) }; DBGINFO_KEY(password_superkey);

    const PasswordKeys retval(
        Crypto::DeriveSubkey(password_superkey, 1, "a2pwauth").Insecure_ToStr(),
        Crypto::DeriveSubkey(password_superkey, 0, "a2pwe2ee"));
    DBGINFO_KEY(retval.e2eesubkey); DBGINFO_KEY(retval.authsubkey);

    return retval;
}

/*****************************************************/
std::string Account::GetPrivateKeyNonce()
{
    std::string retval(sizeof(KeyUsage),KeyUsage::PRIVATE_KEY);
    Crypto::ResizeNonce(retval, Crypto::SecretNonceLength());
    return retval;
}

/*****************************************************/
std::string Account::EncryptSecret(const SecureBuffer& msg, const std::string& nonce, const std::string& extra)
{
    if (mE2ee_master.empty()) throw KeysNotAvailableException();
    return Crypto::EncryptSecret(msg, nonce, mE2ee_master, extra);
}

/*****************************************************/
SecureBuffer Account::DecryptSecret(const std::string& msg, const std::string& nonce, const std::string& extra)
{
    if (mE2ee_master.empty()) throw KeysNotAvailableException();
    return Crypto::DecryptSecret(msg, nonce, mE2ee_master, extra);
}

/*****************************************************/
SecureBuffer Account::EncodeRecoveryKey(const SecureBuffer& rawkey)
{ 
    return SecureBuffer::Insecure_FromCstr("e2rk:") + StringUtil::base64_encode(rawkey);
}

/*****************************************************/
SecureBuffer Account::DecodeRecoveryKey(const SecureBuffer& fullkey)
{
    if (fullkey.size() < 5 || fullkey.substr(0,5) != "e2rk:")
        throw InvalidRecoveryKeyException();

    const std::optional<SecureBuffer> rawkey { StringUtil::base64_decode(fullkey.substr(5)) };
    if (!rawkey) throw InvalidRecoveryKeyException();
    else return *rawkey;
}

/*****************************************************/
void Account::RunBackend(const std::function<void()>& func)
{
    if (mSession) BackendImpl::WithSession(mSession, func);
    else BackendImpl::WithSudoUsername(mUsername, func);
}

/*****************************************************/
SecureBuffer Account::InitE2eeKeys(bool force)
{
    // the server will check for this too but we might as well now also
    if (!force && HasE2eeKeys()) throw AlreadyInitializedException();

    // create a master key, and recovery key, wrap master with recovery
    const SecureBuffer master { Crypto::GenerateSecretKey() }; DBGINFO_KEY(master);
    const SecureBuffer recovery { Crypto::GenerateSecretKey() }; DBGINFO_KEY(recovery);

    // recovery key is used only ONCE - can use a 0 nonce
    const std::string rkmaster_nonce(Crypto::SecretNonceLength(),'\0'); DBGINFO_KEY(rkmaster_nonce);
    const std::string rkmaster { Crypto::EncryptSecret(master, rkmaster_nonce, recovery) }; DBGINFO_KEY(rkmaster);

    // create a public/private keypair, wrap private with master
    const Crypto::KeyPair masterpair { Crypto::GeneratePublicKeyPair() };
    DBGINFO_KEY(masterpair.privkey); DBGINFO_KEY(masterpair.pubkey);

    const std::string privateenc_nonce { GetPrivateKeyNonce() }; DBGINFO_KEY(privateenc_nonce);
    const std::string privateenc { Crypto::EncryptSecret(masterpair.privkey, privateenc_nonce, master) }; DBGINFO_KEY(privateenc);

    RunBackend([&](){ mBackend.InitAccountE2ee(rkmaster, privateenc, masterpair.pubkey, force); });

    // not changing state until after the backend call succeeds!
    mE2ee_master = master;
    mE2ee_private = masterpair.privkey;
    mE2ee_privateenc = privateenc;

    return recovery;
}

/*****************************************************/
void Account::StoreE2eePwMaster(const SecureBuffer& pwsubkey)
{
    if (mE2ee_master.empty()) throw KeysNotAvailableException();

    // password e2ee subkey is used only ONCE - can use a 0 nonce
    const std::string pwmaster_nonce(Crypto::SecretNonceLength(),'\0'); DBGINFO_KEY(pwmaster_nonce);
    const std::string masterenc { Crypto::EncryptSecret(mE2ee_master, pwmaster_nonce, pwsubkey) }; DBGINFO_KEY(masterenc);

    RunBackend([&](){ mBackend.SetE2eePwMaster(&masterenc); });
}

/*****************************************************/
void Account::UnstoreE2eePwMaster()
{
    RunBackend([&](){ mBackend.SetE2eePwMaster(nullptr); });
}

/*****************************************************/
void Account::UnlockE2eePrivateKey()
{
    if (mE2ee_master.empty()) throw KeysNotAvailableException();
    const std::string privateenc_nonce { GetPrivateKeyNonce() }; DBGINFO_KEY(privateenc_nonce);
    mE2ee_private = Crypto::DecryptSecret(mE2ee_privateenc, privateenc_nonce, mE2ee_master); DBGINFO_KEY(mE2ee_private);
}

/*****************************************************/
void Account::UnlockE2eeFromRecovery(const SecureBuffer& recovery)
{
    MDBG_INFO("()");

    if (mE2ee_rkmaster.empty()) 
        { MDBG_INFO("e2ee not initialized"); return; }
    else if (!mE2ee_master.empty())
        { MDBG_INFO("already unlocked"); return; }
    
    // recovery key is used only ONCE - can use a 0 nonce
    const std::string rkmaster_nonce(Crypto::SecretNonceLength(),'\0'); DBGINFO_KEY(rkmaster_nonce);
    mE2ee_master = Crypto::DecryptSecret(mE2ee_rkmaster, rkmaster_nonce, recovery); DBGINFO_KEY(mE2ee_master);

    UnlockE2eePrivateKey();
}

/*****************************************************/
void Account::UnlockE2eeFromPwSubkey(const SecureBuffer& pwsubkey)
{
    MDBG_INFO("()");

    if (mE2ee_rkmaster.empty()) 
        { MDBG_INFO("e2ee not initialized"); return; }
    else if (!mE2ee_master.empty())
        { MDBG_INFO("already unlocked"); return; }

    if (mE2ee_pwmaster.empty())
        throw RecoveryKeyRequiredException();

    // password e2ee subkey is used only ONCE - can use a 0 nonce
    const std::string pwmaster_nonce(Crypto::SecretNonceLength(),'\0'); DBGINFO_KEY(pwmaster_nonce);
    mE2ee_master = Crypto::DecryptSecret(mE2ee_pwmaster, pwmaster_nonce, pwsubkey); DBGINFO_KEY(mE2ee_master);

    UnlockE2eePrivateKey();
}

/*****************************************************/
void Account::UnlockE2eeFromPassword(const SecureBuffer& password)
{
    MDBG_INFO("()");

    if (mE2ee_rkmaster.empty()) 
        { MDBG_INFO("e2ee not initialized"); return; }
    else if (!mE2ee_master.empty())
        { MDBG_INFO("already unlocked"); return; }

    if (mE2ee_pwmaster.empty())
        throw RecoveryKeyRequiredException();

    UnlockE2eeFromPwSubkey(GetPasskeys(mBackend,mUsername,password).e2eesubkey);
}

/*****************************************************/
void Account::UnlockE2eeInteractive(SecureBuffer& password, SecureBuffer& recoveryb64, const Session* session)
{
    if (!mE2ee_pwmaster.empty())
    {
        if (session != nullptr && !session->TryGetE2eePwSubkey().empty())
            { UnlockE2eeFromPwSubkey(session->TryGetE2eePwSubkey()); return; }

        if (password.empty())
        {
            std::cout << "Enter password: ";
            password = PlatformUtil::SecureReadConsole();
        }

        if (!password.empty()) // allow empty = force recovery key
            { UnlockE2eeFromPassword(password); return; }
    }

    if (recoveryb64.empty())
    {
        std::cout << "Enter e2ee recovery key: ";
        recoveryb64 = PlatformUtil::SecureReadConsole();
    }

    UnlockE2eeFromRecovery(DecodeRecoveryKey(recoveryb64));
}

} // namespace Andromeda::Account