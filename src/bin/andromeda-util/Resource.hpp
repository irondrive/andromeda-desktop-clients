#ifndef A2UTIL_RESOURCE_H_
#define A2UTIL_RESOURCE_H_

#include <memory>
#include "andromeda/common.hpp"

namespace Andromeda
{
    namespace Account { class Account; class Session; }
    namespace Backend { class BaseRunner; class RunnerPool; class BackendImpl; }
}

namespace AndromedaUtil {

struct Options;

/** Class to enable fetching the backend and session as needed for some actions */
class Resource
{
public:
    explicit Resource(Options& options_);
    virtual ~Resource();
    DELETE_MOVE(Resource);
    DELETE_COPY(Resource);

    Options& GetOptions() { return options; }
    /** Initiates (if not already) and returns a BackendImpl from the given options */
    Andromeda::Backend::BackendImpl& GetBackend();
    /** Initiates (if not already) and returns a Session from the given options (if available) */
    Andromeda::Account::Session* TryGetSession();
    /** 
     * Initiates (if not already) and returns a Session from the given options
     * This forces that a session is provided and does not allow auth_sudouser
     */
    Andromeda::Account::Session& GetSession();
    /** 
     * Initiates (if not already) and returns the Account from the given options
     * If using auth_sudouser, this doesn't force a session.  Otherwise it will use GetSession() and use the account from there.
     * NOTE that if using auth_sudouser, then you call GetSession() after this, it will have its own separate account object
     */
    Andromeda::Account::Account& GetAccount();

private:
    Options& options;
    std::unique_ptr<Andromeda::Backend::BaseRunner> runner;
    std::unique_ptr<Andromeda::Backend::RunnerPool> runnerPool;
    std::unique_ptr<Andromeda::Backend::BackendImpl> backend;
    std::unique_ptr<Andromeda::Account::Session> session;
    std::unique_ptr<Andromeda::Account::Account> account;
};

} // namespace AndromedaUtil

#endif // A2UTIL_RESOURCE_H_