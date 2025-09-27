#ifndef A2UTIL_RESOURCE_H_
#define A2UTIL_RESOURCE_H_

#include <memory>
#include "andromeda/common.hpp"

namespace Andromeda
{
    namespace Account { class Session; }
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

    // TODO RAY !! comments
    Options& GetOptions() { return options; }
    Andromeda::Backend::BackendImpl& GetBackend();
    Andromeda::Account::Session* TryGetSession();

private:
    Options& options;
    std::unique_ptr<Andromeda::Backend::BaseRunner> runner;
    std::unique_ptr<Andromeda::Backend::RunnerPool> runnerPool;
    std::unique_ptr<Andromeda::Backend::BackendImpl> backend;
    std::unique_ptr<Andromeda::Account::Session> session;
};

} // namespace AndromedaUtil

#endif // A2UTIL_RESOURCE_H_