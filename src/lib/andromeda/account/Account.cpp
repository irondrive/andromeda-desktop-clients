#include "Account.hpp"

#include "andromeda/Crypto.hpp"
#include "andromeda/SecureBuffer.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda::Account {

namespace { // anonymous
Debug sDebug("Session",nullptr); // NOLINT(cert-err58-cpp)
} // anonymous namespace

/*****************************************************/
Account::PasswordKeys Account::GetPasskeys(BackendImpl& backend, const std::string& username, const SecureBuffer& password)
{
    SDBG_INFO("(username:" << username << ")");

    const std::string password_salt { backend.GetPasswordSalt(username) };
    if (password_salt.size() != Crypto::SaltLength())
        throw BackendImpl::JSONErrorException("incorrect salt length "+std::to_string(password_salt.size()));
    SDBG_INFO("... password_salt:"); sDebug.Info(sDebug.DumpBytes(password_salt.data(), password_salt.size()));

    const SecureBuffer password_superkey { Crypto::DeriveKey(password, password_salt, Crypto::SuperKeyLength()) };
    SDBG_INFO("... password_superkey:"); sDebug.Info(sDebug.DumpBytes(password_superkey.data(), password_superkey.size()));

    const SecureBuffer authkeyb { Crypto::DeriveSubkey(password_superkey, 1, "a2pwauth") };
    const std::string authkey { authkeyb.Insecure_ToStr() }; // extract from SecureBuffer

    const PasswordKeys retval(authkey, Crypto::DeriveSubkey(password_superkey, 0, "a2pwe2ee"));
    SDBG_INFO("... password_e2eekey:"); sDebug.Info(sDebug.DumpBytes(retval.e2eekey.data(), retval.e2eekey.size()));
    SDBG_INFO("... password_authkey:"); sDebug.Info(sDebug.DumpBytes(retval.authkey.data(), retval.authkey.size()));

    return retval;
}

/*****************************************************/
void Account::InitE2ee(BackendImpl& backend, const std::string& username, const SecureBuffer& password)
{
    const SecureBuffer master { Crypto::GenerateSecretKey() }; // master account e2ee key
    SDBG_INFO("... master:"); sDebug.Info(sDebug.DumpBytes(master.data(), master.size()));

    const Crypto::KeyPair masterpair { Crypto::GeneratePublicKeyPair() };
    SDBG_INFO("... masterpub:"); sDebug.Info(sDebug.DumpBytes(masterpair.pubkey.data(), masterpair.pubkey.size()));
    SDBG_INFO("... masterpriv:"); sDebug.Info(sDebug.DumpBytes(masterpair.privkey.data(), masterpair.privkey.size()));

    // TODO RAY !! actually use all 0's? or account ID or something for safety? or something that specifies the "purpose" just in case? could derive a subkey from all 0's, that's fast
    // the passkey.e2eekey is used ONLY once, to wrap the master key, use a 0 nonce
    const std::string privateenc_nonce(Crypto::SecretNonceLength(),'\0');
    SDBG_INFO("... privateenc_nonce:"); sDebug.Info(sDebug.DumpBytes(privateenc_nonce.data(), privateenc_nonce.size()));

    const std::string privateenc { Crypto::EncryptSecret(masterpair.privkey, privateenc_nonce, master) };
    SDBG_INFO("... privateenc:"); sDebug.Info(sDebug.DumpBytes(privateenc.data(), privateenc.size()));



    // should be a separate function to store via the password (maybe not used if using sessions/recovery keys)
    // TODO RAY !! get this from the Session if available, if not, prompt for password? if interactive
    const Account::PasswordKeys pwkeys { GetPasskeys(backend, username, password) };

    // TODO RAY !! actually use all 0's? or account ID or something for safety? or something that specifies the "purpose" just in case? could derive a subkey from all 0's, that's fast, or just some hardcoded string?
    // the passkey.e2eekey is used ONLY once, to wrap the master key, use a 0 nonce
    // need to add a generic string padding function StringUtil
    const std::string masterenc_nonce(Crypto::SecretNonceLength(),'\0');
    SDBG_INFO("... masterenc_nonce:"); sDebug.Info(sDebug.DumpBytes(masterenc_nonce.data(), masterenc_nonce.size()));

    const std::string masterenc { Crypto::EncryptSecret(master, masterenc_nonce, pwkeys.e2eekey) };
    SDBG_INFO("... masterenc:"); sDebug.Info(sDebug.DumpBytes(masterenc.data(), masterenc.size()));
}

} // namespace Andromeda::Account