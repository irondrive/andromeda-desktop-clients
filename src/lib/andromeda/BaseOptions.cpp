
#include <cstring>
#include <fstream>
#include <sstream>

#include "BaseOptions.hpp"
#include "Debug.hpp"
#include "PlatformUtil.hpp"
#include "StringUtil.hpp"

namespace Andromeda {

/*****************************************************/
std::string BaseOptions::CoreBaseHelpText()
{
    std::ostringstream output;

    output << "[-h|--help | -V|--version] [-q|--quiet]";

    return output.str();
}

/*****************************************************/
std::string BaseOptions::DetailBaseHelpText(const std::string& name)
{
    std::ostringstream output;
    using std::endl;

    output << "Config File:     [-c|--config-file path]" << endl
           << "Debugging:       [-d|--debug 0-" << static_cast<size_t>(Debug::Level::LAST)-1 << "] [--debug-filter str1,str2+] [--debug-log path]" << endl << endl

           << "Any flag or option can also be listed in andromeda.conf";
    if (!name.empty()) output << " or andromeda-" << name << ".conf";
    output << " with one option=value per line.";

    return output.str();
}

/*****************************************************/
size_t BaseOptions::ParseArgs(size_t argc, const char* const* argv, bool stopmm) // NOLINT(readability-function-cognitive-complexity)
{
    size_t argIdx { 0 }; for (; argIdx < argc; argIdx++)
    {
        std::string key { argv[argIdx] };
        if (key.empty())
            throw BadUsageException(
                "empty key at arg "+std::to_string(argIdx));
        if (key[0] != '-')
        {
            if (stopmm) return argIdx;
            else throw BadUsageException(
                "expected key at arg "+std::to_string(argIdx));
        }

        key.erase(0, 1); // key++
        const bool ext { (key[0] == '-') };
        if (ext) key.erase(0, 1); // --opt

        if (key.empty() || std::isspace(key[0]))
            throw BadUsageException(
                "empty key at arg "+std::to_string(argIdx));

        if (key.find('=') != std::string::npos) // -x=3 or --x=3
        {
            const StringUtil::StringPair pair { StringUtil::split(key, "=") };
            if (!AddOption(pair.first, pair.second))
                throw BadOptionException(pair.first);
        }
        else if (!ext && key.size() > 1) // -x3
        {
            if (!AddOption(key.substr(0,1), key.substr(1)))
                throw BadOptionException(key.substr(0,1));
        }
        else if (argc > argIdx+1 && argv[argIdx+1][0] != '-') // -x 3, --x 3
        {
            if (!AddOption(key, argv[++argIdx]))
            {
                if (!stopmm) throw BadOptionException(key);
                else if (!AddFlag(key)) throw BadFlagException(key);
                else return argIdx;
            }
        }
        else // -x, --x
        {
            if (!AddFlag(key)) throw BadFlagException(key);
        }
    }

    return argIdx;
}

/*****************************************************/
void BaseOptions::ParseFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);

    while (file.good())
    {
        std::string line; std::getline(file,line);

        // Windows-formatted files will have extra \r at the end of lines
        if (!line.empty() && line.back() == '\r') line.pop_back();

        if (line.empty() || line.at(0) == '#' || line.at(0) == ' ') continue;

        if (line.find('=') == std::string::npos)
        {
            if (!AddFlag(line))
                throw BadFlagException(line);
        }
        else
        {
            const StringUtil::StringPair pair { StringUtil::split(line, "=") };
            if (!AddOption(pair.first, pair.second))
                throw BadOptionException(pair.first);
        }
    }
}

/*****************************************************/
void BaseOptions::ParseConfig(const std::string& prefix)
{
    std::list<std::string> paths { 
        "/etc/andromeda", "/usr/local/etc/andromeda" };

    const std::string home { PlatformUtil::GetHomeDirectory() };
    if (!home.empty()) paths.push_back(home+"/.config/andromeda");

    paths.emplace_back("."); for (std::string path : paths)
    {
        path += "/"+prefix+".conf";
        if (std::filesystem::is_regular_file(path))
            ParseFile(path);
    }
}

/*****************************************************/
void BaseOptions::ParseUrl(const std::string& url)
{
    const size_t sep(url.find('?'));

    if (sep != std::string::npos)
    {
        const std::string& substr(url.substr(sep+1));

        for (const std::string& param : StringUtil::explode(substr,"&"))
        {
            if (param.find('=') == std::string::npos)
                TryAddUrlFlag(param);
            else
            {
                const StringUtil::StringPair pair { StringUtil::split(param, "=") };
                TryAddUrlOption(pair.first, pair.second);
            }
        }
    }
}

/*****************************************************/
bool BaseOptions::AddFlag(const std::string& flag)
{
    if (flag == "h" || flag == "help")
        throw ShowHelpException();
    else if (flag == "V" || flag == "version")
        throw ShowVersionException();
    else if (flag == "q" || flag == "quiet")
        mQuiet = true;
    else return false; // not used

    return true;
}

/*****************************************************/
bool BaseOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "c" || option == "config")
    {
        if (std::filesystem::is_regular_file(value))
            ParseFile(value);
        else throw BadValueException(option);
    }
    else if (option == "d" || option == "debug")
    {
        try { Debug::SetLevel(static_cast<Debug::Level>(stoul(value))); }
        catch (const std::logic_error& e) { 
            throw BadValueException(option); }
    }
    else if (option == "debug-filter")
    {
        Debug::SetFilters(value);
    }
    else if (option == "debug-log")
    {
        Debug::AddLogFile(value); // path
    }
    else return false; // not used

    return true;
}

} // namespace Andromeda
