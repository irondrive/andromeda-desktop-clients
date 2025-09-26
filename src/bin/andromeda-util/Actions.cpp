
#include <iostream>

#include "Actions.hpp"
#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/Crypto.hpp"
using Andromeda::Crypto;
#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;

namespace AndromedaUtil {

/*****************************************************/
void Actions::RunAction(int argc, const char* const* argv)
{
    if (argc < 1) throw BaseOptions::BadUsageException("Missing action");

    const std::string action { argv[0] };
    --argc; ++argv;

    if (action == "random")         Random(argc,argv);
    else if (action == "inite2ee")  InitE2ee(argc,argv); 
    else if (action == "changepw")  ChangePassword(argc,argv);

    else throw BaseOptions::BadUsageException("Invalid action");
}

/** Options parser for the Random action */
class RandomOptions : public BaseOptions
{
public:
    // TODO RAY !! help text

    bool AddFlag(const std::string& flag) override
    {
        if (flag == "base64")
            mBase64 = true;
        else if (flag == "alphanum")
            mAlphanum = true;
        else return false;

        return true;
    }

    bool AddOption(const std::string& option, const std::string& value) override
    {
        if (option == "size" || option == "length")
        {
            // TODO RAY !! make this (stoul) a BaseOptions helper function
            try { mLength = static_cast<decltype(mLength)>(stoul(value)); }
            catch (const std::logic_error& e) { 
                throw BaseOptions::BadValueException(option); }
        }
        else return false;

        return true;
    }

    [[nodiscard]] bool isBase64() const { return mBase64; }
    [[nodiscard]] bool isAlphanum() const { return mAlphanum; }
    [[nodiscard]] size_t GetLength() const { return mLength; }

private:

    bool mBase64 { false };
    bool mAlphanum { false };
    size_t mLength { 64 };
};

/*****************************************************/
void Actions::Random(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
    RandomOptions options;
    options.ParseArgs(static_cast<size_t>(argc), argv);

    std::string ret;
    
    if (options.isAlphanum())
        ret = StringUtil::Random(options.GetLength());
    else ret = Crypto::GenerateRandom(options.GetLength());
    
    if (options.isBase64()) 
        ret = StringUtil::base64_encode(ret);

    std::cout << ret; // no endl
}

/*****************************************************/
void Actions::InitE2ee(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
    // TODO RAY !! assert session not null, or backend has sudo username (new backend call to consolidate)
}

/*****************************************************/
void Actions::ChangePassword(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
    // TODO RAY !! assert session not null, or backend has sudo username (new backend call to consolidate)
}

} // namespace AndromedaUtil

