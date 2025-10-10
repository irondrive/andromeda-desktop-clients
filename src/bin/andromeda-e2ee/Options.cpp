#include <chrono>
#include <sstream>

#include "Options.hpp"

#include "andromeda/account/SessionOptions.hpp"
using Andromeda::Account::SessionOptions;
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
        << "andromeda-e2ee (-a|--apiurl url | -p|--apipath [path]) [global flags] action [action flags]" << endl
        << "Misc Flags: " << CoreBaseHelpText() << endl << endl

        << SessionOptions::HelpText() << endl << endl

        << HTTPOptions::HelpText() << endl
        << RunnerOptions::HelpText() << endl << endl
           
        << DetailBaseHelpText("e2ee") << endl;

    return output.str();
}

/*****************************************************/
Options::Options(HTTPOptions& httpOptions_,
                 RunnerOptions& runnerOptions_,
                 SessionOptions& sessionOptions_) :
    httpOptions(httpOptions_), 
    runnerOptions(runnerOptions_),
    sessionOptions(sessionOptions_) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (flag == "p" || flag == "apipath")
        apiType = ApiType::API_PATH;

    else if (BaseOptions::AddFlag(flag)) { }
    else if (httpOptions.AddFlag(flag)) { }
    else if (runnerOptions.AddFlag(flag)) { }
    else if (sessionOptions.AddFlag(flag)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    /** Backend endpoint selection */
    if (option == "a" || option == "apiurl")
    {
        apiUrl = value;
        apiType = ApiType::API_URL;

        // Certain details can be parsed from the URL
        ParseUrl(apiUrl);
    }
    else if (option == "p" || option == "apipath")
    {
        apiPath = value;
        apiType = ApiType::API_PATH;
    }

    else if (BaseOptions::AddOption(option, value)) { }
    else if (httpOptions.AddOption(option, value)) { }
    else if (runnerOptions.AddOption(option, value)) { }
    else if (sessionOptions.AddOption(option, value)) { }

    else return false; // not used
    
    return true;
}

} // namespace AndromedaE2ee
