#include <chrono>
#include <sstream>

#include "Options.hpp"

#include "andromeda-fuse/FuseOptions.hpp"

#include "andromeda/ConfigOptions.hpp"
using Andromeda::ConfigOptions;
#include "andromeda/account/SessionOptions.hpp"
using Andromeda::Account::SessionOptions;
#include "andromeda/backend/HTTPOptions.hpp"
using Andromeda::Backend::HTTPOptions;
#include "andromeda/backend/RunnerOptions.hpp"
using Andromeda::Backend::RunnerOptions;
#include "andromeda/filesystem/filedata/CacheOptions.hpp"
using Andromeda::Filesystem::Filedata::CacheOptions;

namespace AndromedaFuse {

/*****************************************************/
std::string Options::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Usage Syntax: " << endl
        << "andromeda-fuse " << CoreBaseHelpText() << " -m|--mountpath path (-a|--apiurl url | -p|--apipath [path])" << endl << endl

        << "Remote Object:   [--folder [id] | --filesystem [id]]" << endl
        << SessionOptions::HelpText() << endl << endl
       
        << HTTPOptions::HelpText() << endl
        << RunnerOptions::HelpText() << endl << endl
        << FuseOptions::HelpText() << endl << endl
        
        << ConfigOptions::HelpText() << endl
        << CacheOptions::HelpText() << endl << endl
           
        << DetailBaseHelpText("fuse") << endl;

    return output.str();
}

/*****************************************************/
Options::Options(ConfigOptions& configOptions_, 
                 HTTPOptions& httpOptions_,
                 RunnerOptions& runnerOptions_,
                 SessionOptions& sessionOptions_,
                 CacheOptions& cacheOptions_,
                 FuseOptions& fuseOptions_) :
    configOptions(configOptions_), 
    httpOptions(httpOptions_), 
    runnerOptions(runnerOptions_),
    sessionOptions(sessionOptions_),
    cacheOptions(cacheOptions_),
    fuseOptions(fuseOptions_) { }

/*****************************************************/
bool Options::AddFlag(const std::string& flag)
{
    if (flag == "p" || flag == "apipath")
        apiType = ApiType::API_PATH;

    else if (flag == "storage")
        mountRootType = RootType::STORAGE;
    else if (flag == "folder")
        mountRootType = RootType::FOLDER;

    else if (flag == "d" || flag == "debug")
        foreground = true;
    
    else if (BaseOptions::AddFlag(flag)) { }
    else if (configOptions.AddFlag(flag)) { }
    else if (httpOptions.AddFlag(flag)) { }
    else if (runnerOptions.AddFlag(flag)) { }
    else if (sessionOptions.AddFlag(flag)) { }
    else if (cacheOptions.AddFlag(flag)) { }
    else if (fuseOptions.AddFlag(flag)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
bool Options::AddOption(const std::string& option, const std::string& value)
{
    /** Backend endpoint selection */
    if (option == "a" || option == "apiurl")
    {
        apiPath = value;
        apiType = ApiType::API_URL;

        // Certain details can be parsed from the URL
        ParseUrl(apiPath);
    }
    else if (option == "p" || option == "apipath")
    {
        apiPath = value;
        apiType = ApiType::API_PATH;
    }

    /** Backend mount object selection */
    else if (option == "ri" || option == "storage")
    {
        mountItemID = value;
        mountRootType = RootType::STORAGE;
    }
    else if (option == "rf" || option == "folder")
    {
        mountItemID = value;
        mountRootType = RootType::FOLDER;
    }
    else if (option == "m" || option == "mountpath")
        mountPath = value;

    else if (option == "d" || option == "debug")
    {
        foreground = true;
        BaseOptions::AddOption(option, value);
    }

    else if (BaseOptions::AddOption(option, value)) { }
    else if (configOptions.AddOption(option, value)) { }
    else if (httpOptions.AddOption(option, value)) { }
    else if (runnerOptions.AddOption(option, value)) { }
    else if (sessionOptions.AddOption(option, value)) { }
    else if (cacheOptions.AddOption(option, value)) { }
    else if (fuseOptions.AddOption(option, value)) { }

    else return false; // not used
    
    return true;
}

/*****************************************************/
void Options::TryAddUrlOption(const std::string& option, const std::string& value)
{
    if (option == "folder")
    {
        mountItemID = value;
        mountRootType = RootType::FOLDER;
    }
}

/*****************************************************/
void Options::Validate() const
{
    if (apiType == ApiType::API_INVALID)
        throw MissingOptionException("apiurl/apipath");

    if (mountPath.empty())
        throw MissingOptionException("mountpath");

    sessionOptions.Validate();
}

} // namespace AndromedaFuse
