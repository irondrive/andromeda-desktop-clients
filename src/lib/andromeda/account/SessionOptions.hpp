#ifndef LIBA2_SESSIONOPTIONS_H_
#define LIBA2_SESSIONOPTIONS_H_

#include <memory>
#include <string>
#include "andromeda/BaseOptions.hpp"
#include "andromeda/common.hpp"
#include "Session.hpp"

namespace Andromeda::Backend { class BackendImpl; }

namespace Andromeda::Account {

/** Options for backend authentication */
struct SessionOptions : public BaseOptions
{
    SessionOptions() = default;
    
    ~SessionOptions() override;
    // don't proliferate the password...
    DELETE_MOVE(SessionOptions);
    DELETE_COPY(SessionOptions);

    /** Retrieve the standard help text string */
    static std::string HelpText();

    bool AddFlag(const std::string& flag) override;
    bool AddOption(const std::string& option, const std::string& value) override;
    void Validate() const override;

    /**
     * Returns a session object based on given session input
     * @param backend backend to call to create a session
     * @param interactive if true, interactively prompt for password
     * @return a new session object if available, else nullptr
     */
    std::unique_ptr<Session> GetSession(Backend::BackendImpl& backend, bool interactive) const;

    /**
     * Returns a password, always, prompting interactively if needed
     * @param quiet if true, throw rather than prompting interactively
     */
    [[nodiscard]] SecureBuffer RequirePassword(bool quiet) const;

    /** Username to use to create a session, or use auth_sudouser */
    std::string username;
    /** Password to use to create a session */
    std::string password;
    /** E2EE recovery key used to unlock crypto */
    std::string e2ee_recoveryb64;
    /** True to force using a session even with CLI */
    bool forceSession { false };

    /** Pre-created session ID to use */
    std::string sessionid;
    /** Pre-created session key to use */
    std::string sessionkey;
};

} // namespace Andromeda::Account

#endif // LIBA2_SESSIONOPTIONS_H_
