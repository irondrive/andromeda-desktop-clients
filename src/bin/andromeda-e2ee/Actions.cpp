
#include <iostream>
#include <optional>
#include <sstream>

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
#include "andromeda/account/Session.hpp"
using Andromeda::Account::Session;
#include "andromeda/account/SessionOptions.hpp"

namespace AndromedaE2ee {

/** Options parser for the Random action */
struct RandomOptions : public BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText() { return "[--size uint32] [--alphanum] [--base64]"; }

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
    options.Validate();

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

/*****************************************************/
void Actions::GetPasskey(const int argc, const char* const* const argv)
{
    const std::string username{ mResource.GetOptions().sessionOptions.username };
    if (username.empty()) throw Options::MissingOptionException("username");

    (void)mResource.GetBackend(); // init before asking for password

    const SecureBuffer password { mResource.GetOptions().sessionOptions.RequirePassword(mResource.GetOptions().mQuiet) };

    const std::string authsubkey { Account::GetPasskeys(mResource.GetBackend(), username, password).authsubkey };
    std::cout << StringUtil::base64_encode(authsubkey) << std::endl;
}

/** Options parser for the InitAccountKeys action */
struct InitAccountKeysOptions : public BaseOptions
{
    /** Retrieve the standard help text string */
    static std::string HelpText() { return "[--allow_pwlogin [bool]] [--force]"; }

    bool AddFlag(const std::string& flag) override
    {
        if (flag == "allow_pwlogin")
            addpwkey = std::make_optional<bool>(true);
        else if (flag == "force")
            force = true;
        else return false;
        return true;
    }

    bool AddOption(const std::string& option, const std::string& value) override
    {
        if (option == "allow_pwlogin")
            addpwkey = std::make_optional<bool>(StringUtil::stringToBool(value));
        else return false;
        return true;
    }

    std::optional<bool> addpwkey;
    bool force { false };
};

/*****************************************************/
void Actions::InitAccountKeys(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");
    InitAccountKeysOptions options;
    options.ParseArgs(static_cast<size_t>(argc), argv);
    options.Validate();

    Account& account { mResource.GetAccount() };
    const SecureBuffer recovery { account.InitE2eeKeys(options.force) };
    const SecureBuffer recoveryb64 { Account::EncodeRecoveryKey(recovery) };

    std::string recoverystr { recoveryb64.Insecure_ToStr() };
    std::cout << "Recovery key (keep this!): " << recoverystr << std::endl;
    StringUtil::Zeroize(recoverystr); // best effort

    if (options.addpwkey == std::nullopt)
        options.addpwkey = (PlatformUtil::MatchConsoleInput("Enable login with password only? [Y] or n: ", {"Y","n"}, "Y") == "Y");

    if (*options.addpwkey)
    {
        SecureBuffer pwsubkey;
        
        if (mResource.TryGetSession() != nullptr)
            pwsubkey = mResource.GetSession().TryGetE2eePwSubkey(); 
        // pwsbukey might still be empty if session was not just created

        if (pwsubkey.empty())
        {
            const SecureBuffer password { mResource.GetOptions().sessionOptions.RequirePassword(mResource.GetOptions().mQuiet) };
            pwsubkey = Account::GetPasskeys(mResource.GetBackend(), account.GetUsername(), password).e2eesubkey;
        }
        
        account.EnableE2eePwKey(pwsubkey);
    }
}

/*****************************************************/
void Actions::TestAccountKeys(const int argc, const char* const* const argv)
{
    MDBG_INFO("(argc:" << argc << ")");

    SecureBuffer password { SecureBuffer::Insecure_FromStr(mResource.GetOptions().sessionOptions.password) };
    SecureBuffer recoveryb64 { SecureBuffer::Insecure_FromStr(mResource.GetOptions().sessionOptions.e2ee_recoveryb64) };

    mResource.GetAccount().UnlockE2eeInteractive(password, recoveryb64, mResource.TryGetSession());

    std::cout << "E2EE master key: " << StringUtil::base64_encode(mResource.GetAccount().Insecure_GetMasterKey()) << std::endl;
}

/*****************************************************/
void Actions::StorePwMasterKey(const int argc, const char* const* const argv) // TODO E2EE
{
    MDBG_INFO("(argc:" << argc << ")");
    // TODO E2EE add --erase option as well
}

/*****************************************************/
void Actions::GenRkMasterKey(const int argc, const char* const* const argv) // TODO E2EE
{
    MDBG_INFO("(argc:" << argc << ")");
}

/*****************************************************/
void Actions::CreateSession(const int argc, const char* const* const argv) // TODO E2EE
{
    MDBG_INFO("(argc:" << argc << ")");
}

/*****************************************************/
void Actions::ChangePassword(const int argc, const char* const* const argv) // TODO E2EE
{
    MDBG_INFO("(argc:" << argc << ")");
}

/*****************************************************/
void Actions::InitFilesystemKeys(const int argc, const char* const* const argv) // TODO E2EE
{
    MDBG_INFO("(argc:" << argc << ")");
}

/*****************************************************/
/*****************************************************/
/*****************************************************/
void Actions::RunAction(int argc, const char* const* argv)
{
    if (argc < 1) throw BaseOptions::BadUsageException("Missing action");

    const std::string action { argv[0] };
    --argc; ++argv;

    if (action == "random")             Random(argc,argv);
    else if (action == "getpasskey")    GetPasskey(argc, argv);

    else if (action == "initacctkeys")  InitAccountKeys(argc,argv); 
    else if (action == "testacctkeys")  TestAccountKeys(argc,argv);
    else if (action == "storepwmaster") StorePwMasterKey(argc,argv);
    else if (action == "genrkmaster")   GenRkMasterKey(argc,argv);

    else if (action == "createsession") CreateSession(argc,argv);
    else if (action == "changepassword") ChangePassword(argc,argv);
    
    else if (action == "initfskeys")    InitFilesystemKeys(argc,argv); 

    else throw BaseOptions::BadUsageException("Invalid action");
}

/*****************************************************/
std::string Actions::HelpText()
{
    std::ostringstream output;

    using std::endl;

    output 
        << "Valid Actions:" << endl

        << "random " << RandomOptions::HelpText() << endl
        << "getpasskey " << endl

        << "initacctkeys " << InitAccountKeysOptions::HelpText() << endl
        << "testacctkeys " << endl
    ;

    return output.str();
}

// TODO FUTURE - some filesystem actions? upload, download, getfolder, etc.
// TODO FUTURE - also share file, getshares, etc. 

} // namespace AndromedaE2ee

