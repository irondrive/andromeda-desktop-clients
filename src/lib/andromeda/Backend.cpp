#include <string>
#include <iostream>
#include <sstream>
#include <map>

#include <nlohmann/json.hpp>

#include "Backend.hpp"
#include "Utilities.hpp"

/*****************************************************/
Backend::Backend(Runner& runner) : 
    runner(runner), debug("Backend",this) { }

/*****************************************************/
Backend::~Backend()
{
    this->debug << __func__ << "()"; this->debug.Info();

    try { CloseSession(); }
    catch(const Utilities::Exception& ex) 
    { 
        this->debug << __func__ << "..." << ex.what(); this->debug.Error();
    }
}

/*****************************************************/
void Backend::Initialize(const Config::Options& options)
{
    this->debug << __func__ << "()"; this->debug.Info();

    this->config.Initialize(*this, options);
}

/*****************************************************/
std::string Backend::RunAction(Backend::Runner::Input& input)
{
    this->reqCount++;

    if (this->debug)
    {
        this->debug << __func__ << "() " << this->reqCount
            << " app:" << input.app << " action:" << input.action;

        for (const auto& [key,val] : input.params)
            this->debug << " " << key << ":" << val;

        for (const auto& [key,file] : input.files)
            this->debug << " " << key << ":" << file.name << ":" << file.data.size();

        this->debug.Backend();
    }

    if (!this->sessionID.empty())
    {
        input.params["auth_sessionid"] = this->sessionID;
        input.params["auth_sessionkey"] = this->sessionKey;
    }
    else if (!this->username.empty())
    {
        input.params["auth_sudouser"] = this->username;
    }

    return this->runner.RunAction(input);
}

/*****************************************************/
nlohmann::json Backend::GetJSON(const std::string& resp)
{
    try {
        nlohmann::json val(nlohmann::json::parse(resp));

        this->debug << __func__ << "... json:" << val.dump(4); this->debug.Info();

        if (val.at("ok").get<bool>())
            return val.at("appdata");
        else
        {
            const int code = val.at("code").get<int>();
            const auto [message, details] = Utilities::split(
                val.at("message").get<std::string>(),":");
            
            this->debug << __func__ << "()... message:" << message; this->debug.Backend();

                 if (code == 400 && message == "FILESYSTEM_MISMATCH")         throw UnsupportedException();
            else if (code == 400 && message == "STORAGE_FOLDERS_UNSUPPORTED") throw UnsupportedException();
            else if (code == 400 && message == "ACCOUNT_CRYPTO_NOT_UNLOCKED") throw DeniedException(message);
            else if (code == 403 && message == "AUTHENTICATION_FAILED") throw AuthenticationFailedException();
            else if (code == 403 && message == "TWOFACTOR_REQUIRED")    throw TwoFactorRequiredException();
            else if (code == 403 && message == "READ_ONLY_DATABASE")   throw ReadOnlyException("Database");
            else if (code == 403 && message == "READ_ONLY_FILESYSTEM") throw ReadOnlyException("Filesystem");

            else if (code == 403) throw DeniedException(message); 
            else if (code == 404) throw NotFoundException(message);
            else throw APIException(code, message);
        }
    }
    catch (const nlohmann::json::exception& ex) 
    {
        std::ostringstream message; message << 
            ex.what() << " ... body:" << resp;
        throw JSONErrorException(message.str()); 
    }
}

/*****************************************************/
void Backend::Authenticate(const std::string& username, const std::string& password, const std::string& twofactor)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Info();

    CloseSession();

    Runner::Input input { "accounts", "createsession", {{ "username", username }, { "auth_password", password }}};

    if (!twofactor.empty()) input.params["auth_twofactor"] = twofactor;

    nlohmann::json resp(GetJSON(RunAction(input)));

    try
    {
        this->createdSession = true;

        resp.at("account").at("id").get_to(this->accountID);
        resp.at("client").at("session").at("id").get_to(this->sessionID);
        resp.at("client").at("session").at("authkey").get_to(this->sessionKey);

        this->debug << __func__ << "... sessionID:" << this->sessionID; this->debug.Info();
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }

    SetUsername(username);
    config.LoadAccountLimits(*this);
}

/*****************************************************/
void Backend::AuthInteractive(const std::string& username, std::string password, bool forceSession)
{
    this->debug << __func__ << "(username:" << username << ")"; this->debug.Info();

    if (this->runner.RequiresSession() || forceSession)
    {
        if (password.empty())
        {
            std::cout << "Password? ";
            Utilities::SilentReadConsole(password);
        }

        try
        {
            Authenticate(username, password);
        }
        catch (const TwoFactorRequiredException& e)
        {
            std::string twofactor; std::cout << "Two Factor? ";
            Utilities::SilentReadConsole(twofactor);

            Authenticate(username, password, twofactor);
        }
    }
    else SetUsername(username);
}

/*****************************************************/
void Backend::PreAuthenticate(const std::string& sessionID, const std::string& sessionKey)
{
    this->debug << __func__ << "()"; this->debug.Info();

    CloseSession();

    this->sessionID = sessionID;
    this->sessionKey = sessionKey;

    Runner::Input input {"accounts", "getaccount"};

    nlohmann::json resp(GetJSON(RunAction(input)));

    try
    {
        resp.at("id").get_to(this->accountID);
    }
    catch (const nlohmann::json::exception& ex) {
        throw Backend::JSONErrorException(ex.what()); }
}

/*****************************************************/
void Backend::CloseSession()
{
    this->debug << __func__ << "()"; this->debug.Info();
    
    if (this->createdSession)
    {
        Runner::Input input {"accounts", "deleteclient"};

        GetJSON(RunAction(input));
    }

    this->createdSession = false;
    this->sessionID.clear();
    this->sessionKey.clear();
}

/*****************************************************/
void Backend::RequireAuthentication() const
{
    if (this->runner.RequiresSession())
    {
        if (this->sessionID.empty())
            throw AuthRequiredException();
    }
    else
    {
        if (this->sessionID.empty() && this->username.empty())
            throw AuthRequiredException();
    }
}

/*****************************************************/
bool Backend::isMemory() const
{
    return this->config.GetOptions().cacheType == Config::Options::CacheType::MEMORY;
}

/*****************************************************/
nlohmann::json Backend::GetConfigJ()
{
    this->debug << __func__ << "()"; this->debug.Info();

    nlohmann::json config;

    {
        Runner::Input input {"core", "getconfig"};
        config["core"] = GetJSON(RunAction(input));
    }

    {
        Runner::Input input {"files", "getconfig"};
        config["files"] = GetJSON(RunAction(input));
    }

    return config;
}

/*****************************************************/
nlohmann::json Backend::GetAccountLimits()
{
    if (this->accountID.empty())
        return nullptr;

    Runner::Input input {"files", "getlimits", {{"account", this->accountID}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::GetFolder(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    if (isMemory() && id.empty())
    {
        nlohmann::json retval;

        retval["files"] = std::map<std::string,int>();
        retval["folders"] = std::map<std::string,int>();

        return retval;
    }

    Runner::Input input {"files", "getfolder"}; 
    
    if (!id.empty()) input.params["folder"] = id;

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::GetFSRoot(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    Runner::Input input {"files", "getfolder"}; 
    
    if (!id.empty()) input.params["filesystem"] = id;

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::GetFilesystem(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    if (isMemory() && id.empty()) return nullptr;

    Runner::Input input {"files", "getfilesystem"};

    if (!id.empty()) input.params["filesystem"] = id;

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::GetFSLimits(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    if (isMemory() && id.empty()) return nullptr;

    Runner::Input input {"files", "getlimits", {{"filesystem", id}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::GetFilesystems()
{
    this->debug << __func__ << "()"; this->debug.Info();

    Runner::Input input {"files", "getfilesystems"};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::GetAdopted()
{
    this->debug << __func__ << "()"; this->debug.Info();

    Runner::Input input {"files", "listadopted"};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::CreateFile(const std::string& parent, const std::string& name)
{
    this->debug << __func__ << "(parent:" << parent << " name:" << name << ")"; this->debug.Info();

    if (isMemory())
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"size", 0}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};
        
        return retval;
    }

    Runner::Input input {"files", "upload", {{"parent", parent}, {"name", name}}, {{"file", {name, ""}}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::CreateFolder(const std::string& parent, const std::string& name)
{
    this->debug << __func__ << "(parent:" << parent << " name:" << name << ")"; this->debug.Info();

    if (isMemory())
    {
        nlohmann::json retval {{"id", ""}, {"name", name}, {"filesystem", ""}};

        retval["dates"] = {{"created",0},{"modified",nullptr},{"accessed",nullptr}};

        retval["files"] = std::map<std::string,int>(); 
        retval["folders"] = std::map<std::string,int>();

        return retval;
    }

    Runner::Input input {"files", "createfolder", {{"parent", parent},{"name", name}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
void Backend::DeleteFile(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    if (isMemory()) return;

    Runner::Input input {"files", "deletefile", {{"file", id}}};
    
    try { GetJSON(RunAction(input)); }
    catch (const NotFoundException& ex) { }
}

/*****************************************************/
void Backend::DeleteFolder(const std::string& id)
{
    this->debug << __func__ << "(id:" << id << ")"; this->debug.Info();

    if (isMemory()) return;

    Runner::Input input {"files", "deletefolder", {{"folder", id}}}; 
    
    try { GetJSON(RunAction(input)); }
    catch (const NotFoundException& ex) { }
}

/*****************************************************/
nlohmann::json Backend::RenameFile(const std::string& id, const std::string& name, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " name:" << name << ")"; this->debug.Info();

    if (isMemory()) return nullptr;

    Runner::Input input {"files", "renamefile", {{"file", id}, {"name", name}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::RenameFolder(const std::string& id, const std::string& name, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " name:" << name << ")"; this->debug.Info();

    if (isMemory()) return nullptr;

    Runner::Input input {"files", "renamefolder", {{"folder", id}, {"name", name}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::MoveFile(const std::string& id, const std::string& parent, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " parent:" << parent << ")"; this->debug.Info();

    if (isMemory()) return nullptr;

    Runner::Input input {"files", "movefile", {{"file", id}, {"parent", parent}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::MoveFolder(const std::string& id, const std::string& parent, bool overwrite)
{
    this->debug << __func__ << "(id:" << id << " parent:" << parent << ")"; this->debug.Info();

    if (isMemory()) return nullptr;

    Runner::Input input {"files", "movefolder", {{"folder", id}, {"parent", parent}, {"overwrite", overwrite?"true":"false"}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
std::string Backend::ReadFile(const std::string& id, const size_t offset, const size_t length)
{
    std::string fstart(std::to_string(offset));
    std::string flast(std::to_string(offset+length-1));

    this->debug << __func__ << "(id:" << id << " fstart:" << fstart << " flast:" << flast; this->debug.Info();

    if (isMemory()) return std::string(length,'\0');

    Runner::Input input {"files", "download", {{"file", id}, {"fstart", fstart}, {"flast", flast}}};

    std::string data(RunAction(input));

    if (data.size() != length) throw ReadSizeException(length, data.size());

    return data;
}

/*****************************************************/
nlohmann::json Backend::WriteFile(const std::string& id, const size_t offset, const std::string& data)
{
    this->debug << __func__ << "(id:" << id << " offset:" << offset << " size:" << data.size(); this->debug.Info();

    if (isMemory()) return nullptr;

    Runner::Input input {"files", "writefile", {{"file", id}, {"offset", std::to_string(offset)}}, {{"data", {"data", data}}}};

    return GetJSON(RunAction(input));
}

/*****************************************************/
nlohmann::json Backend::TruncateFile(const std::string& id, const size_t size)
{
    this->debug << __func__ << "(id:" << id << " size:" << size << ")"; this->debug.Info();

    if (isMemory()) return nullptr;

    Runner::Input input {"files", "ftruncate", {{"file", id}, {"size", std::to_string(size)}}};

    return GetJSON(RunAction(input));
}
