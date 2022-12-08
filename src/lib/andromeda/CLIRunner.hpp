#ifndef LIBA2_CLIRUNNER_H_
#define LIBA2_CLIRUNNER_H_

#include <chrono>
#include <string>
#include <system_error>

#include "BaseRunner.hpp"
#include "Debug.hpp"

namespace Andromeda {

struct RunnerInput;

/** Runs the API locally by invoking it as a process */
class CLIRunner : public BaseRunner
{
public:

    /** Exception indicating the CLI runner had an error */
    class Exception : public EndpointException { public: 
        /** @param msg error message string */
        explicit Exception(const std::string& msg) : 
            EndpointException("Subprocess Error: "+msg) {} };

    /** @param apiPath path to the API index.php */
    explicit CLIRunner(const std::string& apiPath);

    virtual std::string GetHostname() const override { return "local"; }

    virtual std::string RunAction(const RunnerInput& input) override;

    virtual bool RequiresSession() override { return false; }

private:

    Debug mDebug;

    std::string mApiPath;

    const std::chrono::seconds mTimeout { std::chrono::seconds(120) };
};

} // namespace Andromeda

#endif

