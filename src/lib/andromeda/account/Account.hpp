#ifndef LIBA2_ACCOUNT_H_
#define LIBA2_ACCOUNT_H_

#include <string>

#include "andromeda/common.hpp"
#include "andromeda/Debug.hpp"
#include "andromeda/SecureBuffer.hpp"

namespace Andromeda::Backend { class BackendImpl; }

namespace Andromeda::Account {

// TODO RAY !! comments
class Account
{
public:
    virtual ~Account() = default;
    DELETE_COPY(Account)
    DELETE_MOVE(Account)

    struct PasswordKeys
    {
        inline PasswordKeys(const std::string& a, const SecureBuffer& b):
            authkey(a),e2eekey(b){};
        std::string authkey;
        SecureBuffer e2eekey;
    };

    /**
     * Returns the passkey (client-side hashing) to use with the backend for authentication
     * @param backend backend reference (will call GetPasswordSalt)
     * @param username username of the account
     * @param password raw password of the account
     * @return std::string passkey derived from the password
     */
    static PasswordKeys GetPasskeys(Backend::BackendImpl& backend, const std::string& username, const std::string& password);

    // TODO RAY !! demo only - need to store off new key in Session, support not using password-based, etc.
    // also get the password_cryptkey from an existing session rather than re-calculating?
    static void InitE2ee(Backend::BackendImpl& backend, const std::string& username, const std::string& password);
};

} // namespace Andromeda::Account

#endif // LIBA2_ACCOUNT_H_