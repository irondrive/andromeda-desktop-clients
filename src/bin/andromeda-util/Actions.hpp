#ifndef A2UTIL_ACTIONS_H_
#define A2UTIL_ACTIONS_H_

namespace Andromeda::Account { class Session; }
namespace Andromeda::Backend { class BackendImpl; }

#include "andromeda/Debug.hpp"

namespace AndromedaUtil {

/** Class for executing andromeda-util subcommands */
class Actions
{
public:
    inline Actions(Andromeda::Backend::BackendImpl* backend_, Andromeda::Account::Session* session_):
        mDebug(__func__, this), backend(backend_), session(session_){ }

    /**
     * Runs the action given as the first arg, with the remaining args
     * @throws BaseOptions::Exception if the action is invalid
     */
    void RunAction(int argc, const char* const* argv);

    /** Generates a random value, maybe base64 */
    void Random(int argc, const char* const* argv);
    // TODO RAY !! comments
    void InitE2ee(int argc, const char* const* argv);
    void ChangePassword(int argc, const char* const* argv);

private:
    Andromeda::Debug mDebug;

    Andromeda::Backend::BackendImpl* backend; // NOLINT(*unused*) 
    Andromeda::Account::Session* session; // NOLINT(*unused*) // TODO RAY !! nolint
}; 

} // namespace AndromedaUtil

#endif // A2UTIL_ACTIONS_H_
