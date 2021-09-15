#include "Backend.hpp"
#include "Runner.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <iostream>
#include <utility>

Backend::Backend(Runner& runner) : 
    runner(runner), debug("Backend",this)
{
    this->config.Initialize(RunAction("server","getconfig"));
}

nlohmann::json Backend::RunAction(const std::string& app, const std::string& action)
{
    std::string resp = this->runner.RunAction("server", "getconfig");
    this->debug << __func__ << ": resp:" << resp; this->debug.Print();

    nlohmann::json val; try
    {
        val = nlohmann::json::parse(resp);
    }
    catch (const nlohmann::json::exception& ex)
    {
        throw JSONErrorException(ex.what());
    }

    this->debug << __func__ << ": json:" << val.dump(4); this->debug.Print();

    // TODO do our usual andromeda response checks

    return std::move(val);
}

void Backend::Authenticate(const std::string& username)
{
//    std::cout << "Password?" << std::endl;
//    std::string password; std::getline(std::cin, password);
}