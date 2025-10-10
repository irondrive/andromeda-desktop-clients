#ifndef A2UTIL_ACTIONS_H_
#define A2UTIL_ACTIONS_H_

#include "andromeda/Debug.hpp"

namespace AndromedaUtil {

class Resource;

/** Class for executing andromeda-util subcommands */
class Actions
{
public:
    /** Retrieve the standard help text string */
    static std::string HelpText();

    inline explicit Actions(Resource& resource): 
        mDebug(__func__, this), mResource(resource){ }

    /**
     * Runs the action given as the first arg, with the remaining args
     * @throws BaseOptions::Exception if the action is invalid
     */
    void RunAction(int argc, const char* const* argv);

    /** Generates a random alphanum or binary string, maybe base64 */
    void Random(int argc, const char* const* argv);
    /** Calculates the passkey base64 from a username and password */    
    void GetPasskey(int argc, const char* const* argv);

    /** Initializes e2ee keys for an account */
    void InitAccountKeys(int argc, const char* const* argv);
    /** Attempts to load and unlock e2ee on an account */
    void TestAccountKeys(int argc, const char* const* argv);
    /** Stores or deletes the password-wrapped master key */
    void StorePwMasterKey(int argc, const char* const* argv);
    /** Generates and stores a new recovery key (overwriting old) */
    void GenRkMasterKey(int argc, const char* const* argv);
    
    /** Creates a session and returns it + e2ee keys */
    void CreateSession(int argc, const char* const* argv);
    /** Changes an account's password, possibly re-storing the PwMasterKey */
    void ChangePassword(int argc, const char* const* argv);

    /** Initializes e2ee folder keys throughout a filesystem */
    void InitFilesystemKeys(int argc, const char* const* argv);
    
private:
    Andromeda::Debug mDebug;
    Resource& mResource;
}; 

} // namespace AndromedaUtil

#endif // A2UTIL_ACTIONS_H_
