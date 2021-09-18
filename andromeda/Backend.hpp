#ifndef LIBA2_BACKEND_H_
#define LIBA2_BACKEND_H_

#include <map>
#include <string>

#include <nlohmann/json_fwd.hpp>

#include "Config.hpp"
#include "Utilities.hpp"

/** Manages communication with the backend API */
class Backend
{
public:

    typedef std::map<std::string, std::string> Params;

    /** Implements the actual external call to the API */
    class Runner
    {
    public:
        /**
         * Runs an API call and returns the result
         * @param app the app name to run
         * @param action the app action to run
         * @param params map of input parameters
         * @return result string from API
         */
        virtual std::string RunAction(
            const std::string& app, const std::string& action, 
            const Backend::Params& params = Backend::Params()) = 0;
    };

    /** Base Exception for all backend issues */
    class Exception : public Utilities::Exception { public:
        Exception(int code) : 
            Utilities::Exception("Backend Error: Code "+std::to_string(code)){};
        Exception(const std::string& message) :
            Utilities::Exception("Backend Error: "+message) {}; };

    /** Exception indicating there was a JSON format error */
    class JSONErrorException : public Exception { public:
        JSONErrorException(const std::string& error) : 
            Exception("JSON Error: "+error) {}; };
    
    /** Base exception for Andromeda-returned errors */
    class APIException : public Exception { public:
        APIException(int code, const std::string& message) : 
            Exception("API code:"+std::to_string(code)+" message:"+message) {}; };

    /** Base exception for Andromeda-returned 403 errors */
    class APIDeniedException : public Exception { public:
        APIDeniedException() : Exception("Access Denied") {};
        APIDeniedException(const std::string& message) : Exception(message) {}; };

    /** Base exception for Andromeda-returned 404 errors */
    class APINotFoundException : public Exception { public:
        APINotFoundException() : Exception("Not Found") {};
        APINotFoundException(const std::string& message) : Exception(message) {}; };

    /** Andromeda exception indicating authentication failed */
    class AuthenticationFailedException : public APIDeniedException { public:
        AuthenticationFailedException() : APIDeniedException("Authentication Failed") {}; };

    /** Andromeda exception indicating two factor is needed */
    class TwoFactorRequiredException : public APIDeniedException { public:
        TwoFactorRequiredException() : APIDeniedException("Two Factor Required") {}; };

    /** @param runner the Runner to use */
    Backend(Runner& runner);

    virtual ~Backend();

    /** Initializes the backend by loading config */
    void Initialize();

    /** Creates a new backend session and registers for use */
    void Authenticate(const std::string& username, const std::string& password, const std::string& twofactor = "");

    /** Creates a new backend session, interactively prompting for input as required */
    void AuthInteractive(const std::string& username, std::string password = "", std::string twofactor = "");

    /** Registers a pre-existing session for use */
    void PreAuthenticate(const std::string& sessionID, const std::string& sessionKey);

    /** Closes the existing session */
    void CloseSession();

    /**
     * Loads server config
     * @return loaded config as JSON with "server" and "files" keys
     */
    nlohmann::json GetConfig();

    /**
     * Load folder metadata
     * @param id folder ID (or blank for default)
     */
    nlohmann::json GetFolder(const std::string& id = "");

private:

    std::string RunAction(const std::string& app, const std::string& action);

    /**
     * Runs an action with authentication details 
     * @see Runner::RunAction()
     */
    std::string RunAction(const std::string& app, const std::string& action, Params& params);

    /** Parses and returns standard Andromeda JSON */
    nlohmann::json GetJSON(const std::string& resp);

    /** True if we created the session in use */
    bool createdSession = false;

    std::string sessionID;
    std::string sessionKey;

    Debug debug;
    Runner& runner;
    Config config;
};

#endif