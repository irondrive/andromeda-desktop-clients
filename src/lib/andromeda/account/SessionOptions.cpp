
#include "SessionOptions.hpp"
#include "andromeda/backend/BackendImpl.hpp"
using Andromeda::Backend::BackendImpl;

namespace Andromeda::Account {

/*****************************************************/
std::string SessionOptions::HelpText()
{
    return "Remote Auth:     [-u|--username str] [--password str] | [--sessionid id] [--sessionkey key] [--force-session]";
}

/*****************************************************/
bool SessionOptions::AddFlag(const std::string& flag)
{
    if (flag == "force-session")
        forceSession = true;
    else return false;

    return true;
}

/*****************************************************/
bool SessionOptions::AddOption(const std::string& option, const std::string& value)
{
    if (option == "u" || option == "username")
        username = value;
    else if (option == "password")
        password = value;
    else if (option == "sessionid")
        sessionid = value;
    else if (option == "sessionkey")
        sessionkey = value;
    else return false;

    return true;
}

/*****************************************************/
std::unique_ptr<Session> SessionOptions::GetSession(BackendImpl& backend, bool interactive) const
{
    if (!sessionid.empty())
        return std::make_unique<Session>(Session::FromExisting(backend, sessionid, sessionkey));
    else if (!username.empty())
    {
        if (backend.RequiresSession() || forceSession || !password.empty())
        {
            // putting a password on the command line is already insecure anyway
            const SecureBuffer pbuf { SecureBuffer::Insecure_FromBuf(password.data(), password.size()) };

            if (interactive)
                return std::make_unique<Session>(Session::CreateInteractive(backend, username, pbuf));
            else return std::make_unique<Session>(Session::Create(backend, username, pbuf));
        }
    }
    return nullptr;
}

/*****************************************************/
void SessionOptions::Validate() const
{
    // TODO FUTURE mounting shares - check GetMountRootType() != RootType::FOLDER
    if (username.empty() && sessionid.empty())
        throw MissingOptionException("username/sessionid");
}

} // namespace Andromeda::Account