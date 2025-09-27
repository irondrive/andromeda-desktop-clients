#ifndef A2UTIL_ACTIONS_H_
#define A2UTIL_ACTIONS_H_

#include "andromeda/Debug.hpp"

namespace AndromedaUtil {

class Resource;

/** Class for executing andromeda-util subcommands */
class Actions
{
public:
    inline explicit Actions(Resource& resource): mDebug(__func__, this), mResource(resource){ }

    /**
     * Runs the action given as the first arg, with the remaining args
     * @throws BaseOptions::Exception if the action is invalid
     */
    void RunAction(int argc, const char* const* argv);

    /** Generates a random alphanum or binary string, maybe base64 */
    void Random(int argc, const char* const* argv);
    // TODO RAY !! comments
    void GetPasskey(int argc, const char* const* argv);
    void InitAccountE2ee(int argc, const char* const* argv);
    void InitFilesystemE2ee(int argc, const char* const* argv);
    void ChangePassword(int argc, const char* const* argv);

private:
    Andromeda::Debug mDebug;
    Resource& mResource;
}; 

} // namespace AndromedaUtil

#endif // A2UTIL_ACTIONS_H_
