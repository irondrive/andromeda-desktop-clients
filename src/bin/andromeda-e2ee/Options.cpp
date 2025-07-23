#include <chrono>
#include <sstream>

#include "Options.hpp"

#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;

namespace AndromedaE2ee {

/*****************************************************/
std::string Options::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-e2ee " << CoreBaseHelpText() << endl
        << "andromeda-e2ee (-a|--apiurl url | -p|--apipath [path])" << endl << endl

        << "Remote Auth:     [-u|--username str] [--password str] | [--sessionid id] [--sessionkey key] [--force-session]" << endl << endl
       
        << HTTPOptions::HelpText() << endl
        << RunnerOptions::HelpText() << endl << endl
        << ConfigOptions::HelpText() << endl
           
        << DetailBaseHelpText("e2ee") << endl;

    return output.str();
}

/*****************************************************/
Options::Options(ConfigOptions& configOptions, 
                 HTTPOptions& httpOptions,
                 RunnerOptions& runnerOptions) :
    mConfigOptions(configOptions), 
    mHttpOptions(httpOptions), 
    mRunnerOptions(runnerOptions) { }

/*****************************************************/
size_t Options::ParseArgs(size_t argc, const char* const* argv, bool stopmm)
{
    if (argc < 2) throw BadUsageException("what action?");
    mAction = argv[1]; --argc; ++argv; // TODO RAY !! add help text
    // TODO RAY !! where is the validation for this? probably in the runner

    return BaseOptions::ParseArgs(argc, argv, stopmm);
}

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (flag == "p" || flag == "apipath")
        mApiType = ApiType::API_PATH;

    else if (flag == "force-session")
        mForceSession = true;

    else if (BaseOptions::AddFlag(flag)) { }
    else if (mConfigOptions.AddFlag(flag)) { } // TODO RAY !! not needed? maybe need to separate out? honestly this is just "FileSystemConfig" - except what does -q do?
    else if (mHttpOptions.AddFlag(flag)) { }
    else if (mRunnerOptions.AddFlag(flag)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    /** Backend endpoint selection */
    if (option == "a" || option == "apiurl")
    {
        mApiPath = value;
        mApiType = ApiType::API_URL;

        // Certain details can be parsed from the URL
        ParseUrl(mApiPath);
    }
    else if (option == "p" || option == "apipath")
    {
        mApiPath = value;
        mApiType = ApiType::API_PATH;
    }

    /** Backend authentication details */
    else if (option == "u" || option == "username")
        mUsername = value;
    else if (option == "password")
        mPassword = value;
    else if (option == "sessionid")
        mSessionid = value;
    else if (option == "sessionkey")
        mSessionkey = value;

    else if (BaseOptions::AddOption(option, value)) { }
    else if (mConfigOptions.AddOption(option, value)) { }
    else if (mHttpOptions.AddOption(option, value)) { }
    else if (mRunnerOptions.AddOption(option, value)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
void Options::Validate()
{
    if (GetApiType() == ApiType::API_INVALID)
        throw MissingOptionException("apiurl/apipath");
}

} // namespace AndromedaE2ee
