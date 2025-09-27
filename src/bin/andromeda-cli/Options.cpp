
#include <sstream>

#include "Options.hpp"

#include "andromeda/BaseOptions.hpp"
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;

namespace AndromedaCli {

/*****************************************************/
std::string Options::CoreHelpText()
{
    return CoreBaseHelpText();
}

/*****************************************************/
std::string Options::MainHelpText()
{
    return "-a|--apiurl url";
}

/*****************************************************/
std::string Options::DetailHelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Other Options:   [--stream-out] [--allow-unsafe-url]" << endl
        << HTTPOptions::HelpText() << endl 
        << RunnerOptions::HelpText() << endl << endl
           
        << DetailBaseHelpText("cli");

    return output.str();
}

/*****************************************************/
Options::Options(HTTPOptions& httpOptions_, RunnerOptions& runnerOptions_) :
    httpOptions(httpOptions_), runnerOptions(runnerOptions_) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (BaseOptions::AddFlag(flag)) { }

    else if (flag == "stream-out") streamOut = true;
    else if (flag == "allow-unsafe-url") allowUnsafeUrl = true;

    else if (httpOptions.AddFlag(flag)) { }
    else if (runnerOptions.AddFlag(flag)) { }
    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    if (BaseOptions::AddOption(option, value)) { }

    /** Backend endpoint selection */
    else if (option == "a" || option == "apiurl") apiUrl = value;

    else if (httpOptions.AddOption(option, value)) { }
    else if (runnerOptions.AddOption(option, value)) { }
    else return false; // not used
    
    return true;
}

/*****************************************************/
void Options::Validate() const
{
    if (apiUrl.empty())
        throw MissingOptionException("apiurl");
}

} // namespace AndromedaCli
