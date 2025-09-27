
#include <iostream>

#include "Actions.hpp"
#include "Options.hpp"
#include "Resource.hpp"
#include "andromeda/BaseOptions.hpp"
using Andromeda::BaseOptions;
#include "andromeda/Crypto.hpp"
using Andromeda::Crypto;
#include "andromeda/SecureBuffer.hpp"
using Andromeda::SecureBuffer;
#include "andromeda/PlatformUtil.hpp"
using Andromeda::PlatformUtil;
#include "andromeda/StringUtil.hpp"
using Andromeda::StringUtil;
#include "andromeda/account/Account.hpp"
using Andromeda::Account::Account;

namespace AndromedaUtil {

/*****************************************************/
void Actions::RunAction(int argc, const char* const* argv)
{
    if (argc < 1) throw BaseOptions::BadUsageException("Missing action");

    const std::string action { argv[0] };
    --argc; ++argv;

    if (action == "random")             Random(argc,argv);
    else if (action == "getpasskey")    GetPasskey(argc, argv);
    else if (action == "inite2ee")      InitAccountE2ee(argc,argv); 
    else if (action == "initfse2ee")    InitFilesystemE2ee(argc,argv); 
    else if (action == "changepw")      ChangePassword(argc,argv);

    else throw BaseOptions::BadUsageException("Invalid action");
}

/** Options parser for the Random action */
struct RandomOptions : public BaseOptions
{
    // TODO RAY !! help text

    bool AddFlag(const std::string& flag) override
    {
        if (flag == "base64")
            base64 = true;
        else if (flag == "alphanum")
            alphanum = true;
        else return false;
        return true;
    }

    bool AddOption(const std::string& option, const std::string& value) override
    {
        if (option == "size" || option == "length")
            length = static_cast<decltype(length)>(GetUnsigned(option, value));
        else return false;
        return true;
    }

    bool base64 { false };
    bool alphanum { false };
    size_t length { 64 };
};

/*****************************************************/
void Actions::Random(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
    RandomOptions options;
    options.ParseArgs(static_cast<size_t>(argc), argv);

    std::string ret;
    
    if (options.alphanum)
        ret = StringUtil::Random(options.length);
    else ret = Crypto::GenerateRandom(options.length);
    
    if (options.base64) 
        ret = StringUtil::base64_encode(ret);

    std::cout << ret;

    if (options.alphanum || options.base64)
        std::cout << std::endl;
}

/** Options parser for the GetPasskey action */
struct GetPasskeyOptions : public BaseOptions
{
    // TODO RAY !! help text

    bool AddFlag(const std::string& flag) override { return false; }

    bool AddOption(const std::string& option, const std::string& value) override
    {
        if (option == "username")
            username = value;
        else return false;
        return true;
    }

    void Validate() const override
    {
        if (username.empty())
            throw MissingOptionException("username");
    }

    std::string username;
};

/*****************************************************/
void Actions::GetPasskey(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
    GetPasskeyOptions options;
    options.ParseArgs(static_cast<size_t>(argc), argv);

    if (mResource.GetOptions().isQuiet())
        throw Options::BadUsageException("quiet prevents password prompt");

    std::cout << "Password? ";
    const SecureBuffer password { PlatformUtil::SecureReadConsole() };

    const std::string authkey { Account::GetPasskeys(mResource.GetBackend(), options.username, password).authkey };
    std::cout << StringUtil::base64_encode(authkey) << std::endl;
}

/*****************************************************/
void Actions::InitAccountE2ee(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");

    // TODO RAY !! or should this just be internal to the backend? if not, what exception to use here?
    //const BackendImpl& backend { mResource.GetBackend() };
    //if (!backend.UsingAccount()) throw BackendImpl::AuthenticationFailedException();
}

/*****************************************************/
void Actions::InitFilesystemE2ee(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
}

/*****************************************************/
void Actions::ChangePassword(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
}

} // namespace AndromedaUtil

