
#include <algorithm>
#include <iostream>

// SilentReadConsole()
#if WIN32
#include <windows.h>
#else // !WIN32
#include <termios.h>
#endif // WIN32

// GetEnvironment()
#if WIN32
#include <stdlib.h>
#else // !WIN32
#include <unistd.h>
extern char** environ;
#endif // WIN32

#include "Utilities.hpp"

namespace Andromeda {

/*****************************************************/
Utilities::StringList Utilities::explode(
    std::string str, const std::string& delim, 
    const size_t skip, const bool reverse, const size_t max)
{
    StringList retval;
    
    if (str.empty()) return retval;
    if (delim.empty()) return { str };

    std::string segment; size_t skipped { 0 };

    if (reverse) std::reverse(str.begin(), str.end());

    while ( retval.size() + 1 < max )
    {
        const size_t segEnd { str.find(delim) };
        if (segEnd == std::string::npos) break;

        segment += str.substr(0, segEnd);

        if (skipped >= skip)
        { 
            retval.push_back(segment); 
            segment.clear(); 
        }
        else { ++skipped; segment += delim; }

        str.erase(0, segEnd + delim.length());
    }

    retval.push_back(segment+str);

    if (reverse)
    {
        for (std::string& el : retval)
            std::reverse(el.begin(), el.end());
        std::reverse(retval.begin(), retval.end());
    }

    return retval;
}

/*****************************************************/
Utilities::StringPair Utilities::split(
    const std::string& str, const std::string& delim, 
    const size_t skip, const bool reverse)
{
    Utilities::StringList list { explode(str, delim, skip, reverse, 2) };

    if (list.size() < 1) list.push_back("");
    if (list.size() < 2) list.push_back("");

    return Utilities::StringPair { list[0], list[1] };
}

/*****************************************************/
bool Utilities::startsWith(const std::string& str, const std::string& start)
{
    if (start.size() > str.size()) return false;

    return std::equal(start.begin(), start.end(), str.begin());
}

/*****************************************************/
bool Utilities::endsWith(const std::string& str, const std::string& end)
{
    if (end.size() > str.size()) return false;

    return std::equal(end.rbegin(), end.rend(), str.rbegin());
}

/*****************************************************/
std::string Utilities::trim(const std::string& str)
{
    const size_t size { str.size() };

    size_t start = 0; while (start < size && std::isspace(str[start])) ++start;
    size_t end = size; while (end > 0 && std::isspace(str[end-1])) --end;

    return str.substr(start, end-start);
}

/*****************************************************/
bool Utilities::stringToBool(const std::string& stri)
{
    const std::string str { trim(stri) };
    return (str != "" && str != "0" && str != "false" && str != "off" && str != "no");
}

/*****************************************************/
void Utilities::SilentReadConsole(std::string& retval)
{
    #if WIN32
        HANDLE hStdin { GetStdHandle(STD_INPUT_HANDLE) }; 
        DWORD mode { 0 }; GetConsoleMode(hStdin, &mode);
        SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
    #else // !WIN32
        struct termios oflags, nflags;
        tcgetattr(fileno(stdin), &oflags);

        nflags = oflags;
        nflags.c_lflag &= ~static_cast<decltype(nflags.c_lflag)>(ECHO); // -Wsign-conversion
        tcsetattr(fileno(stdin), TCSANOW, &nflags);
    #endif // WIN32

    std::getline(std::cin, retval);
    
    #if WIN32
        SetConsoleMode(hStdin, mode);
    #else // !WIN32
        tcsetattr(fileno(stdin), TCSANOW, &oflags);
    #endif // WIN32

    std::cout << std::endl;
}

/*****************************************************/
Utilities::StringMap Utilities::GetEnvironment()
{
    char** env { nullptr };
#if WIN32
    env = *__p__environ();
#else // !WIN32
    env = environ;
#endif // WIN32

    StringMap retval;

    while (env != nullptr && *env != nullptr)
        retval.emplace(split(*env++, "="));

    return retval;
}

/*****************************************************/
std::string Utilities::GetHomeDirectory()
{
    #if WIN32
        #pragma warning(push)
        #pragma warning(disable:4996) // getenv is safe in C++11
    #endif // WIN32

    for (const char* env : { "HOME", "HOMEDIR", "HOMEPATH" })
    {
        const char* path { std::getenv(env) };
        if (path != nullptr) return path;
    }

    #if WIN32
        #pragma warning(pop)
    #endif // WIN32

    return ""; // not found
}

} // namespace Andromeda
