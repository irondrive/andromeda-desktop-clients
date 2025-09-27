#ifndef LIBA2_SESSIONOPTIONS_H_
#define LIBA2_SESSIONOPTIONS_H_

#include <memory>
#include <string>
#include "andromeda/BaseOptions.hpp"
#include "Session.hpp"

namespace Andromeda::Backend { class BackendImpl; }

namespace Andromeda::Account {

/** Options for backend authentication */
struct SessionOptions : public BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText();

    bool AddFlag(const std::string& flag) override;
    bool AddOption(const std::string& option, const std::string& value) override;

    /**
     * Returns a session object based on given session input
     * @param backend backend to call to create a session
     * @param interactive if true, interactively prompt for password
     * @return a new session object if available, else nullptr
     */
    std::unique_ptr<Session> GetSession(Backend::BackendImpl& backend, bool interactive) const;

    /** Username to use to create a session, or use auth_sudouser */
    std::string username;
    /** Password to use to create a session */
    std::string password;
    /** True to force using a session even with CLI */
    bool forceSession { false };

    /** Pre-created session ID to use */
    std::string sessionid;
    /** Pre-created session key to use */
    std::string sessionkey;
};

} // namespace Andromeda::Account

#endif // LIBA2_SESSIONOPTIONS_H_
